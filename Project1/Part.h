#ifndef PART_H
#define PART_H

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "shaderClass.h"

// One rigid piece of a bigger object: a mesh, plus a matrix saying where that
// mesh sits INSIDE its owner.
//
// Why this exists: a wheel isn't one shape, it's ~14 shapes (a tire, a rim,
// ten spokes, a hub, two caps) that all move together. Rather than giving
// each of them its own Wheel-like class, a Wheel just owns a list of Parts.
// Drawing then reads as:
//
//     final matrix = parentMatrix * theObjectsOwnTransform * part.local
//                    ^world/carriage  ^"where the wheel is" ^"where the spoke
//                                      and how it's rolled"  is on the wheel"
//
// So `local` is the *only* new idea here, and it never changes after the
// object is built - it's the part's fixed place in the assembly.
struct Part {
    Mesh mesh;
    glm::mat4 local;
};

// Draws every part, setting the shader's "model" uniform to
// objectMatrix * part.local before each one.
void DrawParts(Shader& shader, const glm::mat4& objectMatrix, std::vector<Part>& parts);

// Draws only parts[first .. first+count-1]. The game never needs this - it is
// what lets tools/RenderDocShots.cpp draw a wheel with only its rim, then only
// its rim + spokes, and so on, to make the step-by-step pictures in
// docs/code-walkthrough.md.
void DrawPartRange(Shader& shader, const glm::mat4& objectMatrix, std::vector<Part>& parts,
                   size_t first, size_t count);

void DeleteParts(std::vector<Part>& parts);

// Small, readable builders for the `local` matrices above. Each one is two
// lines of glm, but naming them keeps Wheel.cpp / Carriage.cpp / Shaft.cpp
// readable as a list of "put this box here, lay this cylinder along Z".
//
// IMPORTANT: every mesh Primitives makes is built standing UP (its length runs
// along +Y), because that's the one orientation the generator code has to
// handle. AlongX/AlongZ below are what turn such a mesh sideways, so the
// cylinder code never needs a "which way does it point" parameter.
namespace Local {

    // Just move the part to p, no rotation.
    glm::mat4 Move(glm::vec3 p);

    // Move to p, THEN spin about Z (the axis a cannon tips/rolls around).
    // Used for the tilted trail beams: position their centre, then tilt them.
    glm::mat4 MoveTurn(glm::vec3 p, float degreesAboutZ);

    // Spin about Z FIRST, then move out to p along the spun axes.
    // Used for wheel spokes: "point in this direction, then step outward".
    glm::mat4 TurnMove(float degreesAboutZ, glm::vec3 p);

    // Move to p and lay the part down so its length runs along +X (forward).
    // Used for every piece of the barrel.
    glm::mat4 AlongX(glm::vec3 p);

    // Move to p and lay the part down so its length runs along +Z (rightward).
    // Used for the axle, the trunnions and the wheel's own discs.
    glm::mat4 AlongZ(glm::vec3 p);

    // Same as AlongZ but pointing the other way (-Z), for the mirrored half
    // of a symmetric pair such as the two brass hub caps.
    glm::mat4 AlongNegZ(glm::vec3 p);

}

#endif
