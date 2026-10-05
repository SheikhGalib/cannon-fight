#ifndef SIGNALTOWER_H
#define SIGNALTOWER_H

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Part.h"
#include "shaderClass.h"

// A small stone signal tower used as a riverside landmark.
//
// Built as:
//   - a square stone body (1.6 x 4.0 x 1.6 m)
//   - a stone parapet ring at the top (slightly wider than the body)
//   - a crenellated top (merlons spaced around the parapet)
//   - a conical wooden roof
//   - a tiny fire brazier on the very top (an orange-red box)
//
// One tower sits at a fixed world position; the constructor takes
// the foot centre + a per-tower size scale (so we can vary tower
// heights along the river without per-tower subclasses).
class SignalTower {
public:
    // baseWorld : foot-centre of the tower (in world space).
    // height    : total tower height including roof (m).
    SignalTower(glm::vec3 baseWorld, float height);

    void Draw(Shader& shader);
    void Delete();

private:
    std::vector<Part> parts;
};

#endif