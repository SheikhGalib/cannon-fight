#ifndef CRENELLATION_H
#define CRENELLATION_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"

// A crenellated parapet is the tooth-like top of a castle wall - alternating
// upright blocks (merlons) and gaps (crenels). This helper takes a length
// along one axis and produces a list of upright merlons ready to be added
// to a Tower or wall's Part list.
//
//   ▢ ▢ ▢ ▢ ▢ ▢         (top view of the merlons)
//
// All merlons share the same width, depth and height; only their X (or Z)
// position varies. The gaps between merlons are empty space - no geometry.
//
// The shape is a single axis-aligned box built with Primitives::CreateBox.
// Each merlon is centred on the requested axis and lifted so its BASE sits
// at `yBase` (typically the top of a wall).
//
//   yBase
//    │ ┌──┐    ┌──┐    ┌──┐
//    │ │  │    │  │    │  │  ← merlons (height = merlonH)
//    ▼ └──┘    └──┘    └──┘
//      └──┘    └──┘    └──┘
//      m0      m1      m2     ← along +X (or +Z)
namespace Crenellation {

    // Build merlons along the +X axis.
    //   baseX, baseY, baseZ : world-space position of the BOTTOM-LEFT-BACK
    //                         corner of the parapet's first merlon.
    //   lengthX              : total run of merlons (not including any final
    //                          half-spacing at the end).
    //   merlonW              : width of each merlon (along X).
    //   merlonH              : height of each merlon (along Y).
    //   merlonD              : depth of each merlon (along Z).
    //   gap                  : gap between adjacent merlons (along X).
    //   color                : Palette::Stone, etc.
    //
    // The number of merlons is `floor(lengthX / (merlonW + gap))`; the last
    // merlon may not sit exactly at `baseX + lengthX` if `lengthX` doesn't
    // divide evenly. That's fine - in practice the caller picks a length
    // that divides cleanly.
    std::vector<Part> AlongX(float baseX, float baseY, float baseZ,
                             float lengthX,
                             float merlonW, float merlonH, float merlonD,
                             float gap, glm::vec3 color);

    // Same, but along +Z (so the merlons form a row going front-to-back).
    std::vector<Part> AlongZ(float baseX, float baseY, float baseZ,
                             float lengthZ,
                             float merlonW, float merlonH, float merlonD,
                             float gap, glm::vec3 color);

}

#endif
