#ifndef CASTLE_H
#define CASTLE_H

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Part.h"
#include "Tower.h"
#include "Door.h"
#include "FortGate.h"
#include "shaderClass.h"

// The top-level castle composition. Owns:
//
//   * Two flanking Tower objects, one on each side of the gate
//   * The Phase-3 FortGate (breakable brick stacks + lintel + pillars)
//     sandwiched between the towers
//   * Two Door objects filling the gate opening (the new breakable thing
//     - cannon balls aimed through the doorway hit the doors first)
//   * Two crenellated curtain walls extending left/right from the towers
//   * A decorative drawbridge in front of the door
//
// CheckHit(centre, radius) dispatches to the doors first (the cannon is
// aimed at them), then to the FortGate's brick stacks.
//
// `centreWorld` is the world-space centre of the gate opening (same as the
// FortGate constructor takes).
class Castle {
public:
    Castle(glm::vec3 centreWorld,
           float gateWidth,
           float wallHeight    = 3.0f,
           float towerHeight   = 5.0f,
           float curtainLength = 6.0f);

    // Dispatch a sphere-vs-AABB hit test to all breakable pieces.
    // (Doors first, then the FortGate's bricks.)
    bool CheckHit(glm::vec3 sphereCentre, float sphereRadius);

    // Counts of alive / total breakable pieces.
    int AliveDoorPanelCount() const;
    int TotalDoorPanelCount() const;
    int AliveBrickCount() const;
    int TotalBrickCount() const;

    void Draw(Shader& shader);
    void Delete();

private:
    Door      doors;     // inside the gate opening
    FortGate  gate;      // the brick + lintel + pillar assembly
    Tower     leftTower;
    Tower     rightTower;

    // Crenellated curtain walls extending left/right of the towers.
    std::vector<Part> leftCurtain;
    std::vector<Part> rightCurtain;

    // Decorative wooden drawbridge in front of the door.
    std::vector<Part> drawbridge;
};

#endif