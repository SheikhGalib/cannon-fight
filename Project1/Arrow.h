#ifndef ARROW_H
#define ARROW_H

#include <glm/glm.hpp>
#include "Mesh.h"
#include "shaderClass.h"

// A single arrow in flight, used by the combat system.  An arrow has
// a position + velocity (no rotation, since the lit.frag doesn't
// support per-mesh normal rotation without rebuilding the mesh and
// arrows are tiny on screen).  The mesh is a thin cylinder; the tip
// is implicit in the length direction.
class Arrow {
public:
    Arrow(glm::vec3 startPos, glm::vec3 velocity, float radius = 0.04f);
    void Update(float deltaTime, float gravity = 9.81f);
    void Draw(Shader& shader);
    glm::vec3 GetPosition() const { return position; }
    float GetRadius() const { return radius; }
    bool IsDead() const { return dead; }
    void Kill() { dead = true; }
    void Delete();

private:
    Mesh mesh;
    glm::vec3 position;
    glm::vec3 velocity;
    float radius;
    float ageSeconds = 0.0f;
    bool dead = false;
};

#endif
