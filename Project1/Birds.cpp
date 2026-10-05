#include "Birds.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Birds::Birds(int numBirds, glm::vec3 centre, float alt,
             float radiusMin, float radiusMax)
    : centre(centre), altitude(alt),
      // Shared thin-box wing mesh.  Local X = wing length (out
      // from the body), Y = thickness, Z = chord (front-to-back of
      // the wing).  Wing is centred on its origin.
      wingMesh(Primitives::CreateBox(1.0f, 0.05f, 0.30f,
                                      Palette::Bird)) {

    // Deterministic LCG-style PRNG so the flock composition is
    // identical between runs.
    unsigned int seed = 31415u;
    auto rnd = [&]() {
        seed = seed * 1103515245u + 12345u;
        return float((seed >> 8) & 0xFFFFFFu) / float(0xFFFFFFu);
    };

    birds.reserve(numBirds);
    parts.reserve(numBirds * 2);

    for (int i = 0; i < numBirds; i++) {
        Bird b;
        b.radius       = radiusMin + rnd() * (radiusMax - radiusMin);
        // Angular speed: 0.10 .. 0.20 rad/s => ~30..60 s per orbit.
        b.angularSpeed = 0.10f + rnd() * 0.10f;
        b.phase        = rnd() * 6.2831853f;
        b.flapPhase    = rnd() * 6.2831853f;
        // Bird size: 1.0 .. 1.6 m wingspan.  Reads clearly at
        // altitude ~50 m.
        b.scale        = 1.0f + rnd() * 0.6f;
        birds.push_back(b);

        // Two parts per bird: left wing and right wing.  Each is
        // a thin box pointing along +X (the wing "out" direction);
        // the per-frame transform places it at the bird's position
        // and rotates it about the bird's heading + wing angle.
        parts.push_back(Part{ wingMesh, glm::mat4(1.0f) });
        parts.push_back(Part{ wingMesh, glm::mat4(1.0f) });
    }
}

void Birds::Update(float dt) {
    // Constants for the wing animation.
    //   - Wing length: half the bird's scale (so each wing sticks
    //     out scale/2 to either side of the body).
    //   - Flap speed:  2*PI / 0.6 s -> ~1.7 Hz, a slow but
    //     visible flap.
    //   - Flap amplitude: 25 degrees up/down about the wing root.
    const float kFlapHz     = 1.7f;
    const float kFlapAmpRad = 25.0f * 3.14159265f / 180.0f;

    size_t partIdx = 0;
    for (Bird& b : birds) {
        b.phase    += b.angularSpeed * dt;
        b.flapPhase += 2.0f * 3.14159265f * kFlapHz * dt;
        if (b.phase > 6.2831853f * 100.0f) {
            // Keep phase bounded so we don't lose precision over
            // long runs.
            b.phase = std::fmod(b.phase, 6.2831853f);
        }
        if (b.flapPhase > 6.2831853f * 100.0f) {
            b.flapPhase = std::fmod(b.flapPhase, 6.2831853f);
        }

        // Current position on the circle.
        float cx = centre.x + std::cos(b.phase) * b.radius;
        float cz = centre.z + std::sin(b.phase) * b.radius;
        float cy = centre.y + altitude;

        // Velocity direction (tangent to the circle, CCW seen
        // from +Y).  At phase phi the position is (cos, sin) and
        // the velocity is (-sin, +cos).  We want the bird's
        // forward to point along +velocity in world XZ.
        float vx = -std::sin(b.phase);
        float vz =  std::cos(b.phase);
        // Yaw such that the bird's local +X points along
        // (vx, vz).  atan2(z, x) gives the angle from +X.
        float yaw = std::atan2(vz, vx);

        // Wing flap: left wing rotates UP at peak of up-stroke,
        // right wing rotates DOWN.  We use sin(flapPhase) so the
        // wings beat symmetrically (opposite signs).
        float flap = std::sin(b.flapPhase) * kFlapAmpRad;

        // Wingspan / 2 = b.scale / 2 metres each side.  Each
        // wing is a 1x0.05x0.3 box (along X); we scale it to the
        // right total wingspan by setting the X scale = b.scale/2
        // and offset it to one side.
        float halfWing = b.scale;

        // Build the world transform for each wing:
        //   1. Translate to bird position (cx, cy, cz).
        //   2. Yaw to face along velocity.
        //   3. Rotate the wing about Z by `flap` (left wing) or
        //      -flap (right wing) so the wings beat.
        //   4. Offset sideways so each wing sticks out from the
        //      centre of the bird.
        //
        // Final matrices:
        //   M = T(cx,cy,cz) * R_y(yaw) *
        //       T(±halfWing, 0, 0) * R_z(±flap) * S(scaleX, 1, 1)
        //
        // Where scaleX is halfWing (so the box's local +X end
        // ends up at ±2*halfWing from the bird centre, i.e.
        // wingtips are full scale apart).  We also want a tiny Z
        // stretch for the chord; we keep the box's local chord as
        // 0.3 (chord) but multiplied by `scale`.

        // Left wing:
        glm::mat4 M = glm::translate(glm::mat4(1.0f),
                                    glm::vec3(cx, cy, cz))
                    * glm::rotate(glm::mat4(1.0f), yaw,
                                  glm::vec3(0.0f, 1.0f, 0.0f))
                    * glm::translate(glm::mat4(1.0f),
                                     glm::vec3(-halfWing, 0.0f, 0.0f))
                    * glm::rotate(glm::mat4(1.0f), flap,
                                  glm::vec3(0.0f, 0.0f, 1.0f))
                    * glm::scale(glm::mat4(1.0f),
                                 glm::vec3(halfWing, 1.0f, b.scale * 0.3f));
        parts[partIdx].local = M;
        partIdx++;

        // Right wing: mirror across the body axis (flap sign and
        // X offset flipped).
        glm::mat4 Mr = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(cx, cy, cz))
                     * glm::rotate(glm::mat4(1.0f), yaw,
                                   glm::vec3(0.0f, 1.0f, 0.0f))
                     * glm::translate(glm::mat4(1.0f),
                                      glm::vec3(halfWing, 0.0f, 0.0f))
                     * glm::rotate(glm::mat4(1.0f), -flap,
                                   glm::vec3(0.0f, 0.0f, 1.0f))
                     * glm::scale(glm::mat4(1.0f),
                                  glm::vec3(halfWing, 1.0f, b.scale * 0.3f));
        parts[partIdx].local = Mr;
        partIdx++;
    }
}

void Birds::Draw(Shader& shader) {
    DrawParts(shader, glm::mat4(1.0f), parts);
}

void Birds::Delete() {
    DeleteParts(parts);
    wingMesh.Delete();
}