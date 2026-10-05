#include "Door.h"
#include "Primitives.h"
#include "Palette.h"
#include "Part.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <cmath>

Door::Door(glm::vec3 centreWorld,
           float doorHeight, float doorWidth, float panelDepth, int plankCount) {
    // Two panels side-by-side, meeting at the doorway centre. Each panel is
    // half the doorWidth. The panel itself is a thin box; the plank strips
    // sit just in front of it (toward +X, the side the cannon is on) so they
    // catch the light and read as raised planks.
    //
    // Phase 5: the door is oriented with its THIN edge along X (the cannon
    // firing axis), so the cannon ball strikes the broad 1 m x 3 m face.
    // The two panels sit side-by-side along Z, meeting at the doorway
    // centre.  Planks poke out the +X side (toward the cannons).
    const float halfDoorW = doorWidth  * 0.5f;
    const float panelW    = doorWidth * 0.5f;   // along Z (was along X)
    const float panelH    = doorHeight;
    const float panelD    = panelDepth;          // along X (thin axis)
    const float plankT    = 0.04f;                // plank thickness
    const float plankW    = panelW * 0.92f;       // slightly narrower than panel (along Z)
    const float plankStep = panelH / float(plankCount + 1);   // evenly spaced

    panelSize = glm::vec3(panelD, panelH, panelW);   // X, Y, Z order

    // Offsets of the two panel CENTRES from the doorway centre, along Z.
    // side=0 -> LEFT  (negative Z);  side=1 -> RIGHT (positive Z).
    const float offsets[2] = { -halfDoorW * 0.5f, +halfDoorW * 0.5f };

    for (size_t side = 0; side < 2; side++) {
        const float cx = centreWorld.x;
        const float cy = doorHeight * 0.5f;
        const float cz = centreWorld.z + offsets[side];

        // Construct the Panel directly in the vector (Panel has no default
        // ctor because Mesh owns GPU buffer IDs - so we build it in place).
        // Box dims: (panelDepth along X, panelHeight along Y, panelWidth along Z).
        //
        // Phase 6: shift the panel +0.5 m along +X (away from the
        // FortGate wall) so the broad 2 m × 3 m face is actually
        // visible in the gate opening rather than buried inside the
        // brick wall.  Without it the door read as a thin brown
        // pillar in screenshots.
        const float xOffset = 0.50f;
        Panel& p = panels.emplace_back(
            Primitives::CreateBox(panelD, panelH, panelW, Palette::Wood),
            glm::translate(glm::mat4(1.0f), glm::vec3(cx + xOffset, cy, cz)),
            std::vector<Part>{},
            /*alive=*/true);
        p.half[0] = panelD * 0.5f;   // X half
        p.half[1] = panelH * 0.5f;   // Y half
        p.half[2] = panelW * 0.5f;   // Z half

        // Each panel hinges on its INNER vertical edge (the one facing
        // the doorway centre), at ground height.  This is where the real
        // iron hinges would be on a castle door; pinning the rotation
        // here makes the smashed door read as "hinged on one edge and
        // swinging outward" rather than tumbling end-over-end.
        //
        // LEFT panel (side=0, cz = centre - halfDoorW/2) -> +Z is the
        // inner edge; RIGHT panel (side=1, cz = centre + halfDoorW/2) ->
        // -Z is the inner edge.
        const float hingeZ = (side == 0) ? (cz + p.half[2])
                                        : (cz - p.half[2]);
        p.hingeWorld = glm::vec3(cx + xOffset, 0.0f, hingeZ);

        // Horizontal plank strips, evenly spaced vertically, sitting just
        // in front of the panel face. The plank's centre X is panel's CX +
        // half-depth + half-plank-thickness (so it pokes out the +X side,
        // toward the cannons).
        for (int i = 0; i < plankCount; i++) {
            const float plankY = (float(i) + 1.0f) * plankStep;
            p.planks.push_back({
                Primitives::CreateBox(plankT, plankT, plankW, Palette::WoodLight),
                glm::translate(glm::mat4(1.0f),
                               glm::vec3(cx + xOffset + panelD * 0.5f + plankT * 0.5f,
                                         plankY,
                                         cz))
            });
        }
    }
}

// Damage / break-physics tuning ----------------------------------------
// A direct hit by a cannon ball applies 25% damage to the panel
// (kDamagePerHit). While health > 0 the panel stays attached but is
// visibly tilted by `kTiltPerDamageFraction * damageFraction` degrees
// around its bottom edge - a damaged panel leans forward into the
// doorway, reading as "battered but still holding".
//
// At health <= 0 the panel detaches and gets a break-physics push:
//   - omega.y (around Y, "swing open") is the dominant term; it makes
//     the panel rotate around its inner vertical hinge like a real door
//     being smashed.
//   - omega.x (around X, "pitch forward") is a small secondary term so
//     the door tips over and falls into the compound rather than
//     staying vertical as it swings.
//
// linearVel is mostly to bump the whole rigid panel along +X (into the
// compound interior) on the first frames, so it doesn't scrape along
// the threshold.  After the initial kick, gravity (handled implicitly
// via the angular velocity tipping it forward) carries it the rest of
// the way.
static const float kDamagePerHit         = 0.25f;  // 25% per shot
static const float kTiltPerDamageFraction = 15.0f;  // deg at 100% damage
static const float kHitSpinY             = 6.5f;   // rad / sec around Y
static const float kHitPitchX            = -1.2f;  // rad / sec around X (top falls +X)
static const float kHitKickX             = 1.2f;   // m / sec initial +X shove
static const float kHitSideImpulse       = 0.8f;   // m / sec along the side the ball hit

bool Door::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
    bool anyHit = false;
    for (Panel& p : panels) {
        if (!p.alive) continue;
        glm::vec3 centre(p.local[3]);
        // Offset of the sphere from the panel centre, then clamped per-axis to
        // the panel's half-extents: that clamp gives the OFFSET of the
        // closest point on the panel box from the box centre. Subtract that
        // from the raw offset to get the actual sphere-to-closest-point
        // vector (zero when the sphere centre is inside the box).
        glm::vec3 offset(
            sphereCentre.x - centre.x,
            sphereCentre.y - centre.y,
            sphereCentre.z - centre.z);
        glm::vec3 clamped(
            std::fmax(-p.half[0], std::fmin(offset.x, p.half[0])),
            std::fmax(-p.half[1], std::fmin(offset.y, p.half[1])),
            std::fmax(-p.half[2], std::fmin(offset.z, p.half[2])));
        glm::vec3 delta = offset - clamped;
        if (glm::dot(delta, delta) <= sphereRadius * sphereRadius) {
            // Phase 7: each cannon hit strips kDamagePerHit of health.
            // If health stays > 0 the panel just tilts (visualised in
            // Draw via a matrix around the panel's bottom edge). If
            // health <= 0 we detach and apply the Phase 5 break-physics
            // impulses so the panel flies off.
            p.health -= kDamagePerHit;
            if (p.health <= 0.0f) {
                // Detached: existing break-physics.
                // Sign of omega.y depends on which panel was hit so both
                // panels rotate OUT of the doorway (the LEFT panel spins
                // around -Y so its outer +Z edge goes -Z, away from the
                // gatehouse centre; the RIGHT panel spins +Y so its outer
                // -Z edge goes +Z).
                //
                // The ball came from the -X side (cannons at x = -15), so
                // the panel must rotate such that its outer face swings
                // away from the doorway's center, i.e. it opens OUTWARD
                // away from the gatehouse.
                const float spinSign = (centre.z < 0.0f) ? +1.0f : -1.0f;
                // Where along the panel the ball struck, along Z: +1 means
                // outer edge, -1 means inner edge (the hinge side).  The
                // closer to the OUTER edge, the faster the spin (a ball
                // dead-centre on the door just pushes; a ball on the outer
                // edge yanks it open).
                const float edgeFactor = (offset.z / std::max(p.half[2], 1e-3f));
                const float spinMag = kHitSpinY * (0.6f + 0.6f * std::fabs(edgeFactor));
                p.angularVel = glm::vec3(kHitPitchX,
                                         spinSign * spinMag,
                                         0.0f);
                // Initial shove along +X (into the compound) plus a small
                // extra push along the side the ball hit, so panels don't
                // all fall in identical straight lines.
                p.linearVel  = glm::vec3(kHitKickX,
                                         0.0f,
                                         spinSign * kHitSideImpulse * edgeFactor);
                // Snap the rotation accumulator to zero (should already be,
                // but be explicit) and mark the panel as in motion.
                p.euler      = glm::vec3(0.0f);
                p.flying     = true;
            }
            anyHit = true;
        }
    }
    return anyHit;
}

void Door::Update(float deltaTime) {
    for (Panel& p : panels) {
        if (!p.flying) continue;
        // Integrate angular velocity -> accumulated Euler angles.
        p.euler += p.angularVel * deltaTime;
        // Integrate linear velocity into the hinge position (so the
        // whole rigid body translates through space).  No gravity here:
        // the angular pitch already makes the top of the panel fall
        // forward, and the panel slides off the threshold on its own
        // once it tips past ~30 degrees.
        p.hingeWorld += p.linearVel * deltaTime;
        // Mild angular damping so the door doesn't spin forever if a
        // ball hits an edge tangentially.  A small per-frame multiplier
        // reads as "wood hinges grinding to a halt in the dirt".
        p.angularVel *= (1.0f - 0.6f * deltaTime);
        // Bail out of integration once the panel is more than 25 m
        // from the hinge in any direction (i.e. it's well clear of the
        // castle, lying in the dirt).  We kill `alive` so Draw stops
        // touching it.
        if (glm::length(p.hingeWorld) > 25.0f ||
            std::fabs(p.euler.x) > 6.28f ||
            std::fabs(p.euler.y) > 6.28f) {
            p.alive  = false;
            p.flying = false;
        }
    }
}

int Door::AlivePanelCount() const {
    int n = 0;
    for (const Panel& p : panels) if (p.alive) ++n;
    return n;
}

void Door::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    for (Panel& p : panels) {
        if (!p.alive) continue;
        if (!p.flying) {
            // Phase 7: if the panel has taken damage but is still
            // attached, tilt it forward (about its bottom edge) by
            // kTiltPerDamageFraction * damageFraction degrees.  The
            // tilt visually says "battered but still holding"; a fresh
            // hit also visibly shifts the planks along with the panel.
            if (p.health < 1.0f) {
                float damageFraction = 1.0f - p.health;  // 0..1
                float tiltDeg = kTiltPerDamageFraction * damageFraction;
                // Bottom edge: y = 0 (panel extends from y=0 to y=panelH),
                // so the pivot in world space is (panelCentre.x, 0, panelCentre.z).
                glm::vec3 panelCentre(p.local[3]);
                glm::vec3 pivotWorld(panelCentre.x, 0.0f, panelCentre.z);
                glm::mat4 R = glm::rotate(glm::mat4(1.0f),
                                          glm::radians(tiltDeg),
                                          glm::vec3(0.0f, 0.0f, 1.0f));
                // We want R to tip the panel TOP toward +X (forward into
                // the compound), so a positive rotation around +Z sends
                // (+Y,0,0) toward (+X,0,0) -- correct.
                glm::mat4 tilted = glm::translate(glm::mat4(1.0f), pivotWorld)
                                 * R
                                 * glm::translate(glm::mat4(1.0f), -pivotWorld)
                                 * p.local;
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(tilted));
                p.mesh.Draw();
                // Planks live in world space too; rebuild them under
                // the same tilt. plank.local is its translation from
                // world origin; we re-anchor it relative to the panel
                // pivot then apply the tilt + pivot translation.
                for (Part& plank : p.planks) {
                    glm::vec3 plankWorldFromPivot(glm::vec3(plank.local[3]) - pivotWorld);
                    glm::mat4 plankTilted = glm::translate(glm::mat4(1.0f), pivotWorld)
                                          * R
                                          * glm::translate(glm::mat4(1.0f), -pivotWorld)
                                          * plank.local;
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(plankTilted));
                    plank.mesh.Draw();
                }
            } else {
                // Untouched resting pose: just the panel + planks as built.
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
                p.mesh.Draw();
                for (Part& plank : p.planks) {
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(plank.local));
                    plank.mesh.Draw();
                }
            }
        } else {
            // Flying pose: rotate the rest pose about the panel's hinge
            // by the accumulated Euler angles, then translate by the
            // hinge's world position.  Planks ride along: they live in
            // panel-LOCAL space (a translation from panel centre), and
            // we build each plank's matrix as R * plankLocal * T where
            // T is the translation from panel centre to the panel's
            // hinge in world space.
            glm::mat4 R = glm::rotate(glm::mat4(1.0f), p.euler.y, glm::vec3(0,1,0));
            R          = glm::rotate(R,            p.euler.x, glm::vec3(1,0,0));
            // Translation that re-roots the panel at the hinge: panel
            // is centred at its `local[3]` translation, but we want to
            // pivot it about the inner vertical edge.  Subtract the
            // hinge from the panel centre to get the local-space offset
            // from hinge to centre, then re-apply R + hinge.
            glm::vec3 panelCentre(p.local[3]);
            glm::vec3 panelToHinge = panelCentre - p.hingeWorld;
            glm::mat4 worldR = glm::translate(
                glm::mat4(1.0f), p.hingeWorld)
              * R
              * glm::translate(glm::mat4(1.0f), panelToHinge);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(worldR));
            p.mesh.Draw();
            for (Part& plank : p.planks) {
                // plank.local is its own translation from world origin;
                // we want it relative to the PANEL CENTRE first, then
                // through R + hinge translation.
                glm::vec3 plankCentre(plank.local[3]);
                glm::vec3 plankToPanel = plankCentre - panelCentre;
                glm::mat4 plankWorldR = glm::translate(
                    glm::mat4(1.0f), p.hingeWorld)
                  * R
                  * glm::translate(glm::mat4(1.0f), panelToHinge + plankToPanel);
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(plankWorldR));
                plank.mesh.Draw();
            }
        }
    }
}

void Door::Delete() {
    for (Panel& p : panels) {
        p.mesh.Delete();
        for (Part& plank : p.planks) plank.mesh.Delete();
    }
}