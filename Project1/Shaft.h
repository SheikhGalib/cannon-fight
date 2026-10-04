#ifndef SHAFT_H
#define SHAFT_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"
#include "Transform.h"
#include "shaderClass.h"

// The cannon's barrel (the "shaft"), and the one moving part Phase 1 asks for:
// it tips up and down about its trunnion pivot when Elevate() is called.
//
// Barrel space: the local origin is the TRUNNION PIVOT - the point the barrel
// turns around, held by the carriage's cheeks - and the tube runs along +X
// from a rounded breech behind the pivot out to the muzzle in front of it.
// Putting the origin at the pivot rather than at the back of the tube is what
// makes elevation a plain rotation with no correcting translation afterwards:
// a rotation always happens about the local origin, so the origin IS the pivot.
//
// The tube is not one cylinder but a short stack of cones with slightly
// different radii - breech, first reinforce, chase, muzzle swell - which is
// how a real gun tube is shaped: thickest where the pressure is, tapering
// towards the mouth, with a lip at the end.
class Shaft {
public:
    // pivotPosition : where the trunnion pivot sits, relative to whatever
    //                 parent matrix Draw() is given (the carriage).
    Shaft(glm::vec3 pivotPosition);

    // Tips the barrel up (positive) or down (negative) by this many degrees,
    // clamped to [MinElevationDeg, MaxElevationDeg]. Call it once per frame
    // with a small step (speed * deltaTime), not once with a big jump.
    void Elevate(float deltaDegrees);

    void Draw(Shader& shader, const glm::mat4& parentMatrix);
    void Delete();

    float GetElevationDegrees() const { return elevationDegrees; }

    // World-space position of the muzzle tip, given the parent matrix Draw()
    // would be called with. Phase 2 uses this to spawn projectiles so they
    // always leave the actual current muzzle, no matter how the gun has been
    // driven or aimed.
    glm::vec3 GetMuzzleWorldPosition(const glm::mat4& parentMatrix) const;

    // World-space forward direction (unit vector, +X in barrel space) at the
    // current elevation, in the same parent space. Phase 2 uses this to give
    // each new projectile its initial velocity.
    glm::vec3 GetForwardWorldDirection(const glm::mat4& parentMatrix) const;

    static constexpr float MinElevationDeg = 0.0f;
    static constexpr float MaxElevationDeg = 45.0f;

    // Only used by tools/RenderDocShots.cpp, to draw the barrel one stage at a
    // time for docs/code-walkthrough.md. Barrel part order is:
    //   0 breech | 1-2 cascabel | 3 reinforce | 4 chase | 5-6 muzzle swell
    //   | 7-8 brass rings | 9 vent | 10 bore
    std::vector<Part>& BarrelPartsForDocs() { return barrelParts; }
    std::vector<Part>& TrunnionPartsForDocs() { return trunnionParts; }
    glm::mat4 GetMatrixForDocs() const { return transform.GetMatrix(); }

private:
    // Parts that turn WITH the barrel (the tube, its rings, the bore).
    std::vector<Part> barrelParts;
    // The two trunnion stubs. They belong to the mount, not the tube, so they
    // are drawn at the pivot WITHOUT the elevation rotation - which also means
    // they stay put in Phase 2 when the tube slides back under recoil.
    std::vector<Part> trunnionParts;

    Transform transform;            // pivot position + the elevation rotation
    float elevationDegrees = 0.0f;
};

#endif
