#include "Archer.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Archer::Archer(glm::vec3 baseWorld)
    : transform(glm::translate(glm::mat4(1.0f), baseWorld))
{
    // Layout, all in archer-local coordinates with the feet at y = 0:
    //
    //     +---+
    //     | H |    head:        box 0.30 x 0.35 x 0.30, Skin colour
    //     +---+
    //     |   |    helmet cone:  r=0.22 h=0.28, DarkIron
    //     +---+   body:         box 0.55 x 0.85 x 0.40, Iron
    //     |   |
    //     | B |
    //     |   |
    //     +---+   shoulders at y = legTop = 1.20
    //     |   |   pauldrons:    two 0.22 x 0.10 x 0.22 Iron boxes
    //     |   |
    //     | L |   legs:         two cylinders r=0.12 h=1.20, DarkIron
    //     |   |                 spaced 0.20 on X (chainmail leggings)
    //     |   |
    //     | A |   arms:         two cylinders r=0.08 h=0.90, Iron
    //     |   |
    //     ────     y = 0 (feet)
    //
    // Plus a longbow held in the right hand and a quiver strapped to the
    // back, so the figure reads as an archer rather than as a Robot.
    //
    // All sizes are constants right here on purpose — there are several
    // archers in the scene (one per tower) but the proportions are
    // always identical, so spreading them out into Dimensions.h would
    // only obscure the layout.

    // --- legs (chainmail) -----------------------------------------------
    const float legRadius = 0.12f;
    const float legHeight = 1.20f;
    const float legGap    = 0.20f;          // half-distance between legs on X

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
        Primitives::CreateBox(bodyW, bodyH, bodyD, Palette::Iron),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, bodyCentre, 0.0f))
    });

    // --- pauldrons (shoulder armour) --------------------------------------
    const float paultronW = 0.22f, paultronH = 0.10f, paultronD = 0.22f;
    const float paultronY = bodyBottom + bodyH - paultronH * 0.5f;
    const float paultronX = bodyW * 0.5f + paultronW * 0.4f;
    for (float x : { -paultronX, +paultronX }) {
        parts.push_back({
            Primitives::CreateBox(paultronW, paultronH, paultronD, Palette::Iron),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, paultronY, 0.0f))
        });
    }

    // --- arms -----------------------------------------------------------
    const float armRadius = 0.08f;
    const float armLength = 0.90f;
    const float armTop    = bodyBottom + bodyH * 0.85f;
    const float armGap    = bodyW * 0.5f + armRadius * 0.5f;
    for (float x : { -armGap, +armGap }) {
        parts.push_back({
            Primitives::CreateCylinder(armRadius, armLength, 12, Palette::Iron,
                                       /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, armTop - armLength * 0.5f, 0.0f))
        });
    }

    // --- head (skin) ----------------------------------------------------
    const float headW = 0.30f, headH = 0.35f, headD = 0.30f;
    const float headBottom = bodyBottom + bodyH;
    const float headCentre = headBottom + headH * 0.5f;
    parts.push_back({
        Primitives::CreateBox(headW, headH, headD, Palette::Skin),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, headCentre, 0.0f))
    });

    // --- helmet (cone) + nose-guard -------------------------------------
    const float helmetR = 0.22f;
    const float helmetH = 0.28f;
    parts.push_back({
        Primitives::CreateCone(helmetR, helmetR * 0.6f, helmetH, 10, Palette::DarkIron,
                               /*centered=*/false, /*yOffset=*/0.0f),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(0.0f, headBottom + headH + helmetH * 0.5f, 0.0f))
    });
    // Nose-guard: small box on the +X face of the helmet.
    parts.push_back({
        Primitives::CreateBox(0.04f, 0.10f, 0.08f, Palette::DarkIron),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(headW * 0.5f + 0.02f, headCentre, 0.0f))
    });

    // --- longbow (held in the right hand, tilted slightly) -------------
    // Box dims: 0.04 wide (across the bow), 1.20 tall (length of bow),
    // 0.15 deep (across the bow's flat face).  Tilted ~25° around Z
    // so it reads as "drawn back" rather than "standing straight up".
    const float bowW = 0.04f, bowH = 1.20f, bowD = 0.15f;
    const float bowX = armGap + 0.10f;          // just outside the right arm
    const float bowY = armTop - armLength * 0.5f;
    glm::mat4 bowT = glm::translate(glm::mat4(1.0f), glm::vec3(bowX, bowY, 0.0f));
    bowT = glm::rotate(bowT, glm::radians(25.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    parts.push_back({
        Primitives::CreateBox(bowW, bowH, bowD, Palette::Wood),
        bowT
    });

    // --- quiver (strapped to the back, on -X side) ---------------------
    const float quiverW = 0.10f, quiverH = 0.40f, quiverD = 0.10f;
    parts.push_back({
        Primitives::CreateBox(quiverW, quiverH, quiverD, Palette::WoodLight),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(-bodyW * 0.5f + quiverW * 0.5f - 0.04f,
                                 bodyCentre, 0.0f))
    });
}

void Archer::SetPosition(glm::vec3 baseWorld) {
    transform = glm::translate(glm::mat4(1.0f), baseWorld);
}

void Archer::Draw(Shader& shader) {
    // Phase 7: apply the optional Y rotation (e.g. to face the camera) on
    // top of the base position transform.
    glm::mat4 m = (yawDegrees == 0.0f) ? transform
                                        : glm::rotate(transform,
                                                                    glm::radians(yawDegrees),
                                                                    glm::vec3(0.0f, 1.0f, 0.0f));
    DrawParts(shader, m, parts);
}

void Archer::Delete() {
    DeleteParts(parts);
}