#ifndef WATER_H
#define WATER_H

#include <glm/glm.hpp>
#include "shaderClass.h"

// A flat water surface for the moat.  Just a horizontal `CreatePlane`
// positioned slightly below the ground so the moat "sinks" relative to
// the surrounding grass.  No animation, no reflection - the goal is
// "reads as water at a glance", not "looks like a pond".
//
// Drawn after the ground so it overwrites the grass in the moat footprint.
class Water {
public:
    // centre : world-space centre of the moat surface.
    // sizeX  : how long the water patch is along X.
    // sizeZ  : how long the water patch is along Z.
    Water(glm::vec3 centre, float sizeX, float sizeZ);

    void Draw(Shader& shader);
    void Delete();

private:
    glm::mat4 model;
};

#endif