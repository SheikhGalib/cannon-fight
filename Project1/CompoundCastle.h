#ifndef COMPOUNDCASTLE_H
#define COMPOUNDCASTLE_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"
#include "Door.h"
#include "FortGate.h"
#include "Tower.h"
#include "CornerTower.h"
#include "Water.h"
#include "Bridge.h"
#include "shaderClass.h"

// A full compound castle: 4 corner towers + 4 curtain walls + 1 main
// gatehouse (Phase 4's door + FortGate + flanking towers + drawbridge)
// + a bridge over the moat in front.
//
// Top-down layout, with X = forward (cannon direction):
//
//                  north (-Z)
//       ┌──────┬──────────────────────┬──────┐
//       │ CT3  │      curtain N       │ CT4  │
//       ├──────┴──────────────────────┴──────┤
//       │                                    │
//   w   │           MAIN GATEHOUSE           │   e
//   e   │      [door → bridge → cannons]     │   a
//   s   │           (x=0 plane)              │   s
//   t   │                                    │   t
//       ├──────┬──────────────────────┬──────┤
//       │ CT1  │      curtain S       │ CT2  │
//       └──────┴──────────────────────┴──────┘
//                  south (+Z)
//
// `centreWorld` is the centre of the COMPOUND footprint, NOT the gate.
// The main gatehouse is centred on the -X face of the compound (facing
// the cannons), so its centre is at `(centreWorld.x - halfCompound, 0,
// centreWorld.z)`.
class CompoundCastle {
public:
    CompoundCastle(glm::vec3 centreWorld,
                   float gateWidth      = 2.0f,
                   float wallHeight     = 3.0f,
                   float towerHeight    = 5.0f,
                   float curtainLength  = 6.0f,
                   float compoundHalfX  = 12.0f,   // half the compound's X width
                   float compoundHalfZ  = 12.0f,   // half the compound's Z depth
                   float cornerSide     = 4.0f,
                   float cornerHeight   = 8.0f);

    // Same breakability interface as Phase 4's Castle.
    bool CheckHit(glm::vec3 sphereCentre, float sphereRadius);

    // Phase 8: sphere-vs-static-AABB collision against every solid
    // (non-breakable) wall / tower. Returns true if the sphere is in
    // contact with any of them.  Cannonballs use this to stop on the
    // stone rather than pass through it.
    bool HitsStatic(glm::vec3 sphereCentre, float sphereRadius) const;

    // Phase 5+: advance any detached door panels one frame (integrates
    // angular velocity + linear velocity so they swing outward and fall).
    void Update(float deltaTime);

    // Phase 6: hold-to-raise drawbridge control. Forwards to bridge.
    void SetBridgeRaised(bool raised) { bridge.SetRaised(raised); }
    int AliveDoorPanelCount() const;
    int TotalDoorPanelCount() const;
    int AliveBrickCount() const;
    int TotalBrickCount() const;

    void Draw(Shader& shader);
    void Delete();

private:
    // Main gatehouse pieces (Phase 4's Castle composition, here on the
    // -X face of the compound).
    Door      doors;
    FortGate  gate;
    Tower     gatehouseTower;     // single flanking tower (one of the two)
    Tower     gatehouseTower2;    // other flanking tower

    // 4 corner towers.
    CornerTower cornerNW;          // (-X, -Z)
    CornerTower cornerNE;          // (+X, -Z)
    CornerTower cornerSW;          // (-X, +Z)
    CornerTower cornerSE;          // (+X, +Z)

    // Curtain walls: each is one big box + a top merlon row.
    std::vector<Part> curtainN;
    std::vector<Part> curtainS;
    std::vector<Part> curtainW;
    std::vector<Part> curtainE;

    // Curtain-wall merlon counts (cached so DrawPartRange-style helpers
    // aren't needed - each wall is just drawn wholesale).
    int curtainNMerlons = 0;
    int curtainSMerlons = 0;
    int curtainWMerlons = 0;
    int curtainEMerlons = 0;

    // Phase 8: AABBs for every solid wall segment. Used by CheckHit()
    // so cannonballs stop on contact with the stone (instead of
    // passing through). Each entry is (centreX, centreY, centreZ,
    // halfX, halfY, halfZ) in world space.
    struct SolidBox {
        glm::vec3 centre;
        glm::vec3 half;
    };
    std::vector<SolidBox> solidBoxes;

    // Moat water + bridge.
    Water  moat;
    Bridge bridge;
};

#endif