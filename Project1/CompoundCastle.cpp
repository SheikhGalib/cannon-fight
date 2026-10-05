#include "CompoundCastle.h"
#include "Crenellation.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

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
    // compoundHalfZ` long along Z.  Each curtain wall has a stone body
    // (one big box) plus a row of merlons on top (Crenellation).
    //
    // The gatehouse sits on the WEST (-X) face, so the western curtain
    // wall (x = -compoundHalfX) is split into two pieces straddling the
    // gatehouse footprint: from z = -compoundHalfZ to z = -3.75, and
    // from z = +3.75 to z = +compoundHalfZ.
    // Phase 7: each curtain wall is now THREE side-by-side pieces
    // (outer body + corridor floor + inner body).  Merlons sit on top
    // of the outer wall only.
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
        curtainN.push_back({
            Primitives::CreateBox(lengthX, curtainBodyH, outerWallW, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(cx, curtainBodyH * 0.5f, outerMidZ))
        });
        curtainN.push_back({
            Primitives::CreateBox(lengthX, corridorT, corridorW, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(cx, corridorY, corridorMidZ))
        });
        curtainN.push_back({
            Primitives::CreateBox(lengthX, curtainBodyH, innerWallW, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(cx, curtainBodyH * 0.5f, innerMidZ))
        });
        // Phase 8: cache AABB so cannonballs can collide with this
        // wall (outer + inner bodies only; the thin corridor floor
        // slab is treated as part of the inner wall).
        solidBoxes.push_back({ glm::vec3(cx, curtainBodyH * 0.5f, outerMidZ),
                                glm::vec3(lengthX * 0.5f, curtainBodyH * 0.5f, outerWallW * 0.5f) });
        solidBoxes.push_back({ glm::vec3(cx, curtainBodyH * 0.5f, innerMidZ),
                                glm::vec3(lengthX * 0.5f, curtainBodyH * 0.5f, innerWallW * 0.5f) });
        auto merlons = Crenellation::AlongX(
            cx - lengthX * 0.5f, merlonY, outerMidZ - merlonD * 0.5f,
            lengthX, merlonW, merlonH, merlonD, gap, Palette::Stone);
        curtainNMerlons = (int)merlons.size();
        for (auto& p : merlons) curtainN.push_back(std::move(p));
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
        curtainS.push_back({
            Primitives::CreateBox(lengthX, curtainBodyH, outerWallW, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(cx, curtainBodyH * 0.5f, outerMidZ))
        });
        curtainS.push_back({
            Primitives::CreateBox(lengthX, corridorT, corridorW, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(cx, corridorY, corridorMidZ))
        });
        curtainS.push_back({
            Primitives::CreateBox(lengthX, curtainBodyH, innerWallW, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(cx, curtainBodyH * 0.5f, innerMidZ))
        });
        solidBoxes.push_back({ glm::vec3(cx, curtainBodyH * 0.5f, outerMidZ),
                                glm::vec3(lengthX * 0.5f, curtainBodyH * 0.5f, outerWallW * 0.5f) });
        solidBoxes.push_back({ glm::vec3(cx, curtainBodyH * 0.5f, innerMidZ),
                                glm::vec3(lengthX * 0.5f, curtainBodyH * 0.5f, innerWallW * 0.5f) });
        auto merlons = Crenellation::AlongX(
            cx - lengthX * 0.5f, merlonY, outerMidZ + merlonD * 0.5f,
            lengthX, merlonW, merlonH, merlonD, gap, Palette::Stone);
        curtainSMerlons = (int)merlons.size();
        for (auto& p : merlons) curtainS.push_back(std::move(p));
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
            curtainW.push_back({
                Primitives::CreateBox(outerWallW, curtainBodyH, splitLength, Palette::Stone),
                glm::translate(glm::mat4(1.0f),
                    glm::vec3(outerMidX, curtainBodyH * 0.5f, czMid))
            });
            curtainW.push_back({
                Primitives::CreateBox(corridorW, corridorT, splitLength, Palette::Stone),
                glm::translate(glm::mat4(1.0f),
                    glm::vec3(corridorMidX, corridorY, czMid))
            });
            curtainW.push_back({
                Primitives::CreateBox(innerWallW, curtainBodyH, splitLength, Palette::Stone),
                glm::translate(glm::mat4(1.0f),
                    glm::vec3(innerMidX, curtainBodyH * 0.5f, czMid))
            });
            solidBoxes.push_back({ glm::vec3(outerMidX, curtainBodyH * 0.5f, czMid),
                                    glm::vec3(outerWallW * 0.5f, curtainBodyH * 0.5f, splitLength * 0.5f) });
            solidBoxes.push_back({ glm::vec3(innerMidX, curtainBodyH * 0.5f, czMid),
                                    glm::vec3(innerWallW * 0.5f, curtainBodyH * 0.5f, splitLength * 0.5f) });
            auto merlons = Crenellation::AlongZ(
                outerMidX - merlonD * 0.5f, merlonY, zStart,
                splitLength, merlonW, merlonH, merlonD, gap, Palette::Stone);
            curtainWMerlons += (int)merlons.size();
            for (auto& p : merlons) curtainW.push_back(std::move(p));
        }
        // South piece (z = +3.75 to z = +compoundHalfZ)
        {
            const float zStart = centreWorld.z + 3.75f;
            const float czMid  = zStart + splitLength * 0.5f;
            curtainW.push_back({
                Primitives::CreateBox(outerWallW, curtainBodyH, splitLength, Palette::Stone),
                glm::translate(glm::mat4(1.0f),
                    glm::vec3(outerMidX, curtainBodyH * 0.5f, czMid))
            });
            curtainW.push_back({
                Primitives::CreateBox(corridorW, corridorT, splitLength, Palette::Stone),
                glm::translate(glm::mat4(1.0f),
                    glm::vec3(corridorMidX, corridorY, czMid))
            });
            curtainW.push_back({
                Primitives::CreateBox(innerWallW, curtainBodyH, splitLength, Palette::Stone),
                glm::translate(glm::mat4(1.0f),
                    glm::vec3(innerMidX, curtainBodyH * 0.5f, czMid))
            });
            solidBoxes.push_back({ glm::vec3(outerMidX, curtainBodyH * 0.5f, czMid),
                                    glm::vec3(outerWallW * 0.5f, curtainBodyH * 0.5f, splitLength * 0.5f) });
            solidBoxes.push_back({ glm::vec3(innerMidX, curtainBodyH * 0.5f, czMid),
                                    glm::vec3(innerWallW * 0.5f, curtainBodyH * 0.5f, splitLength * 0.5f) });
            auto merlons = Crenellation::AlongZ(
                outerMidX - merlonD * 0.5f, merlonY, zStart,
                splitLength, merlonW, merlonH, merlonD, gap, Palette::Stone);
            curtainWMerlons += (int)merlons.size();
            for (auto& p : merlons) curtainW.push_back(std::move(p));
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
        curtainE.push_back({
            Primitives::CreateBox(outerWallW, curtainBodyH, lengthZ, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(outerMidX, curtainBodyH * 0.5f, cz))
        });
        curtainE.push_back({
            Primitives::CreateBox(corridorW, corridorT, lengthZ, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(corridorMidX, corridorY, cz))
        });
        curtainE.push_back({
            Primitives::CreateBox(innerWallW, curtainBodyH, lengthZ, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(innerMidX, curtainBodyH * 0.5f, cz))
        });
        solidBoxes.push_back({ glm::vec3(outerMidX, curtainBodyH * 0.5f, cz),
                                glm::vec3(outerWallW * 0.5f, curtainBodyH * 0.5f, lengthZ * 0.5f) });
        solidBoxes.push_back({ glm::vec3(innerMidX, curtainBodyH * 0.5f, cz),
                                glm::vec3(innerWallW * 0.5f, curtainBodyH * 0.5f, lengthZ * 0.5f) });
        auto merlons = Crenellation::AlongZ(
            outerMidX + merlonD * 0.5f, merlonY, cz - lengthZ * 0.5f,
            lengthZ, merlonW, merlonH, merlonD, gap, Palette::Stone);
        curtainEMerlons = (int)merlons.size();
        for (auto& p : merlons) curtainE.push_back(std::move(p));
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

bool CompoundCastle::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
    bool any = false;
    if (doors.CheckHit(sphereCentre, sphereRadius)) any = true;
    if (gate.CheckHit (sphereCentre, sphereRadius)) any = true;
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

void CompoundCastle::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");

    // 4 corner towers first (so they appear behind the gatehouse).
    cornerNW.Draw(shader);
    cornerNE.Draw(shader);
    cornerSW.Draw(shader);
    cornerSE.Draw(shader);

    // 4 curtain walls + merlons.
    for (Part& p : curtainN) { glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local)); p.mesh.Draw(); }
    for (Part& p : curtainS) { glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local)); p.mesh.Draw(); }
    for (Part& p : curtainW) { glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local)); p.mesh.Draw(); }
    for (Part& p : curtainE) { glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local)); p.mesh.Draw(); }

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
    for (Part& p : curtainN) p.mesh.Delete();
    for (Part& p : curtainS) p.mesh.Delete();
    for (Part& p : curtainW) p.mesh.Delete();
    for (Part& p : curtainE) p.mesh.Delete();
    gatehouseTower.Delete();
    gatehouseTower2.Delete();
    gate.Delete();
    doors.Delete();
    moat.Delete();
    bridge.Delete();
}