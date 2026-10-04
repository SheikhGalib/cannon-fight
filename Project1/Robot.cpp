#include "Robot.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Robot::Robot(glm::vec3 baseWorld)
    : transform(glm::translate(glm::mat4(1.0f), baseWorld))
{
    // Layout, all in robot-local coordinates with the feet at y = 0:
    //
    //     +---+
    //     | H |    head: box 0.30 wide x 0.35 tall x 0.30 deep
    //     +---+
    //     |   |    shoulders at y = legTop = 1.20
    //     +---+   body: box 0.55 wide x 0.85 tall x 0.40 deep
    //     |   |
    //     | B |
    //     +---+
    //     |   |    legs: cylinders 0.10 r x 1.20 tall, spaced 0.20 on X
    //     |   |
    //     | L |
    //     |   |
    //     |   |
    //     ────     y = 0 (feet)
    //
    // The arms hang at the sides of the body (one slightly forward, one
    // slightly back, on the X axis).
    //
    // All sizes are constants right here on purpose — there's only one
    // Robot in the scene so there's no value in spreading them out.

    const float legRadius = 0.10f;
    const float legHeight = 1.20f;
    const float legGap = 0.20f;       // half-distance between the two legs on X

    const float bodyW = 0.55f, bodyH = 0.85f, bodyD = 0.40f;
    const float bodyBottom = legHeight;             // body sits on top of legs
    const float bodyCentre = bodyBottom + bodyH * 0.5f;

    const float headW = 0.30f, headH = 0.35f, headD = 0.30f;
    const float headBottom = bodyBottom + bodyH;
    const float headCentre = headBottom + headH * 0.5f;

    const float armRadius = 0.08f;
    const float armLength = 0.90f;
    const float armTop = bodyBottom + bodyH * 0.85f;
    const float armGap = bodyW * 0.5f + armRadius * 0.5f;   // arm centre just outside body

    // --- two legs -----------------------------------------------------
    for (float x : { -legGap, legGap }) {
        parts.push_back({
            Primitives::CreateCylinder(legRadius, legHeight, 14, Palette::Copper,
                                       /*centered=*/false),
            // centred=false means the cylinder grows upward from y=0; its
            // midpoint sits at y = legHeight/2.
            glm::translate(glm::mat4(1.0f), glm::vec3(x, legHeight * 0.5f, 0.0f))
        });
    }

    // --- body (a copper-coloured cube) --------------------------------
    parts.push_back({
        Primitives::CreateBox(bodyW, bodyH, bodyD, Palette::Copper),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, bodyCentre, 0.0f))
    });

    // --- head (slightly darker / smaller box) --------------------------
    parts.push_back({
        Primitives::CreateBox(headW, headH, headD, Palette::Bore),
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, headCentre, 0.0f))
    });

    // --- two arms (cylinders hanging at the sides) --------------------
    for (float x : { -armGap, armGap }) {
        parts.push_back({
            Primitives::CreateCylinder(armRadius, armLength, 12, Palette::Copper,
                                       /*centered=*/false),
            glm::translate(glm::mat4(1.0f), glm::vec3(x, armTop - armLength * 0.5f, 0.0f))
        });
    }
}

void Robot::Draw(Shader& shader) {
    DrawParts(shader, transform, parts);
}

void Robot::Delete() {
    DeleteParts(parts);
}