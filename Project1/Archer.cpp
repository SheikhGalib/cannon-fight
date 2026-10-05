#include "Archer.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Archer::Archer(glm::vec3 baseWorld)
    : transform(glm::translate(glm::mat4(1.0f), baseWorld))
{
    // =========================================================================
    // Realistic Medieval Archer Figure
    // Proportioned with chainmail greaves, leather boots, archer's tunic,
    // leather bracer/vambrace, longbow, and back-strapped quiver with arrows.
    // =========================================================================

    const float legGap = 0.18f;

    // --- 1. Boots & Lower Legs (Greaves / Mail) -----------------------------
    for (float x : { -legGap, +legGap }) {
        // Leather boot
        parts.push_back({
            Primitives::CreateBox(0.18f, 0.16f, 0.28f, Palette::DarkIron),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.08f, 0.04f))
        });
        // Shin chainmail
        parts.push_back({
            Primitives::CreateCylinder(0.11f, 0.50f, 14, Palette::DarkIron, /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.18f, 0.0f))
        });
        // Thigh
        parts.push_back({
            Primitives::CreateCylinder(0.125f, 0.42f, 14, Palette::DarkIron, /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.68f, 0.0f))
        });
    }

    // --- 2. Body / Quilted Jerkin & Belt ------------------------------------
    // Tunic skirt
    parts.push_back({
        Primitives::CreateBox(0.50f, 0.26f, 0.34f, Palette::Defender),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.18f, 0.0f))
    });
    // Leather belt with brass buckle
    parts.push_back({
        Primitives::CreateBox(0.52f, 0.07f, 0.36f, Palette::Leather),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.315f, 0.0f))
    });
    parts.push_back({
        Primitives::CreateBox(0.09f, 0.08f, 0.04f, Palette::Brass),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.315f, 0.18f))
    });
    // Archer's leather jerkin (torso)
    parts.push_back({
        Primitives::CreateBox(0.48f, 0.42f, 0.32f, Palette::Defender),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.53f, 0.0f))
    });

    // --- 3. Pauldrons -------------------------------------------------------
    const float shoulderX = 0.29f;
    const float shoulderY = 1.66f;
    for (float x : { -shoulderX, +shoulderX }) {
        parts.push_back({
            Primitives::CreateBox(0.18f, 0.10f, 0.22f, Palette::Iron),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, shoulderY, 0.0f))
        });
    }

    // --- 4. Arms & Archer Vambraces -----------------------------------------
    const float armX = 0.30f;
    for (float x : { -armX, +armX }) {
        parts.push_back({
            Primitives::CreateCylinder(0.075f, 0.32f, 12, Palette::Defender, /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 1.38f, 0.0f))
        });
        // Archer's leather forearm bracer (protects from bowstring)
        parts.push_back({
            Primitives::CreateCylinder(0.07f, 0.30f, 12, Palette::Leather, /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 1.10f, 0.0f))
        });
        // Gauntlet / hand
        parts.push_back({
            Primitives::CreateBox(0.09f, 0.10f, 0.10f, Palette::Skin),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 1.04f, 0.0f))
        });
    }

    // --- 5. Head & Sallet / Archer Helmet -----------------------------------
    parts.push_back({
        Primitives::CreateBox(0.24f, 0.26f, 0.24f, Palette::Skin),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.89f, 0.0f))
    });
    // Helmet dome crown
    parts.push_back({
        Primitives::CreateCone(0.20f, 0.12f, 0.22f, 12, Palette::DarkIron, /*centered=*/false),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.98f, 0.0f))
    });
    // Flared brim / nose guard
    parts.push_back({
        Primitives::CreateBox(0.26f, 0.06f, 0.26f, Palette::DarkIron),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.96f, 0.01f))
    });
    parts.push_back({
        Primitives::CreateBox(0.04f, 0.09f, 0.07f, Palette::DarkIron),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.91f, 0.12f))
    });

    // --- 6. Longbow (held in left hand, ready stance) -----------------------
    const float bowX = -armX - 0.08f;
    const float bowY = 1.35f;
    glm::mat4 bowT = glm::translate(glm::mat4(1.0f), glm::vec3(bowX, bowY, 0.12f));
    bowT = glm::rotate(bowT, glm::radians(20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    parts.push_back({
        Primitives::CreateBox(0.04f, 1.30f, 0.10f, Palette::Wood),
        bowT
    });

    // --- 7. Quiver on Back with Arrows --------------------------------------
    glm::mat4 quiverT = glm::translate(glm::mat4(1.0f), glm::vec3(0.08f, 1.55f, -0.20f));
    quiverT = glm::rotate(quiverT, glm::radians(-15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // Quiver body
    parts.push_back({
        Primitives::CreateBox(0.14f, 0.55f, 0.14f, Palette::Leather),
        quiverT
    });
    // Arrow flights (feathers) protruding from quiver
    parts.push_back({
        Primitives::CreateBox(0.08f, 0.20f, 0.08f, glm::vec3(0.85f, 0.88f, 0.90f)),
        glm::translate(quiverT, glm::vec3(0.0f, 0.32f, 0.0f))
    });
}

void Archer::SetPosition(glm::vec3 baseWorld) {
    transform = glm::translate(glm::mat4(1.0f), baseWorld);
}

void Archer::Update(float dt) {
    if (shootAnim > 0.0f) {
        shootAnim = std::max(0.0f, shootAnim - dt);
    }
}

void Archer::Draw(Shader& shader) {
    // Base rotation +90 deg around Y: aligns model's forward (+Z) with world +X
    // When yawDegrees = 180, (180+90)=270 deg, archer faces world -X (towards the cannons)
    glm::mat4 m = glm::rotate(transform,
                              glm::radians(yawDegrees + 90.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f));

    if (shootAnim > 0.0f) {
        // Dynamic bow drawing and release animation: slight forward lean and bow raise
        float t = shootAnim / 0.6f;
        float pull = std::sin(t * 3.14159f);
        m = glm::rotate(m, glm::radians(pull * 12.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    DrawParts(shader, m, parts);
}

void Archer::Delete() {
    DeleteParts(parts);
}