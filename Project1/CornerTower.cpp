#include "CornerTower.h"
#include "Crenellation.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

CornerTower::CornerTower(glm::vec3 baseCentre,
                         float side, float bodyH, float parapetH,
                         float merlonW, float gap, float flagpoleH) {
    centre = glm::vec3(baseCentre.x, bodyH * 0.5f, baseCentre.z);
    half = glm::vec3(side * 0.5f, bodyH * 0.5f, side * 0.5f);
    size = glm::vec3(side, bodyH, side);
    alive = true;
    health = 1.0f;

    const float halfSide = side * 0.5f;

    // --- main stone body ------------------------------------------------
    parts.push_back({
        Primitives::CreateBox(side, bodyH, side, Palette::Stone),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseCentre.x, bodyH * 0.5f, baseCentre.z))
    });

    // --- 4 corner quoins (slightly darker, slightly proud) --------------
    const float qw = 0.25f;
    const float qd = 0.25f;
    const float qh = bodyH;
    const float qoff = halfSide;
    const float cornerOffsets[4][2] = {
        { -qoff, -qoff }, { +qoff, -qoff },
        { -qoff, +qoff }, { +qoff, +qoff },
    };
    for (auto& co : cornerOffsets) {
        parts.push_back({
            Primitives::CreateBox(qw, qh, qd, Palette::DarkIron),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(baseCentre.x + co[0], qh * 0.5f, baseCentre.z + co[1]))
        });
    }

    // --- arrow-slit windows (8 of them, two per face) -------------------
    // Top slit ~70% up, bottom slit ~30% up the body.  All sit on the
    // centre-line of their face.
    const float slitW = 0.12f;
    const float slitH = 0.95f;
    const float slitD = 0.05f;
    const float topSlitY = bodyH * 0.70f;
    const float botSlitY = bodyH * 0.30f;

    for (float yLevel : { botSlitY, topSlitY }) {
        // +X face
        parts.push_back({
            Primitives::CreateBox(slitD, slitH, slitW, Palette::Bore),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(baseCentre.x + halfSide + slitD * 0.5f - 0.02f,
                          yLevel, baseCentre.z))
        });
        // -X face
        parts.push_back({
            Primitives::CreateBox(slitD, slitH, slitW, Palette::Bore),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(baseCentre.x - halfSide - slitD * 0.5f + 0.02f,
                          yLevel, baseCentre.z))
        });
        // +Z face
        parts.push_back({
            Primitives::CreateBox(slitW, slitH, slitD, Palette::Bore),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(baseCentre.x, yLevel,
                          baseCentre.z + halfSide + slitD * 0.5f - 0.02f))
        });
        // -Z face
        parts.push_back({
            Primitives::CreateBox(slitW, slitH, slitD, Palette::Bore),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(baseCentre.x, yLevel,
                          baseCentre.z - halfSide - slitD * 0.5f + 0.02f))
        });
    }

    // --- merlon parapet on top of the body ------------------------------
    const float merlonY = bodyH;
    const float merlonDepth = 0.40f;

    auto frontParts = Crenellation::AlongX(
        baseCentre.x - halfSide, merlonY, baseCentre.z + halfSide - merlonDepth * 0.5f,
        side, merlonW, parapetH, merlonDepth, gap, Palette::Stone);
    for (auto& p : frontParts) parts.push_back(std::move(p));

    auto backParts = Crenellation::AlongX(
        baseCentre.x - halfSide, merlonY, baseCentre.z - halfSide + merlonDepth * 0.5f,
        side, merlonW, parapetH, merlonDepth, gap, Palette::Stone);
    for (auto& p : backParts) parts.push_back(std::move(p));

    auto leftParts = Crenellation::AlongZ(
        baseCentre.x - halfSide + merlonDepth * 0.5f, merlonY, baseCentre.z - halfSide,
        side, merlonW, parapetH, merlonDepth, gap, Palette::Stone);
    for (auto& p : leftParts) parts.push_back(std::move(p));

    auto rightParts = Crenellation::AlongZ(
        baseCentre.x + halfSide - merlonDepth * 0.5f, merlonY, baseCentre.z - halfSide,
        side, merlonW, parapetH, merlonDepth, gap, Palette::Stone);
    for (auto& p : rightParts) parts.push_back(std::move(p));

    // --- flagpole + flag on top -----------------------------------------
    const float poleW = 0.07f;
    const float poleBaseY = bodyH + parapetH;
    const float poleTopY = poleBaseY + flagpoleH;
    parts.push_back({
        Primitives::CreateBox(poleW, flagpoleH, poleW, Palette::Iron),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseCentre.x, poleBaseY + flagpoleH * 0.5f, baseCentre.z))
    });

    const float flagW = 1.00f;
    const float flagH = 0.55f;
    const float flagD = 0.05f;
    const float flagY = poleTopY - flagH * 0.5f - 0.05f;
    parts.push_back({
        Primitives::CreateBox(flagW, flagH, flagD, Palette::Wood),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseCentre.x + flagW * 0.5f + poleW * 0.5f, flagY,
                      baseCentre.z))
    });
}

bool CornerTower::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
    if (!alive) return false;
    glm::vec3 d(
        sphereCentre.x - std::fmax(centre.x - half.x, std::fmin(sphereCentre.x, centre.x + half.x)),
        sphereCentre.y - std::fmax(centre.y - half.y, std::fmin(sphereCentre.y, centre.y + half.y)),
        sphereCentre.z - std::fmax(centre.z - half.z, std::fmin(sphereCentre.z, centre.z + half.z))
    );
    if (glm::dot(d, d) <= sphereRadius * sphereRadius) {
        health -= 0.20f;
        float t = glm::clamp(1.0f - health, 0.0f, 1.0f);
        glm::vec3 newColour = Palette::Stone * (1.0f - t) + Palette::StoneDark * t;
        parts[0].mesh.Delete();
        parts[0].mesh = Primitives::CreateBox(size.x, size.y, size.z, newColour);
        if (health <= 0.0f) {
            alive = false;
        }
        return true;
    }
    return false;
}

void CornerTower::Draw(Shader& shader) {
    if (!alive) return;
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    for (Part& p : parts) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        p.mesh.Draw();
    }
}

void CornerTower::Delete() {
    for (Part& p : parts) p.mesh.Delete();
}