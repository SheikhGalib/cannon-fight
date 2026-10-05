#include "Castle.h"
#include "Crenellation.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Castle::Castle(glm::vec3 centreWorld,
               float gateWidth,
               float wallHeight,
               float towerHeight,
               float curtainLength)
    // Two breakable doors filling the gate opening. They share the wall
    // height, so the door tops line up with the lintel.
    : doors(centreWorld, wallHeight, gateWidth, /*panelDepth=*/0.10f, /*plankCount=*/3),

      // The Phase-3 FortGate: two brick stacks flanking the doorway, with
      // a lintel across the top. We give it the same width as before
      // (5.0 m), so the brick stacks + pillars + lintel fill the gap
      // between the two towers.
      gate(centreWorld,
           /*width=*/5.0f, /*height=*/wallHeight, /*depth=*/0.8f,
           /*gateWidth=*/gateWidth, /*rows=*/4),

      // One tower on each side of the gate. The towers' near faces line up
      // with the outer ends of the gate's brick stacks.
      leftTower (glm::vec3(centreWorld.x - 3.75f, 0.0f, centreWorld.z),
                 /*side=*/2.5f, /*bodyH=*/towerHeight,
                 /*parapetH=*/0.6f, /*merlonW=*/0.4f,
                 /*gap=*/0.4f, /*flagpoleH=*/1.5f),
      rightTower(glm::vec3(centreWorld.x + 3.75f, 0.0f, centreWorld.z),
                 2.5f, towerHeight, 0.6f, 0.4f, 0.4f, 1.5f)
{
    // ----- crenellated curtain walls left and right -----------------------
    // The wall starts at the outer face of each tower and extends outward
    // by `curtainLength`. It sits on the ground (y in [0, wallHeight]) and
    // is topped by merlons.
    const float merlonW = 0.35f;
    const float gap     = 0.35f;
    const float merlonH = 0.45f;
    const float merlonD = 0.60f;

    // Left curtain: extends from x = centreWorld.x - 3.75 - 1.25 (outer face
    // of left tower) to x = centreWorld.x - 5.0 (i.e. its right end is at
    // the outer face of the tower). Length is `curtainLength`.
    const float curtainBaseY = wallHeight;
    const float curtainYTop  = wallHeight + merlonH;

    auto leftMerlons = Crenellation::AlongX(
        /*baseX=*/centreWorld.x - 3.75f - 1.25f,  // outer face of left tower
        /*baseY=*/curtainBaseY,
        /*baseZ=*/centreWorld.z - merlonD * 0.5f,
        /*lengthX=*/curtainLength,
        merlonW, merlonH, merlonD, gap, Palette::Stone);
    for (auto& p : leftMerlons) leftCurtain.push_back(std::move(p));

    // The wall body itself (the solid part below the merlons).
    leftCurtain.push_back({
        Primitives::CreateBox(curtainLength, wallHeight, merlonD, Palette::Stone),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(centreWorld.x - 3.75f - 1.25f + curtainLength * 0.5f,
                      wallHeight * 0.5f, centreWorld.z))
    });

    // Right curtain: mirror.
    auto rightMerlons = Crenellation::AlongX(
        centreWorld.x + 3.75f + 1.25f - curtainLength,
        curtainBaseY,
        centreWorld.z - merlonD * 0.5f,
        curtainLength,
        merlonW, merlonH, merlonD, gap, Palette::Stone);
    for (auto& p : rightMerlons) rightCurtain.push_back(std::move(p));

    rightCurtain.push_back({
        Primitives::CreateBox(curtainLength, wallHeight, merlonD, Palette::Stone),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(centreWorld.x + 3.75f + 1.25f - curtainLength * 0.5f,
                      wallHeight * 0.5f, centreWorld.z))
    });

    // ----- decorative drawbridge in front of the door -------------------
    // A short wooden bridge laid in front of the gate, on the side the
    // cannon is on. We pick +Z since the gate is centred on z = 0 and the
    // cannons are at z = 0 too. The bridge is `gateWidth` wide, 0.15 m
    // thick, and 1.5 m deep, sitting just in front of the door.
    const float bridgeW = gateWidth;
    const float bridgeD = 1.5f;
    const float bridgeH = 0.15f;
    const float bridgeY = bridgeH * 0.5f;     // sits on the ground
    const float bridgeZ = centreWorld.z + 0.4f + bridgeD * 0.5f;   // 0.4 = gate's half-depth

    drawbridge.push_back({
        Primitives::CreateBox(bridgeW, bridgeH, bridgeD, Palette::Wood),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(centreWorld.x, bridgeY, bridgeZ))
    });

    // 4 thin wooden plank strips on top of the bridge (visual).
    for (int i = 0; i < 4; i++) {
        const float t = (float(i) + 0.5f) / 4.0f;
        const float px = centreWorld.x - bridgeW * 0.5f + t * bridgeW;
        drawbridge.push_back({
            Primitives::CreateBox(bridgeW / 6.0f, 0.04f, bridgeD * 0.92f, Palette::WoodLight),
            glm::translate(glm::mat4(1.0f),
                glm::vec3(px, bridgeH + 0.02f, bridgeZ))
        });
    }
}

bool Castle::CheckHit(glm::vec3 sphereCentre, float sphereRadius) {
    bool any = false;
    if (doors.CheckHit(sphereCentre, sphereRadius)) any = true;
    if (gate.CheckHit (sphereCentre, sphereRadius)) any = true;
    return any;
}

int Castle::AliveDoorPanelCount() const { return doors.AlivePanelCount(); }
int Castle::TotalDoorPanelCount() const { return doors.TotalPanelCount(); }
int Castle::AliveBrickCount() const     { return gate.AliveBrickCount(); }
int Castle::TotalBrickCount() const     { return gate.TotalBrickCount(); }

void Castle::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");

    // Towers
    leftTower.Draw(shader);
    rightTower.Draw(shader);

    // Curtain walls + merlons (left and right of the gate)
    for (Part& p : leftCurtain) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        p.mesh.Draw();
    }
    for (Part& p : rightCurtain) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        p.mesh.Draw();
    }

    // The Phase-3 gate (brick stacks + lintel + pillars)
    gate.Draw(shader);

    // The doors (drawn AFTER the gate so the planks sit in front of the
    // door panels, and the doors sit in front of the brick stacks)
    doors.Draw(shader);

    // Decorative drawbridge
    for (Part& p : drawbridge) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        p.mesh.Draw();
    }
}

void Castle::Delete() {
    leftTower.Delete();
    rightTower.Delete();
    for (Part& p : leftCurtain)  p.mesh.Delete();
    for (Part& p : rightCurtain) p.mesh.Delete();
    gate.Delete();
    doors.Delete();
    for (Part& p : drawbridge) p.mesh.Delete();
}