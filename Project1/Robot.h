#ifndef ROBOT_H
#define ROBOT_H

#include <glm/glm.hpp>
#include "Mesh.h"
#include "Part.h"
#include "shaderClass.h"

// A simple wooden dummy robot, "18th-century automaton" style: cubic body,
// smaller cubic head, cylindrical arms and legs. Phase 3 places one inside
// the fort gate opening as a stationary target the cannon is pointing at.
//
// Robot space: the robot's origin is at the centre of its feet on the
// ground (y = 0). The legs grow up from there; the body sits on top of the
// legs; the head sits on top of the body; the arms hang from the body's
// shoulders. So one translation matrix carries the whole robot around.
class Robot {
public:
    // baseWorld : world-space position of the robot's feet centre.
    Robot(glm::vec3 baseWorld);

    void Draw(Shader& shader);
    void Delete();

private:
    std::vector<Part> parts;
    glm::mat4 transform;
};

#endif