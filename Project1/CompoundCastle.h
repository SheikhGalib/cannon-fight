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
#include "Mesh.h"
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
//
// Phase 9: each curtain-wall stone body (outer + inner wall) is now a
// breakable segment with its own health pool.  Cannonballs strip 10 %
// per hit; at 0 the segment is removed.  Towers, the door, the
// FortGate bricks, the corridor floor, the gatehouse towers, the
// pillars and the lintel remain non-breakable (they use the existing
// `HitsStatic` / `solidBoxes` machinery so cannonballs still stop on
// contact rather than pass through).
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

    // Same breakability interface as Phase 4's Castle.  Also damages
    // the breakable curtain-wall segments added in Phase 9.
    bool CheckHit(glm::vec3 sphereCentre, float sphereRadius);

    // Phase 8: sphere-vs-static-AABB collision against every solid
    // (non-breakable) wall / tower. Returns true if the sphere is in
    // contact with any of them.  Cannonballs use this to stop on the
    // stone rather than pass through it.  Includes the still-alive
    // curtain-wall segments so a partially-broken wall still blocks.
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
    // Phase 9: counts of breakable curtain-wall segments.
    int AliveWallSegmentCount() const;
    int TotalWallSegmentCount() const;

    void Draw(Shader& shader);
    void Delete();

private:
    // Phase 9: each breakable curtain-wall stone segment carries its
    // own health.  Lives parallel to the static parts in
    // `curtain{N,S,W,E}` - we keep the merlons / corridor slabs as
    // non-breakable Parts and only the OUTER + INNER stone bodies
    // become segments.
    struct WallSegment {
        Mesh        mesh;
        glm::mat4   local;
        glm::vec3   centre;        // world-space centre (for sphere-vs-AABB)
        glm::vec3   half;          // world-space half-extents
        glm::vec3   size;          // box dimensions (used to rebuild the
                                   // mesh when the colour darkens)
        bool        alive = true;
        float       health = 1.0f;
    };
    struct WallSet {
        std::vector<WallSegment> segments;       // breakable stone bodies
        std::vector<Part>        decor;          // corridor slabs + merlons
    };

    // Phase 9: damage every alive wall segment in `ws` that the
    // sphere touches.  Also drops the dead segment's solidBox so
    // cannonballs can fly through the gap.
    bool CheckWallSet(WallSet& ws, glm::vec3 sphereCentre, float sphereRadius);

    // Phase 9: draw all alive segments + decor for a single wall.
    void DrawWallSet(const WallSet& ws, GLuint modelLoc);

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

    WallSet curtainN;
    WallSet curtainS;
    WallSet curtainW;
    WallSet curtainE;

    // Phase 8: AABBs for every solid wall segment. Used by CheckHit()
    // so cannonballs stop on contact with the stone (instead of
    // passing through). Each entry is (centreX, centreY, centreZ,
    // halfX, halfY, halfZ) in world space.  Phase 9: rebuilt as
    // wall segments die.
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