#include "Bridge.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Bridge::Bridge(glm::vec3 centre, float lengthX, float widthZ) {
    // The deck: a thin slab, slightly above the water (so the water
    // doesn't poke through).  Palette::Bridge is the stone-grey.
    const float deckH = 0.15f;
    parts.push_back({
        Primitives::CreateBox(lengthX, deckH, widthZ, Palette::Bridge),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(centre.x, deckH * 0.5f, centre.z))
    });

    // Two low rails along the long sides.  Each rail is a thin box
    // running along the bridge length, just outside the deck footprint.
    const float railH = 0.50f;
    const float railT = 0.10f;
    const float halfW = widthZ * 0.5f;
    const float railOffset = halfW + railT * 0.5f;

    for (float zSide : { -railOffset, +railOffset }) {
        parts.push_back({
            Primitives::CreateBox(lengthX, railH, railT, Palette::Bridge),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(centre.x, railH * 0.5f, centre.z + zSide))
        });
    }

    // Four short posts at each end of each rail, so the bridge doesn't
    // look like a half-finished slab.  Each post is a small box at a
    // corner of the deck, sticking up the height of the rail.
    const float postW = 0.12f;
    const float halfL = lengthX * 0.5f;
    const float postH = railH + 0.10f;
    for (float xEnd : { -halfL, +halfL }) {
        for (float zSide : { -railOffset, +railOffset }) {
            parts.push_back({
                Primitives::CreateBox(postW, postH, postW, Palette::Bridge),
                glm::translate(glm::mat4(1.0f),
                    glm::vec3(centre.x + xEnd, postH * 0.5f, centre.z + zSide))
            });
        }
    }
}

void Bridge::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    for (Part& p : parts) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        p.mesh.Draw();
    }
}

void Bridge::Delete() {
    for (Part& p : parts) p.mesh.Delete();
}