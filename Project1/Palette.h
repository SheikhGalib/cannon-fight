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
    inline const glm::vec3 Stone     (0.62f, 0.60f, 0.55f);  // the breakable wall (Phase 2)
    inline const glm::vec3 Bark      (0.36f, 0.22f, 0.10f);  // tree trunks (Phase 3)
    inline const glm::vec3 Leaf      (0.16f, 0.42f, 0.18f);  // tree leaves (Phase 3)
    inline const glm::vec3 Copper    (0.55f, 0.40f, 0.20f);  // dummy robot body (Phase 3)
    inline const glm::vec3 Water     (0.18f, 0.40f, 0.62f);  // moat surface (Phase 5)
    inline const glm::vec3 Bridge    (0.45f, 0.43f, 0.38f);  // stone bridge deck (Phase 5)
    // Phase 6 additions: figure colours (archers + soldiers + bridge chains).
    inline const glm::vec3 Skin      (0.85f, 0.65f, 0.50f);  // faces / hands on the figures
    inline const glm::vec3 Leather   (0.45f, 0.28f, 0.18f);  // sword grip / spare jerkin detail

    // Phase 7 additions: cannon aim + muzzle flash.
    inline const glm::vec3 Indicator (1.00f, 0.90f, 0.25f);  // yellow aim line on the ground
    inline const glm::vec3 Flash     (1.00f, 0.95f, 0.50f);  // brief muzzle-flash sphere
    inline const glm::vec3 StoneDark (0.18f, 0.16f, 0.14f);  // destination colour for damaged bricks

    // Phase 8: combat / scenery colours.
    inline const glm::vec3 Attacker  (0.42f, 0.20f, 0.18f);  // dark red attacker uniform
    inline const glm::vec3 Defender  (0.18f, 0.30f, 0.45f);  // dark blue defender uniform
    inline const glm::vec3 TentCloth (0.60f, 0.40f, 0.20f);  // canvas tent cloth
    inline const glm::vec3 TentBase  (0.35f, 0.22f, 0.12f);  // wooden tent platform
    inline const glm::vec3 Gold      (0.95f, 0.78f, 0.18f);  // gold crest / treasure
    inline const glm::vec3 SnowCap   (0.85f, 0.92f, 1.00f);  // mountain ice caps (icy blue-white)
    inline const glm::vec3 IceRim    (0.65f, 0.82f, 1.00f);  // icier mid-tone for the cap base
    inline const glm::vec3 Mountain  (0.40f, 0.38f, 0.42f);  // distant mountain body
    inline const glm::vec3 Cloud     (0.96f, 0.97f, 1.00f);  // ring cloud puffs around mountain tops
    inline const glm::vec3 Valley    (0.32f, 0.45f, 0.22f);  // distant valley floor
    inline const glm::vec3 NightSky  (0.05f, 0.06f, 0.15f);  // night sky
    inline const glm::vec3 Arrow     (0.20f, 0.15f, 0.08f);  // wooden arrow shaft
    inline const glm::vec3 Shield    (0.55f, 0.45f, 0.15f);  // brass/wood shield face
    inline const glm::vec3 Bird      (0.10f, 0.08f, 0.08f);  // bird silhouette
    inline const glm::vec3 Sun       (1.00f, 0.92f, 0.40f);  // bright sun sphere / corona
    inline const glm::vec3 Moon      (0.88f, 0.92f, 1.00f);  // cool night moon sphere

}

#endif
