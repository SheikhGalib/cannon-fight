#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>

// Turns a position + one rotation + a scale into a single 4x4 "model matrix".
// This is the one reusable move/rotate/scale building block every drawable
// object (Wheel, Shaft, ...) uses instead of each writing its own matrix math.
//
// Only ONE rotation axis is supported for now (Phase 1 only ever needs one -
// see docs/phase-1-plan.md for why). Phase 2's wheel-rolling will need a
// second, independent rotation; see docs/phase-2-plan.md for how that's
// planned to be added without breaking this class's existing users.
class Transform {
public:
    glm::vec3 position       = glm::vec3(0.0f);
    glm::vec3 rotationAxis   = glm::vec3(0.0f, 1.0f, 0.0f);
    float     rotationDegrees = 0.0f;
    glm::vec3 scale          = glm::vec3(1.0f);

    // Builds the matrix in the standard order: scale first, then rotate,
    // then move into position (matrices apply right-to-left).
    glm::mat4 GetMatrix() const;
};

#endif
