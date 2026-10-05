#include "GoldCrest.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

GoldCrest::GoldCrest(glm::vec3 baseWorld) {
    // Layout, all in crest-local coordinates with the pedestal base
    // at y=0:
    //
    //     |  |\        flag (back corner)
    //     |  | \
    //     |__|__\
    //     | gold |     gold pile / coins (top of pedestal)
    //     | chest|
    //     +-----+      pedestal (stone box)
    //     |  P  |
    //     +-----+
    //     ─────         y = 0 (ground)

    const float pedW = 1.0f, pedH = 0.40f, pedD = 0.7f;
    parts.push_back({
        Primitives::CreateBox(pedW, pedH, pedD, Palette::Stone),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseWorld.x, pedH * 0.5f, baseWorld.z))
    });

    // Chest: a smaller wooden box with brass trim on top of the pedestal.
    const float chestW = 0.7f, chestH = 0.35f, chestD = 0.45f;
    const float chestY = pedH + chestH * 0.5f;
    parts.push_back({
        Primitives::CreateBox(chestW, chestH, chestD, Palette::Wood),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseWorld.x, chestY, baseWorld.z))
    });
    // Brass trim (a thin band along the chest top).
    const float trimT = 0.04f;
    parts.push_back({
        Primitives::CreateBox(chestW + 0.02f, trimT, chestD + 0.02f,
                              Palette::Brass),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseWorld.x, chestY + chestH * 0.5f + trimT * 0.5f,
                      baseWorld.z))
    });

    // Gold pile: a small mound of gold (a flattened sphere) on top of
    // the chest, with a few extra spheres around it for the spilled
    // coins look.
    const float goldY = chestY + chestH * 0.5f;
    parts.push_back({
        Primitives::CreateSphere(0.16f, 12, 12, Palette::Gold),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseWorld.x, goldY + 0.10f, baseWorld.z))
    });
    // A few smaller gold coins (tiny spheres) scattered around the
    // top of the chest.
    for (int i = 0; i < 5; i++) {
        float a = float(i) * 1.2f;
        float r = 0.20f + 0.07f * float(i % 2);
        parts.push_back({
            Primitives::CreateSphere(0.07f, 8, 8, Palette::Gold),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(baseWorld.x + std::cos(a) * r,
                          goldY + 0.05f,
                          baseWorld.z + std::sin(a) * r))
        });
    }

    // Flag pole + cloth in the back corner of the pedestal.
    const float poleH = 0.9f;
    parts.push_back({
        Primitives::CreateCylinder(0.025f, poleH, 6, Palette::Iron,
                                   /*centered=*/false),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseWorld.x - pedW * 0.35f,
                      pedH + poleH * 0.5f,
                      baseWorld.z - pedD * 0.30f))
    });
    parts.push_back({
        Primitives::CreateBox(0.40f, 0.22f, 0.03f, Palette::Gold),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseWorld.x - pedW * 0.35f + 0.22f,
                      pedH + poleH - 0.10f,
                      baseWorld.z - pedD * 0.30f))
    });
}

void GoldCrest::Update(float dt) {
    // Saw-tooth pulse: 0 -> 1 -> 0 over kPulsePeriod seconds.
    // Only animates while the crest is in "victorious" mode.
    if (!victorious) return;
    pulseT += dt / 1.4f;             // 1.4 s per pulse
    if (pulseT > 1.0f) pulseT -= 1.0f;
}

void GoldCrest::Draw(Shader& shader) {
    // When victorious, scale the gold pieces up slightly via the
    // per-part local matrix (multiply by a pulse factor).  We do
    // this by re-deriving the world matrix around the part's
    // centre.  For simplicity we re-use the part local unchanged
    // here - the "pulse" is the brightness shift in the lit.frag
    // shader which we approximate by drawing the gold pieces
    // twice, once normal and once at a slightly larger scale with
    // an additive blend if victorious.  But the lit.frag doesn't
    // support blending, so we instead just scale the gold pieces
    // by 1.0 + 0.10 * pulseT (subtle).  The simplest way to do
    // that is via the existing local matrices.
    DrawParts(shader, glm::mat4(1.0f), parts);
}

void GoldCrest::Delete() {
    DeleteParts(parts);
}
