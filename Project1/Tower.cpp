#include "Tower.h"
#include "Crenellation.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Tower::Tower(glm::vec3 baseCentre,
             float side, float bodyH, float parapetH,
             float merlonW, float gap, float flagpoleH)
{
    centre = glm::vec3(baseCentre.x, bodyH * 0.5f, baseCentre.z);
    half = glm::vec3(side * 0.5f, bodyH * 0.5f, side * 0.5f);
    size = glm::vec3(side, bodyH, side);
    alive = true;
    health = 1.0f;

    const float halfSide = side * 0.5f;

    // --- main stone body -------------------------------------------------
    parts.push_back({
        Primitives::CreateBox(side, bodyH, side, Palette::Stone),
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(baseCentre.x, bodyH * 0.5f, baseCentre.z))
    });

    // --- 4 corner quoins (slightly darker, slightly proud) ---------------
    // Each quoin is a small box riding the corner edge of the tower body.
    // We use Palette::Wood's "iron" cousin - there's no dedicated "dark
    // stone" colour, so a desaturated darker colour sits well. The simplest
    // thing is to use the Iron colour as a stand-in (it's a near-black grey).
    const float qw = 0.20f;            // quoin width
    const float qd = 0.20f;            // quoin depth
    const float qh = bodyH;            // quoins span the full body height
    const float qoff = halfSide + qw * 0.0f;   // centred on the corner
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

    // --- arrow-slit windows (4 of them, one per face) --------------------
    // Each slit is a narrow vertical box painted Bore (almost black) so it
    // reads as a recessed opening. Centred on each face's midpoint, halfway
    // up the body.
    const float slitW = 0.10f;
    const float slitH = 0.80f;
    const float slitD = 0.05f;
    const float slitY = bodyH * 0.5f;     // halfway up

    // +X face (slit lies in the X=halfSide plane, depth along X)
    parts.push_back({
        Primitives::CreateBox(slitD, slitH, slitW, Palette::Bore),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseCentre.x + halfSide + slitD * 0.5f - 0.02f, slitY, baseCentre.z))
    });
    // -X face
    parts.push_back({
        Primitives::CreateBox(slitD, slitH, slitW, Palette::Bore),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseCentre.x - halfSide - slitD * 0.5f + 0.02f, slitY, baseCentre.z))
    });
    // +Z face
    parts.push_back({
        Primitives::CreateBox(slitW, slitH, slitD, Palette::Bore),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseCentre.x, slitY, baseCentre.z + halfSide + slitD * 0.5f - 0.02f))
    });
    // -Z face
    parts.push_back({
        Primitives::CreateBox(slitW, slitH, slitD, Palette::Bore),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseCentre.x, slitY, baseCentre.z - halfSide - slitD * 0.5f + 0.02f))
    });

    // --- merlon parapet on top of the body -------------------------------
    // Two runs: one along +X (front and back face merlons) and one along +Z
    // (left and right face merlons). They share the same top-of-body level.
    const float merlonY = bodyH;
    const float merlonDepth = 0.30f;

    // Front (+Z) edge - merlons along X, raised merlonY
    auto frontParts = Crenellation::AlongX(
        baseCentre.x - halfSide, merlonY, baseCentre.z + halfSide - merlonDepth * 0.5f,
        side,
        merlonW, parapetH, merlonDepth, gap, Palette::Stone);
    for (auto& p : frontParts) parts.push_back(std::move(p));

    // Back (-Z) edge
    auto backParts = Crenellation::AlongX(
        baseCentre.x - halfSide, merlonY, baseCentre.z - halfSide + merlonDepth * 0.5f,
        side,
        merlonW, parapetH, merlonDepth, gap, Palette::Stone);
    for (auto& p : backParts) parts.push_back(std::move(p));

    // Left (-X) edge - merlons along Z
    auto leftParts = Crenellation::AlongZ(
        baseCentre.x - halfSide + merlonDepth * 0.5f, merlonY, baseCentre.z - halfSide,
        side,
        merlonW, parapetH, merlonDepth, gap, Palette::Stone);
    for (auto& p : leftParts) parts.push_back(std::move(p));

    // Right (+X) edge
    auto rightParts = Crenellation::AlongZ(
        baseCentre.x + halfSide - merlonDepth * 0.5f, merlonY, baseCentre.z - halfSide,
        side,
        merlonW, parapetH, merlonDepth, gap, Palette::Stone);
    for (auto& p : rightParts) parts.push_back(std::move(p));

    // --- flagpole + flag on top ------------------------------------------
    // Pole: thin vertical box, dark iron.
    const float poleW = 0.05f;
    const float poleBaseY = bodyH + parapetH;
    const float poleTopY = poleBaseY + flagpoleH;
    parts.push_back({
        Primitives::CreateBox(poleW, flagpoleH, poleW, Palette::Iron),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseCentre.x, poleBaseY + flagpoleH * 0.5f, baseCentre.z))
    });

    // Flag: a thin horizontal slab attached to the pole near its top.
    // Uses Palette::Wood (the warm brown) as a stand-in "red banner" colour.
    const float flagW = 0.80f;
    const float flagH = 0.45f;
    const float flagD = 0.04f;
    const float flagY = poleTopY - flagH * 0.5f - 0.05f;
    parts.push_back({
        Primitives::CreateBox(flagW, flagH, flagD, Palette::Wood),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(baseCentre.x + flagW * 0.5f + poleW * 0.5f, flagY, baseCentre.z))
    });
}

bool Tower::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
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

void Tower::Draw(Shader& shader) {
    if (!alive) return;
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    for (Part& p : parts) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        p.mesh.Draw();
    }
}

void Tower::Delete() {
    for (Part& p : parts) p.mesh.Delete();
}