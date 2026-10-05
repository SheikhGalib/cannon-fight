#include "FortGate.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <cmath>

// Phase 7: damage-per-hit tuning.  A direct cannon-ball hit strips
// 10% of a brick's health; at 0 the brick is removed.  Each hit
// rebuilds the brick's mesh at a darker shade so the cumulative damage
// is visible without per-brick shader hacks.
static const float kDamagePerHit = 0.10f;

// Helper: lerp between Stone (untouched) and StoneDark (heavily
// damaged) by `t` in [0, 1].  We rebuild the brick's mesh on each hit
// so the colour shift is permanent rather than a per-frame uniform
// trick.
static glm::vec3 DamageColour(float health) {
    float t = glm::clamp(1.0f - health, 0.0f, 1.0f);
    return Palette::Stone * (1.0f - t) + Palette::StoneDark * t;
}

FortGate::FortGate(glm::vec3 centreWorld,
                   float width, float height, float depth,
                   float gateWidth, int rows,
                   bool alongZ)
{
    const float halfWidth = width / 2.0f;
    const float halfGate = gateWidth / 2.0f;
    const float lintelHeight = 0.30f;
    const float lintelY = height + lintelHeight * 0.5f;
    const float underLintelY = lintelY - lintelHeight * 0.5f;
    const float brickH = underLintelY / float(rows);

    if (alongZ) {
        // Wall runs along Z (facing X, the cannon axis). The door is in the YZ
        // plane. The two stone pillars flank the door on -Z and +Z, parallel
        // to the door, anchoring the wooden lintel that spans across the top.
        const float pillarWidthZ = (halfWidth - halfGate);
        const float pillarHeight = height;
        const float pillarDepthX = depth;

        const float leftPillarZ  = centreWorld.z - halfGate;
        const float rightPillarZ = centreWorld.z + halfGate;
        const float leftOuterZ   = centreWorld.z - halfWidth;
        const float rightOuterZ  = centreWorld.z + halfWidth;

        for (float pz : { leftPillarZ, rightPillarZ }) {
            float zMid = (pz == leftPillarZ)
                ? (leftPillarZ + leftOuterZ) * 0.5f
                : (rightPillarZ + rightOuterZ) * 0.5f;
            staticParts.push_back({
                Primitives::CreateBox(pillarDepthX, pillarHeight, pillarWidthZ, Palette::Stone),
                glm::translate(glm::mat4(1.0f),
                               glm::vec3(centreWorld.x, pillarHeight * 0.5f, zMid))
            });
        }

        // Lintel spanning across the doorway along Z
        staticParts.push_back({
            Primitives::CreateBox(pillarDepthX, lintelHeight, width + 0.10f, Palette::Wood),
            glm::translate(glm::mat4(1.0f),
                           glm::vec3(centreWorld.x, lintelY, centreWorld.z))
        });

        // Breakable bricks under the lintel flanking the doorway
        const float brickD = pillarDepthX * 0.95f;
        int leftCols = std::max(1, int(std::floor(pillarWidthZ / brickH)));
        const float leftBrickW = pillarWidthZ / float(leftCols);

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < leftCols; c++) {
                float z = leftOuterZ + (float(c) + 0.5f) * leftBrickW;
                float y = (float(r) + 0.5f) * brickH;
                float x = centreWorld.x;
                bricks.push_back({
                    Primitives::CreateBox(brickD, brickH, leftBrickW, Palette::Stone),
                    glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)),
                    true
                });
            }
        }
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < leftCols; c++) {
                float z = rightPillarZ + (float(c) + 0.5f) * leftBrickW;
                float y = (float(r) + 0.5f) * brickH;
                float x = centreWorld.x;
                bricks.push_back({
                    Primitives::CreateBox(brickD, brickH, leftBrickW, Palette::Stone),
                    glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)),
                    true
                });
            }
        }
        brickSize = glm::vec3(brickD, brickH, leftBrickW);
    } else {
        // Legacy along-X layout
        const float leftPillarX  = centreWorld.x - halfGate;
        const float rightPillarX = centreWorld.x + halfGate;
        const float leftOuterX   = centreWorld.x - halfWidth;
        const float rightOuterX  = centreWorld.x + halfWidth;

        const float pillarWidth = (halfGate - halfWidth) * -1.0f;
        const float pillarHeight = height;
        const float pillarDepth = depth;

        for (float px : { leftPillarX, rightPillarX }) {
            float xMid = (px == leftPillarX)
                ? (leftPillarX + leftOuterX) * 0.5f
                : (rightPillarX + rightOuterX) * 0.5f;
            staticParts.push_back({
                Primitives::CreateBox(pillarWidth, pillarHeight, pillarDepth, Palette::Stone),
                glm::translate(glm::mat4(1.0f),
                               glm::vec3(xMid, pillarHeight * 0.5f, centreWorld.z))
            });
        }

        staticParts.push_back({
            Primitives::CreateBox(gateWidth + 0.10f, lintelHeight, pillarDepth, Palette::Wood),
            glm::translate(glm::mat4(1.0f),
                           glm::vec3(centreWorld.x, lintelY, centreWorld.z))
        });

        const float brickD = pillarDepth * 0.95f;
        int leftCols = std::max(1, int(std::floor(pillarWidth / brickH)));
        const float leftBrickW = pillarWidth / float(leftCols);
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < leftCols; c++) {
                float x = leftOuterX + (float(c) + 0.5f) * leftBrickW;
                float y = (float(r) + 0.5f) * brickH;
                float z = centreWorld.z;
                bricks.push_back({
                    Primitives::CreateBox(leftBrickW, brickH, brickD, Palette::Stone),
                    glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)),
                    true
                });
            }
        }
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < leftCols; c++) {
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
        brickSize = glm::vec3(leftBrickW, brickH, brickD);
    }
}

bool FortGate::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
    bool anyKilled = false;
    glm::vec3 half = brickSize * 0.5f;
    for (Brick& b : bricks) {
        if (!b.alive) continue;
        glm::vec3 centre(b.local[3]);
        // Offset of the sphere from the brick centre, clamped per-axis to the
        // brick's half-extents. Subtract the clamped offset from the raw
        // offset to get the actual sphere-to-closest-point vector (zero when
        // the sphere centre is inside the brick).
        glm::vec3 offset(
            sphereCentre.x - centre.x,
            sphereCentre.y - centre.y,
            sphereCentre.z - centre.z);
        glm::vec3 clamped(
            std::fmax(-half.x, std::fmin(offset.x, half.x)),
            std::fmax(-half.y, std::fmin(offset.y, half.y)),
            std::fmax(-half.z, std::fmin(offset.z, half.z)));
        glm::vec3 delta = offset - clamped;
        if (glm::dot(delta, delta) <= sphereRadius * sphereRadius) {
            // Phase 7: each cannon hit strips kDamagePerHit of health.
            // We rebuild the brick's mesh at a darker shade so the
            // cumulative damage is visible. At health <= 0 the brick
            // is removed.
            b.health -= kDamagePerHit;
            // Discard old mesh, build a new one at the new colour.
            b.mesh.Delete();
            glm::vec3 newColour = DamageColour(b.health);
            b.mesh = Primitives::CreateBox(brickSize.x, brickSize.y, brickSize.z,
                                            newColour);
            if (b.health <= 0.0f) {
                b.alive = false;
                // Free the mesh now; the loop ignores dead bricks.
                b.mesh.Delete();
                anyKilled = true;
            }
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
    if (!isAlive) return;

    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");

    // Static pieces (lintel, pillars)
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