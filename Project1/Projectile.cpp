#include "Projectile.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Projectile::Projectile(glm::vec3 muzzlePos, glm::vec3 velocity, float radius)
    : mesh(Primitives::CreateSphere(radius, 16, 16, Palette::Iron)),
      position(muzzlePos),
      velocity(velocity),
      radius(radius)
{
    // All initialization happens in the member-initializer list. The sphere
    // mesh has to be built there - Mesh has no default constructor (it owns
    // GPU buffer IDs, so it has to be built deliberately).
    // The ball gets Palette::Iron so it reads as "iron cannon ball", the
    // same metal as the barrel that fired it.
}

void Projectile::Update(float deltaTime, float gravity) {
    // Semi-implicit Euler: update velocity first, then position from the new
    // velocity. This is the standard "good enough" integrator for a parabolic
    // arc - same one the Python version uses (see docs/verification.txt §2).
    velocity.y -= gravity * deltaTime;
    position   += velocity * deltaTime;
    ageSeconds += deltaTime;

    // The ball has hit the ground (a tiny bit below it, so it always
    // registers) or it has been alive too long.
    if (position.y < 0.0f || ageSeconds > LifetimeSeconds) {
        dead = true;
    }
}

void Projectile::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    mesh.Draw();
}

void Projectile::Delete() {
    mesh.Delete();
}