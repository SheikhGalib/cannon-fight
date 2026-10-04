#include "FortGate.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <cmath>

FortGate::FortGate(glm::vec3 centreWorld,
                   float width, float height, float depth,
                   float gateWidth, int rows)
{
    // The wall is `width` wide total and `height` tall. The doorway is
    // `gateWidth` wide, centred on centreWorld.x. The lintel sits on top of
    // the doorway. The two pillars sit just outside the doorway, anchoring
    // the lintel.
    //
    //        leftPillarX          rightPillarX
    //              |                  |
    //              v                  v
    //          +---+------------------+---+
    //          |   |     gateWidth    |   |
    //          | P |     (doorway)    | P |  <-- pillars (height tall, depth thick)
    //          |   |                  |   |
    //          |   +------------------+   |  <-- lintel (spans the gate)
    //          | brick brick   brick brick |
    //          | brick brick   brick brick |  <-- rows of bricks under the lintel
    //          | brick brick   brick brick |
    //          +--------------------------+
    //                                     ^--- ground
    //
    // Bricks fill the spaces to the LEFT and RIGHT of the doorway, between
    // the lintel and the ground. Each brick is the same size so they tile.
    const float halfWidth = width / 2.0f;
    const float halfGate = gateWidth / 2.0f;
    const float leftPillarX  = centreWorld.x - halfGate;
    const float rightPillarX = centreWorld.x + halfGate;
    const float leftOuterX   = centreWorld.x - halfWidth;
    const float rightOuterX  = centreWorld.x + halfWidth;

    // ---- pillars (non-breakable) --------------------------------------
    const float pillarWidth = (halfGate - halfWidth) * -1.0f;  // |leftPillar - leftOuter|
    const float pillarHeight = height;
    const float pillarDepth = depth;

    for (float px : { leftPillarX, rightPillarX }) {
        // Pick the OUTER edge as the centre reference; pillar spans from
        // its outer edge inward to the gate edge.
        float xMid = (px == leftPillarX)
            ? (leftPillarX + leftOuterX) * 0.5f
            : (rightPillarX + rightOuterX) * 0.5f;
        staticParts.push_back({
            Primitives::CreateBox(pillarWidth, pillarHeight, pillarDepth, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                           glm::vec3(xMid, pillarHeight * 0.5f, centreWorld.z))
        });
    }

    // ---- lintel (non-breakable) ----------------------------------------
    // The lintel is a single thick bar sitting on top of the doorway and
    // resting on the inner face of each pillar.
    const float lintelHeight = 0.30f;
    const float lintelY = pillarHeight + lintelHeight * 0.5f;
    staticParts.push_back({
        Primitives::CreateBox(gateWidth + 0.10f, lintelHeight, pillarDepth, Palette::Wood),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(centreWorld.x, lintelY, centreWorld.z))
    });

    // ---- breakable bricks ----------------------------------------------
    // Bricks fill the wall BELOW the lintel, in two stacks: one to the left
    // of the doorway, one to the right. Each stack has the same height and
    // number of rows so they line up with each other and with the lintel.
    //
    // We size the bricks so they fill (rows * brickHeight) = (lintelY - lintelHeight/2),
    // and (cols * brickWidth) = pillarWidth on each side. The exact number
    // of cols follows from the brick size; we pick a brick size, then count.
    const float underLintelY = lintelY - lintelHeight * 0.5f;  // top of brick zone
    const float brickH = underLintelY / float(rows);           // auto-size to fit
    const float brickD = pillarDepth * 0.95f;                  // slightly thinner than the wall

    // Left stack: cols fill the left pillar's horizontal span.
    int leftCols = std::max(1, int(std::floor(pillarWidth / brickH)));
    const float leftBrickW = pillarWidth / float(leftCols);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < leftCols; c++) {
            float x = leftOuterX + (float(c) + 0.5f) * leftBrickW;
            float y = (float(r) + 0.5f) * brickH;
            float z = centreWorld.z;
            // push_back with brace-init: Mesh has no default constructor
            // (it owns GPU buffer IDs), so we have to build it in place.
            bricks.push_back({
                Primitives::CreateBox(leftBrickW, brickH, brickD, Palette::Stone),
                glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)),
                true
            });
        }
    }

    // Right stack: mirror of the left.
    int rightCols = leftCols;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < rightCols; c++) {
            float x = rightPillarX + (float(c) + 0.5f) * leftBrickW;
            float y = (float(r) + 0.5f) * brickH;
            float z = centreWorld.z;
            bricks.push_back({
                Primitives::CreateBox(leftBrickW, brickH, brickD, Palette::Stone),
                glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)),
                true
            });
        }
    }

    // Cache the brick size used for the collision test. Both stacks use the
    // same brickH and brickD; leftBrickW is the same on both sides too.
    brickSize = glm::vec3(leftBrickW, brickH, brickD);
}

bool FortGate::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
    bool anyKilled = false;
    glm::vec3 half = brickSize * 0.5f;
    for (Brick& b : bricks) {
        if (!b.alive) continue;
        glm::vec3 centre(b.local[3]);
        glm::vec3 closest(
            std::fmax(-half.x, std::fmin(sphereCentre.x - centre.x, half.x)),
            std::fmax(-half.y, std::fmin(sphereCentre.y - centre.y, half.y)),
            std::fmax(-half.z, std::fmin(sphereCentre.z - centre.z, half.z))
        );
        if (glm::dot(closest, closest) <= sphereRadius * sphereRadius) {
            b.alive = false;
            anyKilled = true;
        }
    }
    return anyKilled;
}

int FortGate::AliveBrickCount() const {
    int n = 0;
    for (const Brick& b : bricks) if (b.alive) ++n;
    return n;
}

void FortGate::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");

    // Static pieces (lintel, pillars) - always drawn, never broken.
    for (Part& p : staticParts) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        p.mesh.Draw();
    }

    // Breakable bricks - skip the dead ones.
    for (Brick& b : bricks) {
        if (!b.alive) continue;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(b.local));
        b.mesh.Draw();
    }
}

void FortGate::Delete() {
    for (Brick& b : bricks) b.mesh.Delete();
    for (Part& p : staticParts) p.mesh.Delete();
}