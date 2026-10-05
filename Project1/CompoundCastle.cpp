#include "CompoundCastle.h"
#include "Crenellation.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <algorithm>
#include <cmath>

using glm::vec3;

// Phase 9: damage-per-hit tuning for curtain-wall segments.  A direct
// cannon-ball hit strips 10 % of a segment's health; at 0 the segment
// is removed and its solidBox is dropped so cannonballs fly through
// the gap.  Each hit rebuilds the segment's mesh at a darker shade so
// cumulative damage is visible.
static const float kWallSegmentDamagePerHit = 0.10f;

static glm::vec3 WallSegmentDamageColour(float health) {
    float t = glm::clamp(1.0f - health, 0.0f, 1.0f);
    return Palette::Stone * (1.0f - t) + Palette::StoneDark * t;
}

CompoundCastle::CompoundCastle(glm::vec3 centreWorld,
                               float gateWidth,
                               float wallHeight,
                               float towerHeight,
                               float curtainLength,
                               float compoundHalfX,
                               float compoundHalfZ,
                               float cornerSide,
                               float cornerHeight)
    // The Phase-4 Castle uses 3.75 m offsets between gate and flanking
    // towers (FortGate halfWidth 2.5 + Tower halfSide 1.25).  We place
    // the gatehouse on the WEST face of the compound (x = centreWorld.x
    // - compoundHalfX), and the flanking towers at z = ±3.75.
    : doors(glm::vec3(centreWorld.x - compoundHalfX, 0.0f, centreWorld.z),
            wallHeight, /*doorWidth=*/4.0f, /*panelDepth=*/0.10f, /*plankCount=*/3),

      gate(glm::vec3(centreWorld.x - compoundHalfX, 0.0f, centreWorld.z),
           /*width=*/5.0f, /*height=*/wallHeight, /*depth=*/0.8f,
           /*gateWidth=*/gateWidth, /*rows=*/4),

      gatehouseTower (glm::vec3(centreWorld.x - compoundHalfX, 0.0f,
                                centreWorld.z - 3.75f),
                      /*side=*/2.5f, /*bodyH=*/towerHeight,
                      /*parapetH=*/0.6f, /*merlonW=*/0.4f,
                      /*gap=*/0.4f, /*flagpoleH=*/1.5f),
      gatehouseTower2(glm::vec3(centreWorld.x - compoundHalfX, 0.0f,
                                centreWorld.z + 3.75f),
                      2.5f, towerHeight, 0.6f, 0.4f, 0.4f, 1.5f),

      // Four corner towers at the four corners of the 24x24 m compound.
      // Compound corners: (±12, 0, ±12) relative to centreWorld.
      cornerNW(glm::vec3(centreWorld.x - compoundHalfX, 0.0f,
                         centreWorld.z - compoundHalfZ),
               cornerSide, cornerHeight, /*parapetH=*/0.7f,
               /*merlonW=*/0.5f, /*gap=*/0.4f, /*flagpoleH=*/1.8f),
      cornerNE(glm::vec3(centreWorld.x + compoundHalfX, 0.0f,
                         centreWorld.z - compoundHalfZ),
               cornerSide, cornerHeight, 0.7f, 0.5f, 0.4f, 1.8f),
      cornerSW(glm::vec3(centreWorld.x - compoundHalfX, 0.0f,
                         centreWorld.z + compoundHalfZ),
               cornerSide, cornerHeight, 0.7f, 0.5f, 0.4f, 1.8f),
      cornerSE(glm::vec3(centreWorld.x + compoundHalfX, 0.0f,
                         centreWorld.z + compoundHalfZ),
               cornerSide, cornerHeight, 0.7f, 0.5f, 0.4f, 1.8f),

      // Moat: a 6 m wide strip of water on the -X side of the
      // compound (where the cannons live).  Centred at
      // (centreWorld.x - compoundHalfX - 4, 0, centreWorld.z).
      // Now that the gatehouse towers sit on the curtain wall line (x=0),
      // the moat starts just past the towers (x = -1).
      // Phase 6: widened along Z to merge seamlessly with the wide
      // river that runs across the rest of the scene.
      moat(glm::vec3(centreWorld.x - compoundHalfX - 4.0f, 0.0f,
                     centreWorld.z),
           /*sizeX=*/6.0f, /*sizeZ=*/24.0f),

      // Bridge: spans the full moat width (6 m).  Centred at the
      // moat centre so it hinges from the castle side and drops onto
      // the cannon-side bank.  Phase 8: the hinge is at the CASTLE
      // side (x = centre + 3 = -1) and the OUTER end at the cannon
      // side (x = centre - 3 = -7).  When raised 90 deg the outer
      // end swings straight up to y = lengthX = 6, sitting just past
      // the gatehouse towers (which is the "other side" of the river
      // the user is asking about).
      bridge(glm::vec3(centreWorld.x - compoundHalfX - 4.0f, 0.0f,
                       centreWorld.z),
             /*lengthX=*/6.0f, /*widthZ=*/3.0f)
{
    // ----- curtain walls --------------------------------------------------
    // The compound is `2 * compoundHalfX` long along X and `2 *
    // compoundHalfZ` long along Z.  Each curtain wall has an OUTER
    // stone body (Phase 9: breakable), a CORRIDOR floor slab (always
    // present, non-breakable), and an INNER stone body (Phase 9:
    // breakable).  Merlons on top are also non-breakable.
    //
    // The gatehouse sits on the WEST (-X) face, so the western curtain
    // wall (x = -compoundHalfX) is split into two pieces straddling the
    // gatehouse footprint: from z = -compoundHalfZ to z = -3.75, and
    // from z = +3.75 to z = +compoundHalfZ.
    const float curtainBodyH = wallHeight;
    const float merlonH = 0.50f;
    const float merlonW = 0.45f;
    const float gap = 0.45f;
    const float merlonD = 1.0f;                  // matches outer body depth
    const float merlonY = wallHeight;
    const float outerWallW = 1.0f;               // outer stone body depth
    const float corridorW  = 1.0f;               // walkable corridor width
    const float innerWallW = 1.0f;               // inner stone body depth
    const float corridorT  = 0.10f;              // corridor slab thickness
    const float corridorY  = wallHeight - corridorT;

    // Local helper: push a breakable stone segment + remember its
    // solidBox.  `size` is the full box size; `localMid` is the world
    // centre of the segment.
    auto addSegment = [&](WallSet& ws, glm::vec3 size, glm::vec3 localMid,
                          glm::vec3 half) {
        ws.segments.push_back({
            Primitives::CreateBox(size.x, size.y, size.z, Palette::Stone),
            glm::translate(glm::mat4(1.0f), localMid),
            localMid,
            half,
            size,
            true,
            1.0f
        });
        solidBoxes.push_back({ localMid, half });
    };
    // Local helper: push a non-breakable decor piece (corridor slab
    // or merlon).
    auto addDecor = [&](WallSet& ws, Mesh mesh, glm::mat4 local) {
        ws.decor.push_back({ mesh, local });
    };

    // ---- North curtain (-Z side): x ∈ [-compoundHalfX, +compoundHalfX] --
    // Outer face stays at cz - 0.7 (matches the old single-wall face).
    {
        const float lengthX = 2.0f * compoundHalfX;
        const float cx = centreWorld.x;
        const float cz = centreWorld.z - compoundHalfZ;
        const float outerFaceZ = cz - 0.7f;
        const float outerMidZ  = outerFaceZ + outerWallW * 0.5f;
        const float corridorMidZ = outerFaceZ + outerWallW + corridorW * 0.5f;
        const float innerMidZ  = outerFaceZ + outerWallW + corridorW + innerWallW * 0.5f;
        // Outer body (breakable).
        addSegment(curtainN,
                   vec3(lengthX, curtainBodyH, outerWallW),
                   vec3(cx, curtainBodyH * 0.5f, outerMidZ),
                   vec3(lengthX * 0.5f, curtainBodyH * 0.5f, outerWallW * 0.5f));
        // Corridor slab (non-breakable).
        addDecor(curtainN,
                 Primitives::CreateBox(lengthX, corridorT, corridorW,
                                       Palette::Stone),
                 glm::translate(glm::mat4(1.0f),
                     glm::vec3(cx, corridorY, corridorMidZ)));
        // Inner body (breakable).
        addSegment(curtainN,
                   vec3(lengthX, curtainBodyH, innerWallW),
                   vec3(cx, curtainBodyH * 0.5f, innerMidZ),
                   vec3(lengthX * 0.5f, curtainBodyH * 0.5f, innerWallW * 0.5f));
        auto merlons = Crenellation::AlongX(
            cx - lengthX * 0.5f, merlonY, outerMidZ - merlonD * 0.5f,
            lengthX, merlonW, merlonH, merlonD, gap, Palette::Stone);
        for (auto& p : merlons) addDecor(curtainN, p.mesh, p.local);
    }

    // ---- South curtain (+Z side): same idea ------------------------------
    {
        const float lengthX = 2.0f * compoundHalfX;
        const float cx = centreWorld.x;
        const float cz = centreWorld.z + compoundHalfZ;
        const float outerFaceZ = cz + 0.7f;
        const float outerMidZ  = outerFaceZ - outerWallW * 0.5f;
        const float corridorMidZ = outerFaceZ - outerWallW - corridorW * 0.5f;
        const float innerMidZ  = outerFaceZ - outerWallW - corridorW - innerWallW * 0.5f;
        addSegment(curtainS,
                   vec3(lengthX, curtainBodyH, outerWallW),
                   vec3(cx, curtainBodyH * 0.5f, outerMidZ),
                   vec3(lengthX * 0.5f, curtainBodyH * 0.5f, outerWallW * 0.5f));
        addDecor(curtainS,
                 Primitives::CreateBox(lengthX, corridorT, corridorW,
                                       Palette::Stone),
                 glm::translate(glm::mat4(1.0f),
                     glm::vec3(cx, corridorY, corridorMidZ)));
        addSegment(curtainS,
                   vec3(lengthX, curtainBodyH, innerWallW),
                   vec3(cx, curtainBodyH * 0.5f, innerMidZ),
                   vec3(lengthX * 0.5f, curtainBodyH * 0.5f, innerWallW * 0.5f));
        auto merlons = Crenellation::AlongX(
            cx - lengthX * 0.5f, merlonY, outerMidZ + merlonD * 0.5f,
            lengthX, merlonW, merlonH, merlonD, gap, Palette::Stone);
        for (auto& p : merlons) addDecor(curtainS, p.mesh, p.local);
    }

    // ---- West curtain (-X side), split around the gatehouse ------------
    // Two pieces: from z = -compoundHalfZ to z = -3.75 (length =
    // compoundHalfZ - 3.75), and z = +3.75 to z = +compoundHalfZ (same
    // length).  These run along Z, so they use Crenellation::AlongZ.
    {
        const float splitLength = compoundHalfZ - 3.75f;
        const float cx = centreWorld.x - compoundHalfX;
        const float outerFaceX = cx - 0.7f;
        const float outerMidX  = outerFaceX + outerWallW * 0.5f;
        const float corridorMidX = outerFaceX + outerWallW + corridorW * 0.5f;
        const float innerMidX  = outerFaceX + outerWallW + corridorW + innerWallW * 0.5f;
        // North piece (z = -compoundHalfZ to z = -3.75)
        {
            const float zStart = centreWorld.z - compoundHalfZ;
            const float czMid  = zStart + splitLength * 0.5f;
            addSegment(curtainW,
                       vec3(outerWallW, curtainBodyH, splitLength),
                       vec3(outerMidX, curtainBodyH * 0.5f, czMid),
                       vec3(outerWallW * 0.5f, curtainBodyH * 0.5f, splitLength * 0.5f));
            addDecor(curtainW,
                     Primitives::CreateBox(corridorW, corridorT, splitLength,
                                           Palette::Stone),
                     glm::translate(glm::mat4(1.0f),
                         glm::vec3(corridorMidX, corridorY, czMid)));
            addSegment(curtainW,
                       vec3(innerWallW, curtainBodyH, splitLength),
                       vec3(innerMidX, curtainBodyH * 0.5f, czMid),
                       vec3(innerWallW * 0.5f, curtainBodyH * 0.5f, splitLength * 0.5f));
            auto merlons = Crenellation::AlongZ(
                outerMidX - merlonD * 0.5f, merlonY, zStart,
                splitLength, merlonW, merlonH, merlonD, gap, Palette::Stone);
            for (auto& p : merlons) addDecor(curtainW, p.mesh, p.local);
        }
        // South piece (z = +3.75 to z = +compoundHalfZ)
        {
            const float zStart = centreWorld.z + 3.75f;
            const float czMid  = zStart + splitLength * 0.5f;
            addSegment(curtainW,
                       vec3(outerWallW, curtainBodyH, splitLength),
                       vec3(outerMidX, curtainBodyH * 0.5f, czMid),
                       vec3(outerWallW * 0.5f, curtainBodyH * 0.5f, splitLength * 0.5f));
            addDecor(curtainW,
                     Primitives::CreateBox(corridorW, corridorT, splitLength,
                                           Palette::Stone),
                     glm::translate(glm::mat4(1.0f),
                         glm::vec3(corridorMidX, corridorY, czMid)));
            addSegment(curtainW,
                       vec3(innerWallW, curtainBodyH, splitLength),
                       vec3(innerMidX, curtainBodyH * 0.5f, czMid),
                       vec3(innerWallW * 0.5f, curtainBodyH * 0.5f, splitLength * 0.5f));
            auto merlons = Crenellation::AlongZ(
                outerMidX - merlonD * 0.5f, merlonY, zStart,
                splitLength, merlonW, merlonH, merlonD, gap, Palette::Stone);
            for (auto& p : merlons) addDecor(curtainW, p.mesh, p.local);
        }
    }

    // ---- East curtain (+X side), full length (no gatehouse here) -------
    {
        const float lengthZ = 2.0f * compoundHalfZ;
        const float cx = centreWorld.x + compoundHalfX;
        const float cz = centreWorld.z;
        const float outerFaceX = cx + 0.7f;
        const float outerMidX  = outerFaceX - outerWallW * 0.5f;
        const float corridorMidX = outerFaceX - outerWallW - corridorW * 0.5f;
        const float innerMidX  = outerFaceX - outerWallW - corridorW - innerWallW * 0.5f;
        addSegment(curtainE,
                   vec3(outerWallW, curtainBodyH, lengthZ),
                   vec3(outerMidX, curtainBodyH * 0.5f, cz),
                   vec3(outerWallW * 0.5f, curtainBodyH * 0.5f, lengthZ * 0.5f));
        addDecor(curtainE,
                 Primitives::CreateBox(corridorW, corridorT, lengthZ,
                                       Palette::Stone),
                 glm::translate(glm::mat4(1.0f),
                     glm::vec3(corridorMidX, corridorY, cz)));
        addSegment(curtainE,
                   vec3(innerWallW, curtainBodyH, lengthZ),
                   vec3(innerMidX, curtainBodyH * 0.5f, cz),
                   vec3(innerWallW * 0.5f, curtainBodyH * 0.5f, lengthZ * 0.5f));
        auto merlons = Crenellation::AlongZ(
            outerMidX + merlonD * 0.5f, merlonY, cz - lengthZ * 0.5f,
            lengthZ, merlonW, merlonH, merlonD, gap, Palette::Stone);
        for (auto& p : merlons) addDecor(curtainE, p.mesh, p.local);
    }

    // ---- Phase 8: corner + gatehouse tower AABBs -----------------------
    // Each corner tower is a (side x bodyH x side) box centred at its
    // own world (x, bodyH/2, z).  Two gatehouse flanking towers are
    // (2.5 x towerHeight x 2.5).  Add an AABB for each so cannonballs
    // collide with them too (not just the curtain walls).
    const float cSide = 4.0f;
    const float cBodyH = 8.0f;
    const float ghSide = 2.5f;
    const float ghBodyH = 5.0f;
    auto pushTowerAABB = [&](float x, float z, float side, float bodyH) {
        solidBoxes.push_back({
            glm::vec3(x, bodyH * 0.5f, z),
            glm::vec3(side * 0.5f, bodyH * 0.5f, side * 0.5f)
        });
    };
    pushTowerAABB(centreWorld.x - compoundHalfX, centreWorld.z - compoundHalfZ, cSide, cBodyH);
    pushTowerAABB(centreWorld.x + compoundHalfX, centreWorld.z - compoundHalfZ, cSide, cBodyH);
    pushTowerAABB(centreWorld.x - compoundHalfX, centreWorld.z + compoundHalfZ, cSide, cBodyH);
    pushTowerAABB(centreWorld.x + compoundHalfX, centreWorld.z + compoundHalfZ, cSide, cBodyH);
    pushTowerAABB(centreWorld.x - compoundHalfX, centreWorld.z - 3.75f, ghSide, ghBodyH);
    pushTowerAABB(centreWorld.x - compoundHalfX, centreWorld.z + 3.75f, ghSide, ghBodyH);
}

// Phase 9: damage every alive wall segment that the sphere touches.
// Each hit strips kWallSegmentDamagePerHit health; the segment's mesh
// is rebuilt at a darker shade.  At 0 the segment is removed and its
// solidBox entry is dropped so future cannonballs fly through.
bool CompoundCastle::CheckWallSet(WallSet& ws, glm::vec3 sphereCentre, float sphereRadius) {
    bool anyKilled = false;
    for (WallSegment& s : ws.segments) {
        if (!s.alive) continue;
        glm::vec3 d(
            sphereCentre.x - std::fmax(s.centre.x - s.half.x, std::fmin(sphereCentre.x, s.centre.x + s.half.x)),
            sphereCentre.y - std::fmax(s.centre.y - s.half.y, std::fmin(sphereCentre.y, s.centre.y + s.half.y)),
            sphereCentre.z - std::fmax(s.centre.z - s.half.z, std::fmin(sphereCentre.z, s.centre.z + s.half.z))
        );
        if (glm::dot(d, d) > sphereRadius * sphereRadius) continue;
        // Hit!
        s.health -= kWallSegmentDamagePerHit;
        // Discard old mesh, build a new one at the new colour.
        s.mesh.Delete();
        glm::vec3 newColour = WallSegmentDamageColour(s.health);
        s.mesh = Primitives::CreateBox(s.size.x, s.size.y, s.size.z, newColour);
        if (s.health <= 0.0f) {
            s.alive = false;
            // Free the mesh; the Draw loop ignores it from now on.
            s.mesh.Delete();
            // Drop the segment's solidBox so cannonballs fly through
            // the gap.  We identify it by matching centre + half.
            auto it = std::find_if(solidBoxes.begin(), solidBoxes.end(),
                [&](const SolidBox& b) {
                    return b.centre == s.centre && b.half == s.half;
                });
            if (it != solidBoxes.end()) solidBoxes.erase(it);
            anyKilled = true;
        }
    }
    return anyKilled;
}

bool CompoundCastle::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
    bool any = false;
    if (doors.CheckHit(sphereCentre, sphereRadius)) any = true;
    if (gate.CheckHit (sphereCentre, sphereRadius)) any = true;
    if (CheckWallSet(curtainN, sphereCentre, sphereRadius)) any = true;
    if (CheckWallSet(curtainS, sphereCentre, sphereRadius)) any = true;
    if (CheckWallSet(curtainW, sphereCentre, sphereRadius)) any = true;
    if (CheckWallSet(curtainE, sphereCentre, sphereRadius)) any = true;
    return any;
}

bool CompoundCastle::HitsStatic(glm::vec3 sphereCentre, float sphereRadius) const {
    // Sphere-vs-AABB against every solid wall / tower AABB the castle
    // knows about.  Returns true if the sphere overlaps any of them.  The
    // usual offset - clamp = delta trick: clamp each axis of the sphere
    // centre into the box, then the resulting clamped point is the
    // closest point on the box.  If the distance from that point to the
    // sphere centre is less than the sphere's radius, it's a hit.
    const float r2 = sphereRadius * sphereRadius;
    for (const SolidBox& b : solidBoxes) {
        glm::vec3 delta(
            sphereCentre.x - std::fmax(b.centre.x - b.half.x, std::fmin(sphereCentre.x, b.centre.x + b.half.x)),
            sphereCentre.y - std::fmax(b.centre.y - b.half.y, std::fmin(sphereCentre.y, b.centre.y + b.half.y)),
            sphereCentre.z - std::fmax(b.centre.z - b.half.z, std::fmin(sphereCentre.z, b.centre.z + b.half.z))
        );
        if (glm::dot(delta, delta) <= r2) return true;
    }
    return false;
}

void CompoundCastle::Update(float deltaTime) {
    // The doors have break physics (Phase 5+), the bridge has retract
    // animation (Phase 6).  Wire both updates here so the rest of the
    // scene graph doesn't have to know about either.
    doors.Update(deltaTime);
    bridge.Update(deltaTime);
}

int CompoundCastle::AliveDoorPanelCount() const { return doors.AlivePanelCount(); }
int CompoundCastle::TotalDoorPanelCount() const { return doors.TotalPanelCount(); }
int CompoundCastle::AliveBrickCount() const     { return gate.AliveBrickCount(); }
int CompoundCastle::TotalBrickCount() const     { return gate.TotalBrickCount(); }

int CompoundCastle::AliveWallSegmentCount() const {
    int n = 0;
    auto count = [&](const WallSet& ws) {
        for (const WallSegment& s : ws.segments) if (s.alive) ++n;
    };
    count(curtainN); count(curtainS); count(curtainW); count(curtainE);
    return n;
}
int CompoundCastle::TotalWallSegmentCount() const {
    return (int)curtainN.segments.size() + (int)curtainS.segments.size()
         + (int)curtainW.segments.size() + (int)curtainE.segments.size();
}

void CompoundCastle::DrawWallSet(const WallSet& ws, GLuint modelLoc) {
    // Breakable stone segments first (so the corridor slab + merlons
    // sit on top of them visually).  Mesh::Draw() is non-const, so the
    // loop body has to be able to take a non-const reference - the
    // `WallSet` itself is still const but the segment refs are not.
    for (const WallSegment& s : ws.segments) {
        if (!s.alive) continue;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(s.local));
        // mesh.Draw() needs a non-const Mesh, but Draw() doesn't
        // actually mutate it - so cast away const.
        const_cast<Mesh&>(s.mesh).Draw();
    }
    // Corridor slabs + merlons (always drawn).
    for (const Part& p : ws.decor) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        const_cast<Mesh&>(p.mesh).Draw();
    }
}

void CompoundCastle::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");

    // 4 corner towers first (so they appear behind the gatehouse).
    cornerNW.Draw(shader);
    cornerNE.Draw(shader);
    cornerSW.Draw(shader);
    cornerSE.Draw(shader);

    // 4 curtain walls (outer + inner segments + corridor + merlons).
    DrawWallSet(curtainN, modelLoc);
    DrawWallSet(curtainS, modelLoc);
    DrawWallSet(curtainW, modelLoc);
    DrawWallSet(curtainE, modelLoc);

    // Gatehouse towers, then the brick + lintel, then the doors.
    gatehouseTower.Draw(shader);
    gatehouseTower2.Draw(shader);
    gate.Draw(shader);
    doors.Draw(shader);

    // The moat is drawn AFTER the castle so its surface sits on top of
    // the grass in the moat footprint.  Bridge after moat so the deck
    // occludes the water underneath.
    moat.Draw(shader);
    bridge.Draw(shader);
}

void CompoundCastle::Delete() {
    cornerNW.Delete();
    cornerNE.Delete();
    cornerSW.Delete();
    cornerSE.Delete();
    for (WallSegment& s : curtainN.segments) s.mesh.Delete();
    for (WallSegment& s : curtainS.segments) s.mesh.Delete();
    for (WallSegment& s : curtainW.segments) s.mesh.Delete();
    for (WallSegment& s : curtainE.segments) s.mesh.Delete();
    for (Part& p : curtainN.decor) p.mesh.Delete();
    for (Part& p : curtainS.decor) p.mesh.Delete();
    for (Part& p : curtainW.decor) p.mesh.Delete();
    for (Part& p : curtainE.decor) p.mesh.Delete();
    gatehouseTower.Delete();
    gatehouseTower2.Delete();
    gate.Delete();
    doors.Delete();
    moat.Delete();
    bridge.Delete();
}