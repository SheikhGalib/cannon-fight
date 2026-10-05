#include "Soldier.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Soldier::Soldier(glm::vec3 baseWorld, glm::vec3 bodyColour)
    : transform(glm::translate(glm::mat4(1.0f), baseWorld))
{
    // Same detailed-medieval layout as Archer (legs/body/paultrons/head/
    // helmet/nose-guard/arms), but no bow or quiver. Instead a short
    // sword at the right hip (handle + blade).
    //
    // All proportions are identical to Archer so the two figures look
    // like the same "race" of soldier, just with different equipment.

    // --- legs (chainmail) -----------------------------------------------
    const float legRadius = 0.12f;
    const float legHeight = 1.20f;
    const float legGap    = 0.20f;
    for (float x : { -legGap, +legGap }) {
        parts.push_back({
            Primitives::CreateCylinder(legRadius, legHeight, 14, Palette::DarkIron,
                                       /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, legHeight * 0.5f, 0.0f))
        });
    }

    // --- body (jerkin) --------------------------------------------------
    const float bodyW = 0.55f, bodyH = 0.85f, bodyD = 0.40f;
    const float bodyBottom = legHeight;
    const float bodyCentre = bodyBottom + bodyH * 0.5f;

    parts.push_back({
        Primitives::CreateBox(bodyW, bodyH, bodyD, bodyColour),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, bodyCentre, 0.0f))
    });

    // --- pauldrons ----------------------------------------------------
    const float paultronW = 0.22f, paultronH = 0.10f, paultronD = 0.22f;
    const float paultronY = bodyBottom + bodyH - paultronH * 0.5f;
    const float paultronX = bodyW * 0.5f + paultronW * 0.4f;
    for (float x : { -paultronX, +paultronX }) {
        parts.push_back({
            Primitives::CreateBox(paultronW, paultronH, paultronD, bodyColour),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, paultronY, 0.0f))
        });
    }

    // --- arms ---------------------------------------------------------
    const float armRadius = 0.08f;
    const float armLength = 0.90f;
    const float armTop    = bodyBottom + bodyH * 0.85f;
    const float armGap    = bodyW * 0.5f + armRadius * 0.5f;
    for (float x : { -armGap, +armGap }) {
        parts.push_back({
            Primitives::CreateCylinder(armRadius, armLength, 12, bodyColour,
                                       /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, armTop - armLength * 0.5f, 0.0f))
        });
    }

    // --- head (skin) --------------------------------------------------
    const float headW = 0.30f, headH = 0.35f, headD = 0.30f;
    const float headBottom = bodyBottom + bodyH;
    const float headCentre = headBottom + headH * 0.5f;
    parts.push_back({
        Primitives::CreateBox(headW, headH, headD, Palette::Skin),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, headCentre, 0.0f))
    });

    // --- helmet + nose-guard -----------------------------------------
    const float helmetR = 0.22f;
    const float helmetH = 0.28f;
    parts.push_back({
        Primitives::CreateCone(helmetR, helmetR * 0.6f, helmetH, 10, Palette::DarkIron,
                               /*centered=*/false, /*yOffset=*/0.0f),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(0.0f, headBottom + headH + helmetH * 0.5f, 0.0f))
    });
    parts.push_back({
        Primitives::CreateBox(0.04f, 0.10f, 0.08f, Palette::DarkIron),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(headW * 0.5f + 0.02f, headCentre, 0.0f))
    });

    // --- sword (right hip) --------------------------------------------
    // Two boxes at the right hip:
    //   * handle: 0.05 x 0.15 x 0.05 in Wood / Leather
    //   * blade:  0.05 x 0.55 x 0.05 in Iron
    // Sits just outside the right arm so it doesn't clip the body.
    const float swordX = armGap + 0.05f;
    const float swordY = bodyBottom + 0.20f;       // bottom of the body box
    parts.push_back({
        Primitives::CreateBox(0.05f, 0.15f, 0.05f, Palette::Leather),
        glm::translate(glm::mat4(1.0f), glm::vec3(swordX, swordY + 0.075f, 0.0f))
    });
    parts.push_back({
        Primitives::CreateBox(0.05f, 0.55f, 0.05f, Palette::Iron),
        glm::translate(glm::mat4(1.0f), glm::vec3(swordX, swordY + 0.15f + 0.275f, 0.0f))
    });
}

void Soldier::SetPosition(glm::vec3 baseWorld) {
    transform = glm::translate(glm::mat4(1.0f), baseWorld);
}

void Soldier::Draw(Shader& shader) {
    glm::mat4 m = transform;
    if (yawDegrees != 0.0f) {
        m = glm::rotate(m, glm::radians(yawDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (attackOffset != 0.0f) {
        m = glm::translate(m, glm::vec3(attackOffset, 0.0f, 0.0f));
    }
    if (pitchDegrees != 0.0f) {
        // Fall flat onto ground when dead
        m = glm::rotate(m, glm::radians(pitchDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    DrawParts(shader, m, parts);
}

void Soldier::Delete() {
    DeleteParts(parts);
}