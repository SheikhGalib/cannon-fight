#ifndef SKYCLOUDS_H
#define SKYCLOUDS_H

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Part.h"
#include "shaderClass.h"

// Large, slow-moving clouds in the sky.
//
// Each cloud is a cluster of overlapping spheres (same idea as the
// mountain cloud ring, but bigger and at high altitude).  Clouds
// drift along +X at a per-cloud rate; when a cloud's centre crosses
// past +xEnd it wraps back to -xEnd so the population is continuous.
//
// Clouds sit at y in [skyLow..skyHigh] (default 55..75 m) which is
// higher than the castle towers (which top out around 18 m) so they
// read as "sky" rather than "fog around the castle".  Spans an area
// well beyond the ground plane so the user always sees a few of
// them, even when they orbit the camera.
//
// Implementation: each cloud keeps a list of (relativeOffset,
// scale) puffs; the cloud's current centre is updated each frame
// and the per-puff transforms are rebuilt from the relative offsets.
// This keeps memory small (one transform rebuild per cloud per
// frame) and lets clouds drift + wrap cleanly.
class SkyClouds {
public:
    // numClouds : how many cloud blobs to scatter.
    // xSpan     : clouds drift in [-xSpan .. +xSpan] along X and
    //             are initially seeded across this range.
    // zSpan     : the same for Z (clouds are seeded across ±zSpan
    //             so the sky is populated in every direction).
    // skyLow/skyHigh : vertical band the clouds live in (m).
    SkyClouds(int numClouds, float xSpan, float zSpan,
              float skyLow, float skyHigh);

    // Advance each cloud along +X by its own speed; wrap around.
    void Update(float dt);

    void Draw(Shader& shader);
    void Delete();

private:
    // One cloud blob = centre + drift speed + N puffs.
    // Each puff is stored as a (relative offset from the cloud
    // centre, scale) pair, and the corresponding part in `parts`
    // has its `local` matrix rebuilt every frame.
    struct Cloud {
        glm::vec3 centre = glm::vec3(0.0f);
        float speed = 0.0f;       // metres / second along +X
    };

    struct Puff {
        glm::vec3 relative;       // offset from cloud centre
        float     scale;          // uniform sphere radius
    };

    std::vector<Cloud> clouds;
    // For cloud i, puffs[i] is the list of puffs belonging to it
    // (matching the order they appear in `parts`).
    std::vector<std::vector<Puff>> puffs;

    // Wrapping window (metres along X).  When a cloud's X
    // exceeds +xEnd it teleports to -xEnd so the sky stays
    // populated.
    float xEnd;

    // Pre-built sphere mesh shared across every puff.
    Mesh puffMesh;

    std::vector<Part> parts;
};

#endif