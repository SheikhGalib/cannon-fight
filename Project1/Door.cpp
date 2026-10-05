#include "Door.h"
#include "Primitives.h"
#include "Palette.h"
#include "Part.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <cmath>

Door::Door(glm::vec3 centreWorld,
           float doorHeight, float doorWidth, float panelDepth, int plankCount) {
    // Two panels side-by-side, meeting at the doorway centre. Each panel is
    // half the doorWidth. The panel itself is a thin box; the plank strips
    // sit just in front of it (toward +Z, the side the cannon is on) so they
    // catch the light and read as raised planks.
    const float halfDoor  = doorWidth  * 0.5f;
    const float panelW    = doorWidth * 0.5f;
    const float panelH    = doorHeight;
    const float panelD    = panelDepth;
    const float plankT    = 0.04f;                  // plank thickness
    const float plankW    = panelW * 0.92f;          // slightly narrower than panel
    const float plankStep = panelH / float(plankCount + 1);   // evenly spaced

    panelSize = glm::vec3(panelW, panelH, panelD);

    // Offsets of the two panel CENTRES from the doorway centre, along X.
    const float offsets[2] = { -halfDoor * 0.5f, +halfDoor * 0.5f };

    for (size_t side = 0; side < 2; side++) {
        const float cx = centreWorld.x + offsets[side];
        const float cy = doorHeight * 0.5f;
        const float cz = centreWorld.z;

        // Construct the Panel directly in the vector (Panel has no default
        // ctor because Mesh owns GPU buffer IDs - so we build it in place).
        Panel& p = panels.emplace_back(
            Primitives::CreateBox(panelW, panelH, panelD, Palette::Wood),
            glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, cz)),
            std::vector<Part>{},
            /*alive=*/true);
        p.half[0] = panelW * 0.5f;
        p.half[1] = panelH * 0.5f;
        p.half[2] = panelD * 0.5f;

        // Horizontal plank strips, evenly spaced vertically, sitting just
        // in front of the panel face. The plank's centre Z is panel's CZ +
        // half-depth + half-plank-thickness (so it pokes out the +Z side).
        for (int i = 0; i < plankCount; i++) {
            const float plankY = (float(i) + 1.0f) * plankStep;
            p.planks.push_back({
                Primitives::CreateBox(plankW, plankT, plankT, Palette::WoodLight),
                glm::translate(glm::mat4(1.0f),
                               glm::vec3(cx, plankY,
                                         cz + panelD * 0.5f + plankT * 0.5f))
            });
        }
    }
}

bool Door::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
    bool anyKilled = false;
    for (Panel& p : panels) {
        if (!p.alive) continue;
        glm::vec3 centre(p.local[3]);
        // Offset of the sphere from the panel centre, then clamped per-axis to
        // the panel's half-extents: that clamp gives the OFFSET of the
        // closest point on the panel box from the box centre. Subtract that
        // from the raw offset to get the actual sphere-to-closest-point
        // vector (zero when the sphere centre is inside the box).
        glm::vec3 offset(
            sphereCentre.x - centre.x,
            sphereCentre.y - centre.y,
            sphereCentre.z - centre.z);
        glm::vec3 clamped(
            std::fmax(-p.half[0], std::fmin(offset.x, p.half[0])),
            std::fmax(-p.half[1], std::fmin(offset.y, p.half[1])),
            std::fmax(-p.half[2], std::fmin(offset.z, p.half[2])));
        glm::vec3 delta = offset - clamped;
        if (glm::dot(delta, delta) <= sphereRadius * sphereRadius) {
            p.alive = false;
            anyKilled = true;
        }
    }
    return anyKilled;
}

int Door::AlivePanelCount() const {
    int n = 0;
    for (const Panel& p : panels) if (p.alive) ++n;
    return n;
}

void Door::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    for (Panel& p : panels) {
        if (!p.alive) continue;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        p.mesh.Draw();
        for (Part& plank : p.planks) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(plank.local));
            plank.mesh.Draw();
        }
    }
}

void Door::Delete() {
    for (Panel& p : panels) {
        p.mesh.Delete();
        for (Part& plank : p.planks) plank.mesh.Delete();
    }
}