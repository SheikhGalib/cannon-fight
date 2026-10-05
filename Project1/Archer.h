#ifndef ARCHER_H
#define ARCHER_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"
#include "shaderClass.h"

// A medieval archer figure standing on top of a castle tower. Same
// detailed-medieval body proportions as Soldier, but with a longbow +
// quiver strapped to the back so it reads as "archer defending the
// battlements" instead of "cannon crew member".
//
// Archer space: the figure's origin is the centre of its feet on the
// ground (y = 0); the legs grow up from there, the body sits on top of
// the legs, the head + helmet on top of the body. So one translation
// matrix carries the whole archer around.
class Archer {
public:
    // baseWorld : world-space position of the archer's feet centre.
    Archer(glm::vec3 baseWorld);

    // Reposition the archer (used by Cannon's fire sequence to swap the
    // archer back to its rest position after lighting the fuse).
    void SetPosition(glm::vec3 baseWorld);
    glm::vec3 GetPosition() const { return glm::vec3(transform[3]); }

    // Phase 7: rotate the whole archer around Y (used to face the camera or battlefield).
    void SetYaw(float degrees) { yawDegrees = degrees; }
    float GetYaw() const { return yawDegrees; }

    void TriggerShootAnim() { shootAnim = 0.6f; }
    void Update(float dt);

    void Draw(Shader& shader);
    void Delete();

private:
    std::vector<Part> parts;
    glm::mat4 transform;
    float yawDegrees = 0.0f;     // extra rotation around Y, applied in Draw
    float shootAnim = 0.0f;
};

#endif