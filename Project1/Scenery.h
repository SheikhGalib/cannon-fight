#ifndef SCENERY_H
#define SCENERY_H

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Part.h"
#include "shaderClass.h"

// Distant scenery: a ring of low-poly mountains + rolling valley
// hills placed at the horizon, so the canvas feels "infinite" rather
// than truncated by the edge of the ground plane.  Each mountain is
// a tall cone with a small white snow-cap cone on top; each valley
// hill is a wider, lower cone.  All are placed far enough out that
// the camera (max radius ~120) can't actually fly through them.
//
// The scenery is drawn AFTER the ground plane so the silhouettes sit
// on top of the grass.  No collision - these are pure decoration.
class Scenery {
public:
    // centreWorld : centre of the ring of mountains in world space.
    // innerRadius  : closest distance from centre any mountain sits.
    // outerRadius  : farthest distance from centre any mountain sits.
    // count        : how many mountains / hills to scatter.
    Scenery(glm::vec3 centreWorld, float innerRadius, float outerRadius,
            int count);

    void Draw(Shader& shader);
    void Delete();

private:
    std::vector<Part> parts;
};

#endif
