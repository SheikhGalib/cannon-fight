#include "Soldier.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <cmath>

Soldier::Soldier(glm::vec3 baseWorld, glm::vec3 bodyColour)
    : transform(glm::translate(glm::mat4(1.0f), baseWorld))
{
    // =========================================================================
    // Realistic Anatomically-Proportioned Medieval Armored Soldier (~1.82 m)
    // 7.5 heads ratio, articulated limbs, steel breastplate, pauldrons,
    // sallet helmet, leather belt with brass buckle, arming sword, and shield.
    // =========================================================================

    const float legGap = 0.18f;

    // --- 1. Feet & Leather Boots (y = 0.00 to 0.22) --------------------------
    for (float x : { -legGap, +legGap }) {
        // Boot foot block with slight forward toe extension
        parts.push_back({
            Primitives::CreateBox(0.18f, 0.16f, 0.28f, Palette::DarkIron),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.08f, 0.04f))
        });
    }

    // --- 2. Lower Legs: Steel Greaves over Chainmail (y = 0.18 to 0.68) ------
    for (float x : { -legGap, +legGap }) {
        parts.push_back({
            Primitives::CreateCylinder(0.11f, 0.50f, 14, Palette::Iron, /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.18f, 0.0f))
        });
        // Knee poleyn (steel knee cop)
        parts.push_back({
            Primitives::CreateSphere(0.13f, 10, 10, Palette::Iron),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.68f, 0.03f))
        });
    }

    // --- 3. Upper Legs: Chausses / Quilted Thighs (y = 0.68 to 1.10) --------
    for (float x : { -legGap, +legGap }) {
        parts.push_back({
            Primitives::CreateCylinder(0.125f, 0.42f, 14, Palette::DarkIron, /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.68f, 0.0f))
        });
    }

    // --- 4. Pelvis & Tunic Faulds (y = 1.05 to 1.30) -------------------------
    // Flared quilted tunic skirts in uniform color
    parts.push_back({
        Primitives::CreateBox(0.52f, 0.26f, 0.36f, bodyColour),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.18f, 0.0f))
    });

    // --- 5. Leather Belt & Brass Buckle (y = 1.28 to 1.35) -------------------
    parts.push_back({
        Primitives::CreateBox(0.54f, 0.07f, 0.38f, Palette::Leather),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.315f, 0.0f))
    });
    // Golden brass buckle at front
    parts.push_back({
        Primitives::CreateBox(0.10f, 0.08f, 0.04f, Palette::Brass),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.315f, 0.20f))
    });

    // --- 6. Torso: Heraldic Surcoat / Tabard over Breastplate (y = 1.32 to 1.76) ---
    // Vibrant uniform tabard / surcoat
    parts.push_back({
        Primitives::CreateBox(0.50f, 0.44f, 0.36f, bodyColour),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.53f, 0.0f))
    });
    // Reinforced steel breastplate center with heraldic crest
    parts.push_back({
        Primitives::CreateBox(0.26f, 0.30f, 0.38f, Palette::Iron),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.55f, 0.0f))
    });
    parts.push_back({
        Primitives::CreateBox(0.12f, 0.12f, 0.39f, Palette::Brass),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.55f, 0.0f))
    });

    // --- 7. Neck: Chainmail Coif / Gorget (y = 1.70 to 1.78) -----------------
    parts.push_back({
        Primitives::CreateCylinder(0.14f, 0.10f, 12, Palette::DarkIron, /*centered=*/false),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.72f, 0.0f))
    });

    // --- 8. Head & Face (y = 1.76 to 2.02) -----------------------------------
    parts.push_back({
        Primitives::CreateBox(0.24f, 0.26f, 0.24f, Palette::Skin),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.89f, 0.0f))
    });

    // --- 9. Medieval Sallet Helmet with Heraldic Plume ----------------------
    // Helmet dome crown
    parts.push_back({
        Primitives::CreateCone(0.20f, 0.12f, 0.22f, 12, Palette::Iron, /*centered=*/false),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.98f, 0.0f))
    });
    // Heraldic crest plume atop helmet
    parts.push_back({
        Primitives::CreateBox(0.07f, 0.14f, 0.24f, bodyColour),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.10f, -0.02f))
    });
    // Helmet brow ridge / flared brim
    parts.push_back({
        Primitives::CreateBox(0.28f, 0.07f, 0.28f, Palette::Iron),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.96f, 0.01f))
    });
    // Visor dark eye-slit / nasal guard
    parts.push_back({
        Primitives::CreateBox(0.18f, 0.035f, 0.06f, Palette::DarkIron),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.91f, 0.12f))
    });

    // --- 10. Articulated Pauldrons (Shoulder Guards with Brass Trim) ---------
    const float shoulderX = 0.31f;
    const float shoulderY = 1.66f;
    for (float x : { -shoulderX, +shoulderX }) {
        // Steel pauldron plate
        parts.push_back({
            Primitives::CreateBox(0.20f, 0.12f, 0.24f, Palette::Iron),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, shoulderY, 0.0f))
        });
        // Brass border rim
        parts.push_back({
            Primitives::CreateBox(0.21f, 0.03f, 0.25f, Palette::Brass),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, shoulderY - 0.04f, 0.0f))
        });
    }

    // --- 11. Arms: Sleeves, Steel Vambraces & Gauntlets ----------------------
    const float armX = 0.32f;
    for (float x : { -armX, +armX }) {
        // Upper arm (sleeved tunic)
        parts.push_back({
            Primitives::CreateCylinder(0.08f, 0.32f, 12, bodyColour, /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 1.38f, 0.0f))
        });
        // Forearm (steel vambrace armor)
        parts.push_back({
            Primitives::CreateCylinder(0.075f, 0.30f, 12, Palette::Iron, /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 1.10f, 0.0f))
        });
        // Gauntlet (hand)
        parts.push_back({
            Primitives::CreateBox(0.10f, 0.11f, 0.11f, Palette::DarkIron),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, 1.04f, 0.0f))
        });
    }

    // --- 12. Heraldic Heater Shield on Left Arm ------------------------------
    const float shieldX = -armX - 0.10f;
    const float shieldY = 1.34f;
    // Wooden shield face in uniform heraldic color
    shieldParts.push_back({
        Primitives::CreateBox(0.05f, 0.65f, 0.44f, bodyColour),
        glm::translate(glm::mat4(1.0f), glm::vec3(shieldX, shieldY, 0.08f))
    });
    // Dark iron rim border
    shieldParts.push_back({
        Primitives::CreateBox(0.06f, 0.68f, 0.46f, Palette::DarkIron),
        glm::translate(glm::mat4(1.0f), glm::vec3(shieldX - 0.005f, shieldY, 0.08f))
    });
    // Golden central shield boss (reinforcing dome)
    shieldParts.push_back({
        Primitives::CreateSphere(0.11f, 10, 10, Palette::Brass),
        glm::translate(glm::mat4(1.0f), glm::vec3(shieldX - 0.035f, shieldY, 0.08f))
    });

    // --- 13. Detailed Steel Arming Sword at Right Side -----------------------
    const float swordX = armX + 0.10f;
    const float swordY = 1.25f;
    // Tapered steel blade
    parts.push_back({
        Primitives::CreateBox(0.03f, 0.70f, 0.07f, Palette::Iron),
        glm::translate(glm::mat4(1.0f), glm::vec3(swordX, swordY - 0.20f, 0.0f))
    });
    // Crossguard bar
    parts.push_back({
        Primitives::CreateBox(0.05f, 0.04f, 0.24f, Palette::Iron),
        glm::translate(glm::mat4(1.0f), glm::vec3(swordX, swordY + 0.16f, 0.0f))
    });
    // Leather grip handle
    parts.push_back({
        Primitives::CreateCylinder(0.025f, 0.14f, 8, Palette::Leather, /*centered=*/false),
        glm::translate(glm::mat4(1.0f), glm::vec3(swordX, swordY + 0.18f, 0.0f))
    });
    // Golden brass pommel button
    parts.push_back({
        Primitives::CreateSphere(0.045f, 8, 8, Palette::Brass),
        glm::translate(glm::mat4(1.0f), glm::vec3(swordX, swordY + 0.33f, 0.0f))
    });
}

void Soldier::SetPosition(glm::vec3 baseWorld) {
    transform = glm::translate(glm::mat4(1.0f), baseWorld);
}

void Soldier::Update(float dt) {
    float targetShield = shieldRaised ? 1.0f : 0.0f;
    if (shieldRaiseAmount < targetShield) {
        shieldRaiseAmount = std::min(targetShield, shieldRaiseAmount + dt * 3.8f);
    } else if (shieldRaiseAmount > targetShield) {
        shieldRaiseAmount = std::max(targetShield, shieldRaiseAmount - dt * 3.8f);
    }

    if (isMarching) {
        marchPhase += dt * 8.0f;
    }
}

void Soldier::Draw(Shader& shader) {
    glm::mat4 m = transform;

    // Base rotation +90 deg around Y: aligns model's forward (+Z) with world +X (where cannons point)
    m = glm::rotate(m, glm::radians(yawDegrees + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    if (attackOffset != 0.0f) {
        // Forward lunge along soldier's facing direction (local +Z)
        m = glm::translate(m, glm::vec3(0.0f, 0.0f, attackOffset));
    }
    if (pitchDegrees != 0.0f) {
        // Casualties fall backwards flat to the ground (pitch = -90 deg)
        m = glm::rotate(m, glm::radians(pitchDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    if (isMarching && pitchDegrees == 0.0f) {
        // Subtle marching step bob
        m = glm::translate(m, glm::vec3(0.0f, std::abs(std::sin(marchPhase)) * 0.04f, 0.0f));
    }

    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    for (Part& p : parts) {
        glm::mat4 partM = m * p.local;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(partM));
        p.mesh.Draw();
    }

    // Dynamic shield raising: when guarding against arrows, shield lifts in front of chest/head
    glm::mat4 shieldAnim = glm::mat4(1.0f);
    if (shieldRaiseAmount > 0.001f) {
        shieldAnim = glm::translate(shieldAnim, glm::vec3(0.24f * shieldRaiseAmount,
                                                          0.22f * shieldRaiseAmount,
                                                          0.28f * shieldRaiseAmount));
        shieldAnim = glm::rotate(shieldAnim, glm::radians(25.0f * shieldRaiseAmount), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    for (Part& p : shieldParts) {
        glm::mat4 partM = m * shieldAnim * p.local;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(partM));
        p.mesh.Draw();
    }
}

void Soldier::Delete() {
    for (Part& p : parts) p.mesh.Delete();
    for (Part& p : shieldParts) p.mesh.Delete();
}