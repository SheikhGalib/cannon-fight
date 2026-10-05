#ifndef TOWER_H
#define TOWER_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"
#include "shaderClass.h"

// A square stone tower - the kind that flanks a castle gatehouse. Static
// (never breaks), built as a list of Parts:
//
//   ┌──────┐     ┌────┐    ┌────┐
//   │░░░░░░│     │    │    │    │   ← flag (2 Part planes)
//   │░░░░░░│     ░░░░░░░░░░░░░░░░    ← flagpole + merlons (Crenellation)
//   │░merlons│   ░░░░░░░░░░░░░░░░
//   ├───────┤
//   │       │
//   │ [≡]   │    ← arrow-slit windows (4: one per face, in middle)
//   │       │
//   │       │
//   │       │
//   └───────┘    ← quoins at 4 corners (slightly darker)
//
// Construction centres the tower at baseCentre. The body is `bodyH` tall;
// above it sits a `parapetH` merlon-and-crenel row from Crenellation::AlongX
// and Crenellation::AlongZ. On top, a thin flagpole and a small flag.
//
// Default sizes used by Castle.cpp:
//   side = 2.5 m, bodyH = 5.0 m, parapetH = 0.6 m, flagpoleH = 1.5 m.
class Tower {
public:
    Tower(glm::vec3 baseCentre,
          float side,        // width and depth of the body (square plan)
          float bodyH,       // height of the main stone block
          float parapetH,    // merlon height on top of the body
          float merlonW,     // merlon width
          float gap,         // gap between merlons
          float flagpoleH);  // height of the thin flagpole above the merlons

    void Draw(Shader& shader);
    void Delete();

private:
    std::vector<Part> parts;
};

#endif