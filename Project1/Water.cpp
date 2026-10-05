#include "Water.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Water::Water(glm::vec3 centre, float sizeX, float sizeZ) {
    // The plane sits a touch above y=0 (z-fighting with the ground would
    // otherwise cause flicker at the moat edges).  Y = 0.01 is small
    // enough that it looks like the ground has sunk slightly into the moat.
    model = glm::translate(glm::mat4(1.0f),
        glm::vec3(centre.x, 0.01f, centre.z));
    model = glm::scale(model, glm::vec3(sizeX, 1.0f, sizeZ));
    // Reuse a single mesh via a static-ish technique: build a 1x1 plane and
    // scale it via model matrix.  Avoids a per-instance mesh allocation.
}

static Mesh& WaterMesh() {
    static Mesh m = Primitives::CreatePlane(1.0f, 1.0f, Palette::Water);
    return m;
}

void Water::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    WaterMesh().Draw();
}

void Water::Delete() {
    WaterMesh().Delete();
}