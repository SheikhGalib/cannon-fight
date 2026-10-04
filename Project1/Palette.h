#ifndef PALETTE_H
#define PALETTE_H

#include <glm/glm.hpp>

// Every color used anywhere in the scene, in one place, so the whole cannon
// stays visually consistent and a color can be re-tuned by editing one line
// instead of hunting through Wheel.cpp / Carriage.cpp / Shaft.cpp.
//
// These are plain RGB values in 0..1 (not 0..255). lit.frag then multiplies
// each one by a brightness that depends on how the surface faces the light,
// so a single color here shows up as a lit side and a shaded side.
namespace Palette {

    inline const glm::vec3 Wood      (0.42f, 0.24f, 0.11f);  // carriage beams, cheeks
    inline const glm::vec3 WoodLight (0.60f, 0.38f, 0.17f);  // wheel spokes / felloe / hub
    inline const glm::vec3 Iron      (0.17f, 0.17f, 0.19f);  // the barrel itself
    inline const glm::vec3 DarkIron  (0.08f, 0.08f, 0.09f);  // wheel tires, axle, trail spade
    inline const glm::vec3 Brass     (0.72f, 0.55f, 0.15f);  // barrel rings, trunnions, hub caps
    inline const glm::vec3 Bore      (0.03f, 0.03f, 0.03f);  // the hole in the muzzle
    inline const glm::vec3 Grass     (0.27f, 0.35f, 0.19f);  // the ground plane

}

#endif
