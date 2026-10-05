#ifndef SOLDIER_H
#define SOLDIER_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"
#include "shaderClass.h"

// A medieval soldier figure. Same detailed-medieval body proportions as
// Archer, but with a short sword at the right hip instead of a longbow.
//
// Used in two flavours:
//   * Cannon crew — 3 of them, one per cannon, with bodyColour =
//     Palette::Copper. They walk over to light the cannon during the
//     firing sequence.
//   * Army behind the cannons — 15 of them in a 5x3 grid, with
//     bodyColour = Palette::Iron.
//
// Soldier space: the figure's origin is the centre of its feet on the
// ground (y = 0); the legs grow up from there, the body sits on top of
// the legs, the head + helmet on top of the body. So one translation
// matrix carries the whole soldier around.
class Soldier {
public:
    // baseWorld   : world-space position of the soldier's feet centre.
    // bodyColour  : the colour of the jerkin/torso box. Cannon crew uses
    //               Palette::Copper (warm leather); army uses
    //               Palette::Iron (uniform grey).
    Soldier(glm::vec3 baseWorld, glm::vec3 bodyColour);

    // Reposition the soldier. Used during the cannon-firing sequence
    // when the crew lerps from its rest position to the cannon.
    void SetPosition(glm::vec3 baseWorld);
    glm::vec3 GetPosition() const { return glm::vec3(transform[3]); }

    // Phase 7: rotate the whole soldier around Y (used so the army
    // consistently faces the castle door rather than the default +X).
    void SetYaw(float degrees) { yawDegrees = degrees; }

    void Draw(Shader& shader);
    void Delete();

private:
    std::vector<Part> parts;
    glm::mat4 transform;
    float yawDegrees = 0.0f;
};

#endif