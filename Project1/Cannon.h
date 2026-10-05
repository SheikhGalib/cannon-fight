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
// `Elevate(d)` tips the barrel up or down.  `Yaw(d)` rotates the whole
// gun left/right around its own centre (Phase 7).
class Cannon {
public:
    Cannon();
    Cannon(glm::vec3 baseWorld);

    // Phase 7: walks the cannon along its current FORWARD direction
    // (local +X rotated by yaw), not world +X.
    void MoveForward(float distance);
    void Elevate(float deltaDegrees);

    // Phase 7: yaws the cannon about its own centre.  `deltaDeg` is
    // added; yawDeg is then clamped to [-MaxYawDeg, +MaxYawDeg] so the
    // field gun doesn't spin all the way around (the trail spade is
    // dug into the dirt in real life).
    void Yaw(float deltaDeg);
    static constexpr float MaxYawDeg = 35.0f;

    // Phase 5+: each frame's Update() advances the carriage recoil decay
    // (the carriage springing back after firing) AND the muzzle-flash
    // intensity decay.
    void Update(float deltaTime);
    void Fire();

    // What the muzzle tip is at in world space, given the cannon's
    // current state. Passes the cannon's carriage matrix into the shaft,
    // then wraps that with the yaw rotation about the cannon centre.
    glm::vec3 GetMuzzleWorldPosition() const;
    glm::vec3 GetForwardWorldDirection() const;

    // World-space centre the cannon yaws about (the carriage.position).
    glm::vec3 GetCentreWorldPosition() const;

    void Draw(Shader& shader);
    void Delete();

    float GetElevationDegrees() const { return shaft.GetElevationDegrees(); }
    float GetYawDegrees()      const { return yawDeg; }

private:
    // Build the matrix that turns a carriage-local point into world
    // space, accounting for the cannon's yaw around its own centre.
    // Used for muzzle + forward queries and Draw().
    glm::mat4 YawWrappedMatrix() const;

    Carriage carriage;
    Wheel    leftWheel;
    Wheel    rightWheel;
    Shaft    shaft;

    float yawDeg = 0.0f;            // accumulated yaw around Y, in degrees

    // Phase 7: muzzle flash. Set to 1.0 in Fire(), decays linearly to
    // 0 over kFlashDuration seconds in Update(). Draw() shows a glowing
    // sphere at the muzzle as long as it's > 0.
    float flashIntensity = 0.0f;
    static constexpr float kFlashDuration = 0.18f;
};

#endif