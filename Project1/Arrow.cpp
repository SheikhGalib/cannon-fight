#include "Arrow.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Arrow::Arrow(glm::vec3 startPos, glm::vec3 velocity, float radius)
    : mesh(Primitives::CreateCylinder(radius, 0.6f, 6, Palette::Arrow,
                                       /*centered=*/true)),
      position(startPos),
      velocity(velocity),
      radius(radius) {}

void Arrow::Update(float deltaTime, float gravity) {
    velocity.y -= gravity * deltaTime;
    position   += velocity * deltaTime;
    ageSeconds += deltaTime;
    if (position.y < 0.0f || ageSeconds > 5.0f) dead = true;
}

void Arrow::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    // Orient the arrow cylinder along its velocity direction.
    glm::vec3 dirN = glm::length(velocity) > 1e-3f
        ? glm::normalize(velocity) : glm::vec3(0.0f, 1.0f, 0.0f);
    // Cylinder length is along +Y.  Compute the rotation that maps
    // (0,1,0) to dirN.
    glm::vec3 axis = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), dirN);
    float axisLen = glm::length(axis);
    float angle   = std::acos(glm::clamp(dirN.y, -1.0f, 1.0f));
    glm::mat4 R = (axisLen < 1e-3f)
        ? glm::mat4(1.0f)
        : glm::rotate(glm::mat4(1.0f), angle, axis / axisLen);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position) * R;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    mesh.Draw();
}

void Arrow::Delete() {
    mesh.Delete();
}
