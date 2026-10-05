#include "SignalTower.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>

SignalTower::SignalTower(glm::vec3 baseWorld, float height)
{
    // Geometry derived from a single `height` knob (everything is
    // scaled proportionally to it).
    const float bodyW   = 1.6f * (height / 6.0f);
    const float bodyH   = height * 0.62f;     // body fills most of the height
    const float roofH   = height * 0.22f;     // conical roof
    const float paraT   = height * 0.06f;     // parapet thickness
    const float paraH   = height * 0.10f;     // parapet height
    const float merlonW = bodyW * 0.32f;
    const float merlonH = paraH * 1.10f;
    const float merlonD = bodyW * 0.30f;
    const float brazierR = bodyW * 0.25f;
    const float brazierH = bodyW * 0.18f;

    // 1. Stone body (square)
    parts.push_back({
        Primitives::CreateBox(bodyW, bodyH, bodyW, Palette::Stone),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(baseWorld.x, baseWorld.y + bodyH * 0.5f,
                                 baseWorld.z))
    });
    // 2. Parapet ring (a slightly wider thin slab on top of the body)
    parts.push_back({
        Primitives::CreateBox(bodyW * 1.18f, paraH, bodyW * 1.18f, Palette::Stone),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(baseWorld.x,
                                 baseWorld.y + bodyH + paraH * 0.5f,
                                 baseWorld.z))
    });
    // 3. Crenellations (4 merlons on the +X/-X/+Z/-Z faces)
    const float merlonY = baseWorld.y + bodyH + paraH + merlonH * 0.5f;
    const float merlonOff = bodyW * 1.18f * 0.5f - merlonD * 0.5f;
    for (float xSide : { -merlonOff, +merlonOff }) {
        parts.push_back({
            Primitives::CreateBox(merlonW, merlonH, merlonD, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                           glm::vec3(baseWorld.x + xSide, merlonY, baseWorld.z))
        });
    }
    for (float zSide : { -merlonOff, +merlonOff }) {
        parts.push_back({
            Primitives::CreateBox(merlonD, merlonH, merlonW, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                           glm::vec3(baseWorld.x, merlonY,
                                     baseWorld.z + zSide))
        });
    }
    // 4. Conical wooden roof (a cone tapering to a point)
    parts.push_back({
        Primitives::CreateCone(bodyW * 0.62f, 0.0f, roofH, 12,
                               Palette::TentCloth, /*centered=*/false),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(baseWorld.x,
                                 baseWorld.y + bodyH + paraH,
                                 baseWorld.z))
    });
    // 5. Fire brazier on top - an orange "flame" cube just above
    //    the roof apex.  Sits at baseWorld.y + bodyH + paraH + roofH
    //    so it reads as a beacon.
    parts.push_back({
        Primitives::CreateBox(brazierR, brazierH, brazierR,
                              Palette::Flash /* orange-yellow */),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(baseWorld.x,
                                 baseWorld.y + bodyH + paraH + roofH
                                              + brazierH * 0.5f,
                                 baseWorld.z))
    });
}

void SignalTower::Draw(Shader& shader) {
    DrawParts(shader, glm::mat4(1.0f), parts);
}

void SignalTower::Delete() {
    DeleteParts(parts);
}