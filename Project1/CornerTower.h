#ifndef CORNERTOWER_H
#define CORNERTOWER_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"
#include "shaderClass.h"

// A big square corner tower for the corners of the compound castle.
// Larger and more menacing than the regular Tower: body 4x4 m, height
// 8 m, two arrow slits per face, taller parapet.  Built as a flat
// vector<Part>; reuse of `Crenellation` keeps the parapet code DRY.
class CornerTower {
public:
    CornerTower(glm::vec3 baseCentre,
                float side, float bodyH, float parapetH,
                float merlonW, float gap, float flagpoleH);

    void Draw(Shader& shader);
    void Delete();

private:
    std::vector<Part> parts;
};

#endif