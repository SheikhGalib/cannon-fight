#include "Scenery.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Scenery::Scenery(glm::vec3 centreWorld, float innerRadius, float outerRadius,
                 int count) {
    // Distribute `count` mountains around the centre on a noisy ring:
    // the angle is uniform in [0, 2pi) and the radius is jittered
    // between innerRadius and outerRadius.  Each one is a tall cone
    // (with a smaller white "snow cap" cone on top) for the mountains
    // and a wider shorter cone for the valley hills, mixed in a 1:1
    // ratio so the horizon line is varied.
    const float kTwoPi = 6.2831853f;
    // Simple LCG-style PRNG so the layout is deterministic between
    // runs (handy for screenshots / docs).
    unsigned int seed = 91823u;
    auto rnd = [&]() {
        seed = seed * 1103515245u + 12345u;
        return float((seed >> 8) & 0xFFFFFFu) / float(0xFFFFFFu);
    };
    for (int i = 0; i < count; i++) {
        float angle  = (float(i) + rnd() * 0.6f) * kTwoPi / float(count);
        float radius = innerRadius + rnd() * (outerRadius - innerRadius);
        float x = centreWorld.x + std::cos(angle) * radius;
        float z = centreWorld.z + std::sin(angle) * radius;
        // 50/50 mountain vs hill.
        bool isMountain = (i % 2) == 0;
        float height, baseR;
        glm::vec3 bodyColour;
        if (isMountain) {
            height   = 8.0f + rnd() * 7.0f;       // 8..15 m
            baseR    = 4.0f + rnd() * 3.0f;       // 4..7 m
            bodyColour = Palette::Mountain;
        } else {
            height   = 2.5f + rnd() * 2.5f;       // 2.5..5 m
            baseR    = 6.0f + rnd() * 4.0f;       // 6..10 m
            bodyColour = Palette::Valley;
        }
        // Body cone.
        parts.push_back({
            Primitives::CreateCone(baseR, 0.0f, height, 16, bodyColour,
                                   /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.0f, z))
        });
        // Snow cap (only for mountains): a small white cone on the
        // top quarter of the body.  Top of body is at y = height;
        // cap starts at height * 0.65 and is height * 0.35 tall.
        if (isMountain) {
            float capH = height * 0.35f;
            float capBaseY = height - capH;
            float capBaseR = baseR * 0.30f;
            parts.push_back({
                Primitives::CreateCone(capBaseR, 0.0f, capH, 12,
                                       Palette::SnowCap, /*centered=*/false),
                glm::translate(glm::mat4(1.0f), glm::vec3(x, capBaseY, z))
            });
        }
    }
}

void Scenery::Draw(Shader& shader) {
    DrawParts(shader, glm::mat4(1.0f), parts);
}

void Scenery::Delete() {
    DeleteParts(parts);
}
