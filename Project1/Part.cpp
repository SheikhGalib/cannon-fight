#include "Part.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

void DrawParts(Shader& shader, const glm::mat4& objectMatrix, std::vector<Part>& parts) {
    DrawPartRange(shader, objectMatrix, parts, 0, parts.size());
}

void DrawPartRange(Shader& shader, const glm::mat4& objectMatrix, std::vector<Part>& parts,
                   size_t first, size_t count) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    size_t last = std::min(first + count, parts.size());
    for (size_t i = first; i < last; i++) {
        glm::mat4 model = objectMatrix * parts[i].local;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        parts[i].mesh.Draw();
    }
}

void DeleteParts(std::vector<Part>& parts) {
    for (Part& part : parts) {
        part.mesh.Delete();
    }
}

namespace Local {

    static const glm::vec3 AxisX(1.0f, 0.0f, 0.0f);
    static const glm::vec3 AxisZ(0.0f, 0.0f, 1.0f);

    glm::mat4 Move(glm::vec3 p) {
        return glm::translate(glm::mat4(1.0f), p);
    }

    glm::mat4 MoveTurn(glm::vec3 p, float degreesAboutZ) {
        // Matrices apply right-to-left, so writing translate-then-rotate here
        // means the shape is rotated about its own centre first and only then
        // carried out to p. (Rotating about its own centre is what we want for
        // a beam: tilt it in place.)
        return glm::rotate(glm::translate(glm::mat4(1.0f), p),
                           glm::radians(degreesAboutZ), AxisZ);
    }

    glm::mat4 TurnMove(float degreesAboutZ, glm::vec3 p) {
        // The opposite order: the translation happens in the already-rotated
        // frame, so p is a distance measured along the spun axis. That's how
        // ten identical spokes end up evenly spread around the hub.
        return glm::translate(glm::rotate(glm::mat4(1.0f),
                                          glm::radians(degreesAboutZ), AxisZ), p);
    }

    glm::mat4 AlongX(glm::vec3 p) {
        // Turning -90 degrees about Z sends the mesh's +Y (its length) onto +X.
        return glm::rotate(glm::translate(glm::mat4(1.0f), p),
                           glm::radians(-90.0f), AxisZ);
    }

    glm::mat4 AlongZ(glm::vec3 p) {
        // Turning +90 degrees about X sends the mesh's +Y onto +Z.
        return glm::rotate(glm::translate(glm::mat4(1.0f), p),
                           glm::radians(90.0f), AxisX);
    }

    glm::mat4 AlongNegZ(glm::vec3 p) {
        return glm::rotate(glm::translate(glm::mat4(1.0f), p),
                           glm::radians(-90.0f), AxisX);
    }

}
