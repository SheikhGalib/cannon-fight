#include "Ladder.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Ladder::Ladder(glm::vec3 bottomPos, glm::vec3 topPos, float width, int numRungs)
    : transform(glm::mat4(1.0f)), bottom(bottomPos), top(topPos)
{
    glm::vec3 dir = topPos - bottomPos;
    float len = glm::length(dir);
    if (len < 0.001f) len = 1.0f;
    glm::vec3 up = dir / len;

    // Normal vector perpendicular to ladder length (along X/Y incline)
    glm::vec3 rungsAxis(0.0f, 0.0f, 1.0f); // horizontal rungs run along Z
    glm::vec3 sideRailOffset = rungsAxis * (width * 0.5f);

    float railThickness = 0.08f;
    float railDepth = 0.12f;

    // Rail rotation: Primitives::CreateBox is aligned with axes.
    // We compute the rotation from +Y to `up`:
    float angleDeg = glm::degrees(std::atan2(topPos.x - bottomPos.x, topPos.y - bottomPos.y));

    // Left and right side rails
    for (float sign : { -1.0f, +1.0f }) {
        glm::vec3 railPos = (bottomPos + topPos) * 0.5f + rungsAxis * (sign * width * 0.5f);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), railPos);
        m = glm::rotate(m, glm::radians(angleDeg), glm::vec3(0.0f, 0.0f, -1.0f));
        parts.push_back({
            Primitives::CreateBox(railDepth, len, railThickness, Palette::Wood),
            m
        });
    }

    // Horizontal rungs
    for (int i = 1; i <= numRungs; i++) {
        float t = float(i) / float(numRungs + 1);
        glm::vec3 rungPos = bottomPos + dir * t;
        glm::mat4 m = glm::translate(glm::mat4(1.0f), rungPos);
        m = glm::rotate(m, glm::radians(angleDeg), glm::vec3(0.0f, 0.0f, -1.0f));
        parts.push_back({
            Primitives::CreateCylinder(0.035f, width - railThickness, 8, Palette::Bark, /*centered=*/true),
            glm::rotate(m, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))
        });
    }
}

void Ladder::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    for (Part& p : parts) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        p.mesh.Draw();
    }
}

void Ladder::Delete() {
    for (Part& p : parts) p.mesh.Delete();
}
