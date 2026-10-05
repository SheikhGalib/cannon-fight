#include "Scenery.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Scenery::Scenery(glm::vec3 centreWorld, float innerRadius, float outerRadius,
                 int count) {
    // Phase 9: mountains are now BIG (20..35 m) and biased toward the
    // SIDES and BACK of the scene (not a uniform noisy ring).  The
    // river / cannon / castle occupy the +X to -X axis, so we want the
    // mountains to cluster on the +Z and -Z flanks and far behind the
    // castle (+X) where the gold crest and the back of the compound
    // sit.  The +X and -X axis stays relatively open so the river and
    // the fight zone are visible.
    //
    // We do this by computing an angle in [0, 2pi), and then
    // remapping the angle through a power curve that pushes density
    // toward 90° (+Z) and 270° (-Z).  The +X axis is also weighted
    // slightly (the back of the castle), but the -X (cannon side) is
    // the least weighted.
    const float kTwoPi = 6.2831853f;
    // Simple LCG-style PRNG so the layout is deterministic between
    // runs (handy for screenshots / docs).
    unsigned int seed = 91823u;
    auto rnd = [&]() {
        seed = seed * 1103515245u + 12345u;
        return float((seed >> 8) & 0xFFFFFFu) / float(0xFFFFFFu);
    };
    for (int i = 0; i < count; i++) {
        float baseAngle = (float(i) + rnd() * 0.6f) * kTwoPi / float(count);
        // Remap angle: weight by sin^2 so we get two clusters at
        // 90° and 270° (i.e. +Z and -Z).  Multiply by 2 to keep
        // the final distribution in [0, 2pi).
        float weighted = std::sin(baseAngle);
        weighted = (weighted * weighted) * (weighted >= 0.0f ? 1.0f : -1.0f);
        // Blend the weighted angle with the original so it's not
        // entirely clustered.
        float angle = baseAngle * 0.35f + weighted * kTwoPi * 0.5f * 0.65f;
        float radius = innerRadius + rnd() * (outerRadius - innerRadius);
        float x = centreWorld.x + std::cos(angle) * radius;
        float z = centreWorld.z + std::sin(angle) * radius;
        // 50/50 mountain vs hill.  Phase 9: mountains are much
        // bigger (20..35 m) and wider (10..18 m) so they read as
        // real mountains; hills are kept small (2.5..5 m) and
        // closer-in to soften the foreground.
        bool isMountain = (i % 2) == 0;
        float height, baseR;
        glm::vec3 bodyColour;
        if (isMountain) {
            height   = 20.0f + rnd() * 15.0f;      // 20..35 m
            baseR    = 10.0f + rnd() * 8.0f;       // 10..18 m
            bodyColour = Palette::Mountain;
        } else {
            height   = 2.5f + rnd() * 2.5f;        // 2.5..5 m
            baseR    = 6.0f + rnd() * 4.0f;        // 6..10 m
            bodyColour = Palette::Valley;
        }
        // Body cone.
        parts.push_back({
            Primitives::CreateCone(baseR, 0.0f, height, 16, bodyColour,
                                   /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.0f, z))
        });
        // Cloud ring around each mountain peak.  Instead of a solid
        // cone "hat" (which read as a UFO perched on the mountain),
        // we scatter a ring of 7-9 overlapping spheres around the
        // upper-mid portion of the mountain.  Each puff has a
        // random radius (so the ring looks natural), and the ring
        // sits at ~75 % of the mountain's height (a typical cloud
        // "hat" position around a peak).  Two or three smaller
        // extra puffs are sprinkled inside the ring at varying
        // heights to break up the silhouette.
        //
        // All spheres share one Mesh - rebuilt for each puff since
        // we want a slightly varied alpha-look, but a single sphere
        // mesh is fine because they all use the same radius.  We
        // accept some vertex waste for visual variety.
        if (isMountain) {
            // Cache one sphere mesh so we don't allocate per-puff.
            // Use a moderate resolution (10 stacks x 12 slices) so
            // the cloud puffs still look round from camera distance.
            static Mesh cloudPuff = Primitives::CreateSphere(
                1.0f, 10, 12, Palette::Cloud);
            // The cloud ring sits at height * 0.72 with a small
            // jitter so neighbouring mountains don't all have
            // clouds at exactly the same altitude.
            float ringY = height * (0.68f + rnd() * 0.10f);
            // Ring radius: a bit wider than the mountain at that
            // height so the cloud ring sticks out past the slope.
            // (At ringY the cone's radius = baseR * (1 - ringY/height).)
            float mountainRAtRingY = baseR * (1.0f - ringY / height);
            float ringR = mountainRAtRingY + baseR * 0.18f;
            int   puffs = 7 + (int)(rnd() * 3.0f);   // 7..9 puffs
            for (int p = 0; p < puffs; p++) {
                float ang = (float(p) + rnd() * 0.4f) * kTwoPi / float(puffs);
                // Slight per-puff radius jitter for a natural look.
                float puffR = (baseR * 0.22f) * (0.85f + rnd() * 0.30f);
                float px = x + std::cos(ang) * ringR;
                float pz = z + std::sin(ang) * ringR;
                // Slight vertical jitter so the ring isn't flat.
                float py = ringY + (rnd() - 0.5f) * baseR * 0.08f;
                parts.push_back(Part{
                    cloudPuff,
                    glm::translate(glm::mat4(1.0f), glm::vec3(px, py, pz))
                        * glm::scale(glm::mat4(1.0f),
                                     glm::vec3(puffR, puffR, puffR))
                });
            }
            // Two or three smaller "extra" puffs at the very top
            // for variation.
            int extrasN = 2 + (int)(rnd() * 2.0f);
            for (int p = 0; p < extrasN; p++) {
                float ang = rnd() * kTwoPi;
                float r   = mountainRAtRingY * 0.35f + rnd() * mountainRAtRingY * 0.4f;
                float px  = x + std::cos(ang) * r;
                float pz  = z + std::sin(ang) * r;
                float py  = ringY + baseR * 0.08f + rnd() * baseR * 0.10f;
                float puffR = baseR * 0.14f * (0.7f + rnd() * 0.4f);
                parts.push_back(Part{
                    cloudPuff,
                    glm::translate(glm::mat4(1.0f), glm::vec3(px, py, pz))
                        * glm::scale(glm::mat4(1.0f),
                                     glm::vec3(puffR, puffR, puffR))
                });
            }
        }
    }
}

void Scenery::Draw(Shader& shader) {
    DrawParts(shader, glm::mat4(1.0f), parts);
}

void Scenery::Delete() {
    DeleteParts(parts);
}
