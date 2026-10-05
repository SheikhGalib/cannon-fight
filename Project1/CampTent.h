#ifndef CAMPTENT_H
#define CAMPTENT_H

#include <glm/glm.hpp>
#include "Mesh.h"
#include "Part.h"
#include "shaderClass.h"

// A medieval campaign tent: a low wooden box for the base, a tall
// cone (the canvas "roof") and a small banner pole flying a cloth
// flag.  Used at the distance behind the army to give the impression
// of a "besieging army camp".
//
// Tent space: the tent's origin is the centre of its base on the
// ground (y = 0).  The base is a wide flat box, the cone sits on top,
// the pole and flag are above that.
class CampTent {
public:
    CampTent(glm::vec3 baseWorld, float baseRadius, float roofHeight,
             glm::vec3 roofColour, glm::vec3 baseColour);

    void Draw(Shader& shader);
    void Delete();

private:
    std::vector<Part> parts;
};

#endif
