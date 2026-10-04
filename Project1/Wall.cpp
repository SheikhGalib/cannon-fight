#include "Wall.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <cmath>

Wall::Wall(glm::vec3 centreWorld, int rows, int cols, glm::vec3 brickSize)
    : brickSize(brickSize)
{
    bricks.reserve(rows * cols);

    // Wall layout: rows tall, cols wide. The wall's centre is centred on
    // (centreWorld.x, centreWorld.z); brick (row r, col c) sits at:
    //
    //     x = centreWorld.x + (c - (cols-1)/2) * brickSize.x
    //     y = brickSize.y / 2 + r * brickSize.y
    //     z = centreWorld.z
    //
    // (no rounding) so the bricks line up exactly edge to edge.
    //
    // Each brick gets its OWN Mesh - Mesh owns GPU buffer IDs, so two bricks
    // can't share one without a careful ref-count. ~50 small boxes is
    // trivial; the simpler design wins.
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float x = centreWorld.x + (float(c) - float(cols - 1) * 0.5f) * brickSize.x;
            float y = brickSize.y * 0.5f + float(r) * brickSize.y;
            float z = centreWorld.z;
            // Mesh has no default constructor (it owns GPU buffer IDs), so
            // we have to build it right inside the push_back. Same effect.
            bricks.push_back({
                Primitives::CreateBox(brickSize.x, brickSize.y, brickSize.z, Palette::Stone),
                glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)),
                true
            });
        }
    }
}

bool Wall::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
    bool anyKilled = false;
    for (Brick& b : bricks) {
        if (!b.alive) continue;

        // The brick's world-space centre is the translation column of its
        // local matrix (we built it with a pure translate, no rotation, so
        // this is exact).
        glm::vec3 centre(b.local[3]);

        // Standard sphere-vs-AABB closest-point test. The "half-extents" of
        // the brick's bounding box are half its dimensions on each axis.
        glm::vec3 half(brickSize * 0.5f);
        glm::vec3 closest(
            std::fmax(-half.x, std::fmin(sphereCentre.x - centre.x, half.x)),
            std::fmax(-half.y, std::fmin(sphereCentre.y - centre.y, half.y)),
            std::fmax(-half.z, std::fmin(sphereCentre.z - centre.z, half.z))
        );
        float distSq = glm::dot(closest, closest);
        if (distSq <= sphereRadius * sphereRadius) {
            b.alive = false;
            anyKilled = true;
        }
    }
    return anyKilled;
}

int Wall::AliveBrickCount() const {
    int n = 0;
    for (const Brick& b : bricks) if (b.alive) ++n;
    return n;
}

void Wall::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    for (Brick& b : bricks) {
        if (!b.alive) continue;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(b.local));
        b.mesh.Draw();
    }
}

void Wall::Delete() {
    for (Brick& b : bricks) {
        b.mesh.Delete();
    }
}