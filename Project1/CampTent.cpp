#include "CampTent.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

CampTent::CampTent(glm::vec3 baseWorld, float baseRadius, float roofHeight,
                   glm::vec3 roofColour, glm::vec3 baseColour) {
    // Layout, all in tent-local coordinates with the base centre at y=0:
    //
    //     |\  |       pole + flag on top
    //     |_\ |
    //     |   |
    //     | R |        roof: tall cone, radius 0 at top, `baseRadius` at bottom
    //     | O |
    //     | O |        base: low wide cylinder / box, sits on the ground
    //     +---+

    const float baseHeight  = 0.40f;
    const float baseRadiusX = baseRadius;
    const float baseRadiusZ = baseRadius * 0.85f;  // slightly elliptical in plan

    // Base: a flat box.  Sit on the ground, centred at baseWorld.
    parts.push_back({
        Primitives::CreateBox(baseRadiusX * 2.0f, baseHeight, baseRadiusZ * 2.0f,
                              baseColour),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseWorld.x, baseHeight * 0.5f, baseWorld.z))
    });

    // Roof: a tall cone (radius 0 at top, full radius at base).
    parts.push_back({
        Primitives::CreateCone(baseRadiusX * 0.95f, 0.0f, roofHeight, 16,
                               roofColour, /*centered=*/false),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseWorld.x, baseHeight, baseWorld.z))
    });

    // Flagpole + flag on the very top of the cone.
    const float poleH = 0.80f;
    const float poleBaseY = baseHeight + roofHeight;
    parts.push_back({
        Primitives::CreateCylinder(0.04f, poleH, 8, Palette::Iron,
                                   /*centered=*/false),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseWorld.x, poleBaseY + poleH * 0.5f, baseWorld.z))
    });

    const float flagW = 0.55f;
    const float flagH = 0.30f;
    const float flagD = 0.04f;
    const float flagY = poleBaseY + poleH - flagH * 0.5f;
    parts.push_back({
        Primitives::CreateBox(flagW, flagH, flagD, roofColour),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseWorld.x + flagW * 0.5f + 0.05f, flagY,
                      baseWorld.z))
    });
}

void CampTent::Draw(Shader& shader) {
    DrawParts(shader, glm::mat4(1.0f), parts);
}

void CampTent::Delete() {
    DeleteParts(parts);
}
