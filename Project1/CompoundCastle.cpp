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
            wallHeight, gateWidth, /*panelDepth=*/0.10f, /*plankCount=*/3),

      gate(glm::vec3(centreWorld.x - compoundHalfX, 0.0f, centreWorld.z),
           /*width=*/5.0f, /*height=*/wallHeight, /*depth=*/0.8f,
           /*gateWidth=*/gateWidth, /*rows=*/4),

      gatehouseTower (glm::vec3(centreWorld.x - compoundHalfX - 3.75f, 0.0f,
                                centreWorld.z - 3.75f),
                      /*side=*/2.5f, /*bodyH=*/towerHeight,
                      /*parapetH=*/0.6f, /*merlonW=*/0.4f,
                      /*gap=*/0.4f, /*flagpoleH=*/1.5f),
      gatehouseTower2(glm::vec3(centreWorld.x - compoundHalfX - 3.75f, 0.0f,
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

      // Moat: a 6 m wide, 12 m long strip of water on the -X side of
      // the compound (where the cannons live), centred at
      // (centreWorld.x - compoundHalfX - 4, 0, centreWorld.z).
      moat(glm::vec3(centreWorld.x - compoundHalfX - 4.0f, 0.0f,
                     centreWorld.z),
           /*sizeX=*/6.0f, /*sizeZ=*/12.0f),

      // Bridge: spans the moat from x = compoundHalfX (just outside the
      // gate) to x = -4 (the cannon-side bank), at the centre Z of the
      // compound.
      bridge(glm::vec3(centreWorld.x - compoundHalfX - 3.0f, 0.0f,
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
    const float curtainBodyH = wallHeight;
    const float merlonH = 0.50f;
    const float merlonW = 0.45f;
    const float gap = 0.45f;
    const float merlonD = 0.70f;
    const float merlonY = wallHeight;

    // ---- North curtain (-Z side): x ∈ [-compoundHalfX, +compoundHalfX] --
    {
        const float lengthX = 2.0f * compoundHalfX;
        const float cx = centreWorld.x;
        const float cz = centreWorld.z - compoundHalfZ;
        curtainN.push_back({
            Primitives::CreateBox(lengthX, curtainBodyH, merlonD, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(cx, curtainBodyH * 0.5f, cz))
        });
        auto merlons = Crenellation::AlongX(
            cx - lengthX * 0.5f, merlonY, cz - merlonD * 0.5f,
            lengthX, merlonW, merlonH, merlonD, gap, Palette::Stone);
        curtainNMerlons = (int)merlons.size();
        for (auto& p : merlons) curtainN.push_back(std::move(p));
    }

    // ---- South curtain (+Z side): same idea ------------------------------
    {
        const float lengthX = 2.0f * compoundHalfX;
        const float cx = centreWorld.x;
        const float cz = centreWorld.z + compoundHalfZ;
        curtainS.push_back({
            Primitives::CreateBox(lengthX, curtainBodyH, merlonD, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(cx, curtainBodyH * 0.5f, cz))
        });
        auto merlons = Crenellation::AlongX(
            cx - lengthX * 0.5f, merlonY, cz + merlonD * 0.5f,
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
        // North piece (z = -compoundHalfZ to z = -3.75)
        {
            const float zStart = centreWorld.z - compoundHalfZ;
            const float czMid  = zStart + splitLength * 0.5f;
            curtainW.push_back({
                Primitives::CreateBox(merlonD, curtainBodyH, splitLength, Palette::Stone),
                glm::translate(glm::mat4(1.0f),
                    glm::vec3(cx, curtainBodyH * 0.5f, czMid))
            });
            auto merlons = Crenellation::AlongZ(
                cx - merlonD * 0.5f, merlonY, zStart,
                splitLength, merlonW, merlonH, merlonD, gap, Palette::Stone);
            curtainWMerlons += (int)merlons.size();
            for (auto& p : merlons) curtainW.push_back(std::move(p));
        }
        // South piece (z = +3.75 to z = +compoundHalfZ)
        {
            const float zStart = centreWorld.z + 3.75f;
            const float czMid  = zStart + splitLength * 0.5f;
            curtainW.push_back({
                Primitives::CreateBox(merlonD, curtainBodyH, splitLength, Palette::Stone),
                glm::translate(glm::mat4(1.0f),
                    glm::vec3(cx, curtainBodyH * 0.5f, czMid))
            });
            auto merlons = Crenellation::AlongZ(
                cx - merlonD * 0.5f, merlonY, zStart,
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
        curtainE.push_back({
            Primitives::CreateBox(merlonD, curtainBodyH, lengthZ, Palette::Stone),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(cx, curtainBodyH * 0.5f, cz))
        });
        auto merlons = Crenellation::AlongZ(
            cx + merlonD * 0.5f, merlonY, cz - lengthZ * 0.5f,
            lengthZ, merlonW, merlonH, merlonD, gap, Palette::Stone);
        curtainEMerlons = (int)merlons.size();
        for (auto& p : merlons) curtainE.push_back(std::move(p));
    }
}

bool CompoundCastle::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
    bool any = false;
    if (doors.CheckHit(sphereCentre, sphereRadius)) any = true;
    if (gate.CheckHit (sphereCentre, sphereRadius)) any = true;
    return any;
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