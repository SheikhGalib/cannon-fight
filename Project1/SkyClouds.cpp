#include "SkyClouds.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

SkyClouds::SkyClouds(int numClouds, float xSpan, float zSpan,
                     float skyLow, float skyHigh)
    : xEnd(xSpan),
      // One low-res sphere shared across every puff.  10 stacks x
      // 16 slices is plenty at sky distances.
      puffMesh(Primitives::CreateSphere(1.0f, 10, 16,
                                        Palette::Cloud)) {

    // Deterministic LCG-style PRNG so the sky cloud layout is
    // identical between runs (handy for screenshots / docs).
    unsigned int seed = 72611u;
    auto rnd = [&]() {
        seed = seed * 1103515245u + 12345u;
        return float((seed >> 8) & 0xFFFFFFu) / float(0xFFFFFFu);
    };

    const float kTwoPi = 6.2831853f;
    clouds.reserve(numClouds);
    puffs.reserve(numClouds);
    parts.reserve(numClouds * 8);

    for (int i = 0; i < numClouds; i++) {
        Cloud c;
        c.centre = glm::vec3(
            (rnd() * 2.0f - 1.0f) * xSpan,
            skyLow + rnd() * (skyHigh - skyLow),
            (rnd() * 2.0f - 1.0f) * zSpan);
        // Drift speed: 1.2 .. 2.5 m/s.  Slow enough that the sky
        // feels alive but the user can tell a given cloud is
        // moving when they watch.
        c.speed = 1.2f + rnd() * 1.3f;
        // Per-cloud overall size: 6 .. 12 m radius.  Mix big and
        // small so the sky has visual variety.
        float baseScale = 6.0f + rnd() * 6.0f;
        clouds.push_back(c);

        // Build the cloud as a cluster of overlapping puffs:
        //   - a main "body" puff at the centre
        //   - 4-6 satellite puffs spread around it
        // Each puff is stored as a relative offset from c.centre
        // (so we can rebuild its transform each frame when the
        // cloud drifts).
        std::vector<Puff> myPuffs;

        // Body puff (slightly squashed in Y for a more
        // cloud-like profile).
        {
            Puff p;
            p.relative = glm::vec3(0.0f);
            p.scale    = baseScale;
            myPuffs.push_back(p);
            parts.push_back(Part{ puffMesh, glm::mat4(1.0f) });
        }

        int satellites = 4 + (int)(rnd() * 3.0f);   // 4..6
        for (int s = 0; s < satellites; s++) {
            float ang = (float(s) + rnd() * 0.4f) * kTwoPi / float(satellites);
            float r   = baseScale * (0.55f + rnd() * 0.55f);
            Puff p;
            p.relative = glm::vec3(std::cos(ang) * r,
                                    (rnd() - 0.5f) * baseScale * 0.20f,
                                    std::sin(ang) * r);
            p.scale = baseScale * (0.45f + rnd() * 0.30f);
            myPuffs.push_back(p);
            parts.push_back(Part{ puffMesh, glm::mat4(1.0f) });
        }
        puffs.push_back(myPuffs);
    }

    // Build the initial transforms so the first frame is correct
    // without having to wait one Update() tick.
    const float dummyDt = 0.0f;
    (void)dummyDt;
    // Inline rebuild:
    size_t partIdx = 0;
    for (size_t i = 0; i < clouds.size(); i++) {
        Cloud& c = clouds[i];
        for (const Puff& p : puffs[i]) {
            glm::vec3 absPos = c.centre + p.relative;
            parts[partIdx].local =
                glm::translate(glm::mat4(1.0f), absPos)
                * glm::scale(glm::mat4(1.0f),
                             glm::vec3(p.scale,
                                       p.scale,
                                       p.scale));
            partIdx++;
        }
    }
}

void SkyClouds::Update(float dt) {
    // 1. Drift each cloud along +X and wrap.
    for (Cloud& c : clouds) {
        c.centre.x += c.speed * dt;
        if (c.centre.x > xEnd) {
            // Wrap by a multiple of 2*xEnd so the cloud never
            // "teleports into view" off-camera - subtract enough
            // to land somewhere we can see drift continuing.
            c.centre.x -= 2.0f * xEnd;
        }
    }
    // 2. Rebuild each cloud's puffs' transforms from the (now
    // updated) cloud centres.
    size_t partIdx = 0;
    for (size_t i = 0; i < clouds.size(); i++) {
        const Cloud& c = clouds[i];
        for (const Puff& p : puffs[i]) {
            glm::vec3 absPos = c.centre + p.relative;
            parts[partIdx].local =
                glm::translate(glm::mat4(1.0f), absPos)
                * glm::scale(glm::mat4(1.0f),
                             glm::vec3(p.scale,
                                       p.scale,
                                       p.scale));
            partIdx++;
        }
    }
}

void SkyClouds::Draw(Shader& shader) {
    DrawParts(shader, glm::mat4(1.0f), parts);
}

void SkyClouds::Delete() {
    DeleteParts(parts);
    puffMesh.Delete();
}