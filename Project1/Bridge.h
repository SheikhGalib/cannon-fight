#ifndef BRIDGE_H
#define BRIDGE_H

#include <glm/glm.hpp>
#include <vector>
#include "Part.h"
#include "shaderClass.h"

// A flat stone bridge deck crossing the moat, with two low rails down
// the long sides. Phase 6 makes the whole thing retractable: while the
// R key is held the deck rotates up about its inner (castle-side)
// hinge edge, and the chains hanging from the gatehouse towers shorten
// accordingly. Release R and the deck drops back flat.
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

    // Phase 6: hold-to-raise control. Setting raised=true aims for 90
    // degrees, false aims for 0. The actual angle lerps towards the
    // target in Update().
    void SetRaised(bool raised);

    // Per-frame: lerps currentAngleDeg towards targetAngleDeg, and
    // re-derives each Part's local matrix about the hinge so the deck
    // visibly rotates. Chain link positions are also re-derived.
    void Update(float deltaTime);

    // True if the bridge is currently flat (angle below 1 degree).
    bool IsDown() const { return currentAngleDeg < 1.0f; }

    // World-space top-of-the-gatehouse-tower Y at which chains are
    // anchored. Used by the chain rendering math.
    float ChainAnchorY() const { return chainAnchorY; }

    // Hinge in world space (the inner edge of the bridge). Used by
    // the chain anchor math to know where the chains attach on the
    // bridge side.
    glm::vec3 Hinge() const { return hinge; }

    void Draw(Shader& shader);
    void Delete();

private:
    struct AnimatedPart {
        Mesh mesh;
        glm::mat4 restLocal;   // the matrix as built in the constructor
        glm::mat4 local;       // the matrix to draw with this frame
    };

    // Chains: one per rail. Each chain is a list of N links whose
    // positions are recomputed every frame based on the current angle.
    struct Chain {
        std::vector<AnimatedPart> links;
    };
    std::vector<Chain> chains;

    std::vector<AnimatedPart> parts;

    glm::vec3 centre;
    float lengthX;
    float widthZ;

    // Hinge about which the bridge rotates. The inner edge of the
    // bridge is at centre.x + lengthX/2 (the castle side, +X), at
    // ground height. Rotating about this axis swings the bridge UP
    // vertically, the way a real drawbridge does.
    glm::vec3 hinge;

    // Retract state.
    float currentAngleDeg = 0.0f;
    float targetAngleDeg  = 0.0f;

    // Rate of approach to the target, in degrees per second.
    static constexpr float kAngularRateDegPerSec = 60.0f;

    // World-space Y where the chains anchor (top of gatehouse tower).
    // Used by the chain-link update math.
    float chainAnchorY = 5.6f;

    // Number of links per chain. Each chain is drawn as `kChainLinks`
    // small torus tubes strung from the gatehouse tower top down to a
    // point on the inner end of the rail (which moves as the bridge
    // rotates).
    static constexpr int kChainLinks = 12;
};

#endif