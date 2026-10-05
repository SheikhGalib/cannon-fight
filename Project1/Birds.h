#ifndef BIRDS_H
#define BIRDS_H

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Part.h"
#include "shaderClass.h"

// A small flock of stylised birds flying in wide circles overhead.
//
// Each bird is a simple "V" of two thin, slightly-tilted boxes
// (the wings), drawn from above as a flat silhouette.  A subtle
// per-frame wing-flap animation rotates the wings about the
// bird's body axis.  Birds follow circular orbits at different
// radii / heights / speeds so the flock feels organic.
//
// Birds orbit around `centre` at altitude `alt`.  The orbit plane
// is the XZ plane (a horizontal circle), with the bird always
// facing along its velocity (tangent to the circle).
class Birds {
public:
    // numBirds   : how many birds in the flock.
    // centre     : orbit centre (world space).  Usually the castle.
    // alt        : orbit altitude (m).
    // radiusMin/radiusMax : orbit radius range for variety.
    Birds(int numBirds, glm::vec3 centre, float alt,
          float radiusMin, float radiusMax);

    // Advance orbit + wing-flap animation.
    void Update(float dt);

    void Draw(Shader& shader);
    void Delete();

private:
    // Each bird flies a horizontal circle around `centre` at a
    // fixed altitude, with its own radius, angular speed and
    // starting phase.
    struct Bird {
        float radius;        // metres from centre
        float angularSpeed;  // radians / second (positive = CCW seen from +Y)
        float phase;         // radians, current angle around the circle
        float flapPhase;     // radians, current wing-flap phase
        float scale;         // overall bird size multiplier
    };
    std::vector<Bird> birds;
    glm::vec3 centre;
    float altitude;

    // Two shared meshes (left wing + right wing), each a thin box.
    Mesh wingMesh;

    // Each bird owns 2 parts (left + right wing) - both have
    // their `local` rebuilt every frame as the bird orbits + the
    // wings flap.
    std::vector<Part> parts;
};

#endif