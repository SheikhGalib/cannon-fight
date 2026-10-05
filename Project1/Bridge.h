#ifndef BRIDGE_H
#define BRIDGE_H

#include <glm/glm.hpp>
#include <vector>
#include "Part.h"
#include "shaderClass.h"

// A flat stone bridge deck crossing the moat, with two low rails down
// the long sides.  All pieces are static Parts with no breakability.
//
// Layout (looking from above, +X = bridge length, +Z = width):
//
//     ┌──────────────────────────┐
//     │ ░ rail ░                 │
//     ╞══════════════════════════╡  ← deck (long box)
//     │              ░ rail ░    │
//     └──────────────────────────┘
//
// `centre` is the world-space centre of the deck.  `lengthX` is how long
// the bridge runs along X.  `widthZ` is the deck width along Z.
class Bridge {
public:
    Bridge(glm::vec3 centre, float lengthX, float widthZ);

    void Draw(Shader& shader);
    void Delete();

private:
    std::vector<Part> parts;     // deck + 2 rails
};

#endif