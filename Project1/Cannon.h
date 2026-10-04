#ifndef CANNON_H
#define CANNON_H

#include <glm/glm.hpp>
#include "Carriage.h"
#include "Wheel.h"
#include "Shaft.h"

// One complete cannon: a `Carriage` (root), two `Wheel`s, and one `Shaft`.
// Wraps all four so a Phase-3 caller can stand three of them side by side
// with three lines of code, instead of declaring three carriages, six
// wheels and three shafts by hand and wiring each one's parent matrix.
//
// Cannon space: the cannon's origin is the centre of the wheels' axle.
// A `MoveForward(d)` call moves the whole gun; the wheels are rolled the
// same distance so the rolling-without-slipping illusion still works.
// `Elevate(d)` tips the barrel up or down.
class Cannon {
public:
    Cannon();
    Cannon(glm::vec3 baseWorld);

    void MoveForward(float distance);
    void Elevate(float deltaDegrees);

    // What the muzzle tip is at in world space, given the cannon's
    // current state. Passes the cannon's carriage matrix into the shaft.
    glm::vec3 GetMuzzleWorldPosition() const;
    glm::vec3 GetForwardWorldDirection() const;

    void Draw(Shader& shader);
    void Delete();

    float GetElevationDegrees() const { return shaft.GetElevationDegrees(); }

private:
    Carriage carriage;
    Wheel    leftWheel;
    Wheel    rightWheel;
    Shaft    shaft;
};

#endif