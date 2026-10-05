// CaptureSim.cpp
// ==============
//
// Phase 9: video-capture tool.  Opens a hidden 1280x800 window, runs
// the full Phase 9 scene, automatically starts the battle simulation
// (B), and saves each frame as `frame_NNNNN.bmp` into the `capture/`
// subdirectory next to the executable.
//
// Once the run finishes (the battle sim reaches the End phase), the
// tool closes the window and prints the total frame count.
//
// Build & run by hand:
//   mingw32-make capture
//   ./build_mingw/capture.exe
//
// Combine the frames into a video with the PowerShell helper:
//   powershell -NoProfile -ExecutionPolicy Bypass -File tools/encode_video.ps1
// (Requires ffmpeg on PATH.)

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>
#include <filesystem>
#include <cmath>
#include <random>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaderClass.h"
#include "Primitives.h"
#include "Dimensions.h"
#include "Palette.h"
#include "Carriage.h"
#include "Wheel.h"
#include "Shaft.h"
#include "Cannon.h"
#include "Projectile.h"
#include "FortGate.h"
#include "CompoundCastle.h"
#include "Tower.h"
#include "CornerTower.h"
#include "Door.h"
#include "Robot.h"
#include "Archer.h"
#include "Soldier.h"
#include "Bridge.h"
#include "CampTent.h"
#include "Scenery.h"
#include "GoldCrest.h"
#include "Arrow.h"
#include "SkyClouds.h"
#include "Birds.h"
#include "SignalTower.h"
#include "Tree.h"
#include "Water.h"

using namespace glm;
using namespace std;

static const int CAPTURE_W = 1280;
static const int CAPTURE_H = 800;
static const std::string kFrameDir = "capture";
static const std::string kFramePattern = "capture/frame_";

static void WriteLE32(std::ofstream& f, unsigned int v) {
    f.put(char(v & 0xFF)); f.put(char((v >> 8) & 0xFF));
    f.put(char((v >> 16) & 0xFF)); f.put(char((v >> 24) & 0xFF));
}
static void WriteLE16(std::ofstream& f, unsigned short v) {
    f.put(char(v & 0xFF)); f.put(char((v >> 8) & 0xFF));
}

static bool SaveBMP(const std::string& path, int w, int h, const std::vector<unsigned char>& rgb) {
    const int rowBytes = w * 3;
    const int padding = (4 - (rowBytes % 4)) % 4;
    const unsigned int pixelBytes = (rowBytes + padding) * h;

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.put('B'); f.put('M');
    WriteLE32(f, 14 + 40 + pixelBytes);
    WriteLE32(f, 0);
    WriteLE32(f, 14 + 40);
    WriteLE32(f, 40);
    WriteLE32(f, (unsigned int)w);
    WriteLE32(f, (unsigned int)h);
    WriteLE16(f, 1);
    WriteLE16(f, 24);
    WriteLE32(f, 0); WriteLE32(f, pixelBytes);
    WriteLE32(f, 2835); WriteLE32(f, 2835); WriteLE32(f, 0); WriteLE32(f, 0);

    for (int y = 0; y < h; y++) {
        const unsigned char* row = rgb.data() + size_t(y) * rowBytes;
        for (int x = 0; x < w; x++) {
            f.put(char(row[x * 3 + 2]));
            f.put(char(row[x * 3 + 1]));
            f.put(char(row[x * 3 + 0]));
        }
        for (int p = 0; p < padding; p++) f.put(char(0));
    }
    return true;
}

// Forward-declare the battle sim phases (kept identical to Main.cpp).
enum class TimeOfDay { Day, Night };
enum class BattlePhase { Inactive, BridgeUp, Defending, Advance, Melee, End };
enum class FireState { Idle, CrewWalking, Lighting, CrewReturning };

struct FireSequence {
    FireState state = FireState::Idle;
    float timer = 0.0f;
    Soldier* crew = nullptr;
    vec3 crewRestPos;
    vec3 crewFirePos;
};

int main() {
    namespace fs = std::filesystem;
    if (!fs::exists(kFrameDir)) fs::create_directory(kFrameDir);

    if (!glfwInit()) {
        cerr << "glfwInit failed" << endl;
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(CAPTURE_W, CAPTURE_H, "CaptureSim", NULL, NULL);
    if (!window) {
        cerr << "Failed to create window" << endl;
        glfwTerminate();
        return 2;
    }
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glViewport(0, 0, CAPTURE_W, CAPTURE_H);
    glEnable(GL_DEPTH_TEST);

    Shader shaderProgram("gouraud.vert", "gouraud.frag");

    // ----- Scene (a copy of Main.cpp's main setup) -----
    Mesh ground = Primitives::CreatePlane(400.0f, 400.0f, Palette::Grass);
    Water river(vec3(-4.0f, 0.0f, 0.0f), 12.0f, 480.0f);

    const float cannonX = -15.0f;
    const float cannonSpacing = 2.5f;
    Cannon leftCannon  (vec3(cannonX, 0.0f,  cannonSpacing));
    Cannon centreCannon(vec3(cannonX, 0.0f,  0.0f));
    Cannon rightCannon (vec3(cannonX, 0.0f, -cannonSpacing));
    centreCannon.Elevate(8.0f);

    CompoundCastle castle(vec3(12.0f, 0.0f, 0.0f));

    const float cornerTowerTopY = 8.0f;
    const float gatehouseTowerTopY = 5.0f;
    const float halfCompound = 12.0f;
    std::vector<Archer> archers;
    archers.emplace_back(vec3(12.0f - halfCompound, cornerTowerTopY, -halfCompound));
    archers.emplace_back(vec3(12.0f + halfCompound, cornerTowerTopY, -halfCompound));
    archers.emplace_back(vec3(12.0f - halfCompound, cornerTowerTopY,  halfCompound));
    archers.emplace_back(vec3(12.0f + halfCompound, cornerTowerTopY,  halfCompound));
    archers.emplace_back(vec3(0.0f, gatehouseTowerTopY, -3.75f));
    archers.emplace_back(vec3(0.0f, gatehouseTowerTopY,  3.75f));
    for (Archer& a : archers) a.SetYaw(90.0f);

    std::vector<Soldier> wallSoldiers;
    wallSoldiers.emplace_back(vec3(0.8f, 2.9f, -8.5f), Palette::Defender);
    wallSoldiers.emplace_back(vec3(0.8f, 2.9f, -5.5f), Palette::Defender);
    wallSoldiers.emplace_back(vec3(0.8f, 2.9f, +5.5f), Palette::Defender);
    wallSoldiers.emplace_back(vec3(0.8f, 2.9f, +8.5f), Palette::Defender);
    for (Soldier& s : wallSoldiers) s.SetYaw(180.0f);
    std::vector<bool> wallSoldierAlive(4, true);

    Mesh sunCore   = Primitives::CreateSphere(4.5f, 20, 20, Palette::Sun);
    Mesh sunCorona = Primitives::CreateSphere(7.0f, 16, 16, glm::vec3(1.0f, 0.96f, 0.60f));
    std::vector<Mesh> sunRays;
    for (int r = 0; r < 8; r++) {
        sunRays.push_back(Primitives::CreateCylinder(0.25f, 95.0f, 8, glm::vec3(1.0f, 0.94f, 0.50f), /*centered=*/false));
    }

    const float crewZOffset = -0.7f;
    Soldier crewLeft (vec3(cannonX - 1.8f, 0.0f,  cannonSpacing + crewZOffset), Palette::Attacker);
    Soldier crewCentre(vec3(cannonX - 1.8f, 0.0f,  0.0f + crewZOffset),       Palette::Attacker);
    Soldier crewRight(vec3(cannonX - 1.8f, 0.0f, -cannonSpacing + crewZOffset),Palette::Attacker);

    std::vector<Soldier> army;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 5; col++) {
            float x = cannonX - 6.0f - float(row) * 1.5f;
            float z = (float(col) - 2.0f) * 2.0f;
            army.emplace_back(vec3(x, 0.0f, z), Palette::Attacker);
        }
    }
    for (Soldier& s : army) s.SetYaw(0.0f);
    crewLeft.SetYaw(0.0f); crewCentre.SetYaw(0.0f); crewRight.SetYaw(0.0f);

    const float crestX = 12.0f + 4.0f;
    const float crestZ = 0.0f;
    std::vector<Soldier> defenders;
    for (int i = 0; i < 4; i++) {
        float a = float(i) * 1.7f;
        defenders.emplace_back(
            vec3(crestX + std::cos(a) * 2.0f, 0.0f, crestZ + std::sin(a) * 2.0f),
            Palette::Defender);
        defenders.back().SetYaw(180.0f);
    }

    std::vector<CampTent> tents;
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 3; c++) {
            float tx = cannonX - 6.0f + float(r) * 4.0f;   // x = -21 .. -17
            float tz = -16.0f - float(r) * 6.0f - float(c) * 4.0f;  // z = -28 .. -16
            tents.emplace_back(vec3(tx, 0.0f, tz), 1.5f, 2.2f,
                               Palette::TentCloth, Palette::TentBase);
        }
    }
    GoldCrest goldCrest(vec3(crestX, 0.0f, crestZ));
    Scenery scenery(vec3(0.0f, 0.0f, 0.0f), 110.0f, 200.0f, 40);
    SkyClouds skyClouds(10, 140.0f, 140.0f, 28.0f, 42.0f);
    Birds birds(6, vec3(crestX, 0.0f, crestZ), 50.0f, 70.0f, 110.0f);

    // Phase 9 (extra): riverside signal towers (matches Main.cpp).
    std::vector<SignalTower> signalTowers;
    for (int side = 0; side < 2; side++) {
        float towerX = (side == 0) ? -16.0f : +8.0f;
        for (int i = 0; i < 6; i++) {
            float tz = -150.0f + float(i) * 60.0f;
            if (std::abs(tz) < 15.0f) continue;
            float h = 5.5f + float((i + side) % 3) * 0.8f;
            signalTowers.emplace_back(vec3(towerX, 0.0f, tz), h);
        }
    }
    Robot robot(vec3(3.0f, 0.0f, 0.0f));

    // Trees (Phase 9): scattered in safe zones only.
    std::vector<Tree> trees;
    {
        unsigned int seed = 4711u;
        auto rnd = [&]() {
            seed = seed * 1103515245u + 12345u;
            return float((seed >> 8) & 0xFFFFFFu) / float(0xFFFFFFu);
        };
        const int kTreeCount = 70;
        int placed = 0, attempts = 0;
        while (placed < kTreeCount && attempts < kTreeCount * 10) {
            ++attempts;
            float angle  = rnd() * 6.2831853f;
            float radius = 18.0f + rnd() * 75.0f;
            float x = 12.0f + std::cos(angle) * radius;
            float z = 0.0f  + std::sin(angle) * radius;
            if (x > -22.0f && x < 8.0f && z > -15.0f && z < 15.0f) continue;
            float trunkH = 1.4f + rnd() * 1.0f;
            float trunkR = 0.12f + rnd() * 0.08f;
            float crownH = 1.8f + rnd() * 1.5f;
            float crownR = 0.9f + rnd() * 0.6f;
            trees.emplace_back(vec3(x, 0.0f, z), trunkH, trunkR, crownH, crownR);
            ++placed;
        }
    }

    FireSequence fireLeft, fireCentre, fireRight;
    fireLeft.crew   = &crewLeft;
    fireCentre.crew = &crewCentre;
    fireRight.crew  = &crewRight;
    fireLeft.crewRestPos   = vec3(cannonX - 1.8f, 0.0f,  cannonSpacing + crewZOffset);
    fireCentre.crewRestPos = vec3(cannonX - 1.8f, 0.0f,  0.0f + crewZOffset);
    fireRight.crewRestPos  = vec3(cannonX - 1.8f, 0.0f, -cannonSpacing + crewZOffset);
    fireLeft.crewFirePos   = vec3(cannonX - 0.8f, 0.0f,  cannonSpacing + crewZOffset);
    fireCentre.crewFirePos = vec3(cannonX - 0.8f, 0.0f,  0.0f + crewZOffset);
    fireRight.crewFirePos  = vec3(cannonX - 0.8f, 0.0f, -cannonSpacing + crewZOffset);

    std::vector<bool> archerAlive(archers.size(), true);
    std::vector<bool> defenderAlive(defenders.size(), true);
    std::vector<bool> armyAlive(army.size(), true);
    int nextReplacementDefender = 0;
    float archerFireTimer = 0.0f;
    const float kArcherFirePeriod = 1.6f;
    float cannonAutoFireTimer = 0.0f;
    const float cannonAutoFirePeriod = 1.2f;

    std::vector<vec3> armyOriginalPos(army.size());
    for (size_t i = 0; i < army.size(); i++) armyOriginalPos[i] = army[i].GetPosition();
    std::vector<vec3> defenderOriginalPos(defenders.size());
    for (size_t i = 0; i < defenders.size(); i++) defenderOriginalPos[i] = defenders[i].GetPosition();

    std::vector<int> armyMarchOrder;
    bool armyMarchStarted = false;
    float leadMarchDistance = 0.0f;
    const float kArmyMarchSpeed = 2.8f;
    float meleeTimer = 0.0f;
    float meleeHitTimer = 0.0f;

    std::vector<Arrow> arrows;
    std::vector<Projectile> projectiles;

    BattlePhase battle = BattlePhase::Inactive;
    float battleTimer = 0.0f;
    TimeOfDay tod = TimeOfDay::Day;
    bool battlePaused = false;

    // Camera: lock to a cinematic top-down-ish angle for the video.
    float camYaw   = 0.7f;
    float camPitch = 0.40f;
    float camRadius = 55.0f;
    vec3  camTarget(12.0f, 4.0f, 0.0f);
    auto computeView = [&]() {
        float cy = std::cos(camYaw),  sy = std::sin(camYaw);
        float cp = std::cos(camPitch), sp = std::sin(camPitch);
        // dir points from eye TOWARD target; eye is the opposite
        // side.  Negative dir.y so the camera is above target
        // looking down.
        vec3 dir = vec3(cp * cy, -sp, cp * sy);
        vec3 eye = camTarget - dir * camRadius;
        return lookAt(eye, camTarget, vec3(0.0f, 1.0f, 0.0f));
    };
    mat4 projMatrix = perspective(radians(55.0f),
                                  float(CAPTURE_W) / float(CAPTURE_H),
                                  0.1f, 400.0f);

    GLuint viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram.ID, "proj");
    GLuint modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
    GLuint lightDirLoc = glGetUniformLocation(shaderProgram.ID, "lightDir");

    auto lerpSoldier = [](Soldier& s, const vec3& from, const vec3& to,
                          float timer, float seconds) {
        float t = (seconds <= 0.0f) ? 1.0f : timer / seconds;
        if (t > 1.0f) t = 1.0f;
        s.SetPosition(from * (1.0f - t) + to * t);
    };

    auto tickCannon = [&](Cannon& cannon, FireSequence& seq, float dt) {
        switch (seq.state) {
            case FireState::Idle:
                seq.crew->SetPosition(seq.crewRestPos);
                break;
            case FireState::CrewWalking:
                seq.timer += dt;
                lerpSoldier(*seq.crew, seq.crewRestPos, seq.crewFirePos,
                            seq.timer, 0.5f);
                if (seq.timer >= 0.5f) {
                    seq.state = FireState::Lighting;
                    seq.timer = 0.0f;
                }
                break;
            case FireState::Lighting:
                seq.timer += dt;
                seq.crew->SetPosition(seq.crewFirePos);
                if (seq.timer >= 1.0f) {
                    vec3 muzzle  = cannon.GetMuzzleWorldPosition();
                    vec3 forward = cannon.GetForwardWorldDirection();
                    projectiles.emplace_back(muzzle,
                                              forward * Projectile::DefaultSpeed,
                                              Projectile::DefaultRadius);
                    cannon.Fire();
                    seq.state = FireState::CrewReturning;
                    seq.timer = 0.0f;
                }
                break;
            case FireState::CrewReturning:
                seq.timer += dt;
                lerpSoldier(*seq.crew, seq.crewFirePos, seq.crewRestPos,
                            seq.timer, 0.3f);
                if (seq.timer >= 0.3f) {
                    seq.state = FireState::Idle;
                    seq.timer = 0.0f;
                }
                break;
        }
    };

    // Framebuffer reader buffer.
    std::vector<unsigned char> framebuffer(CAPTURE_W * CAPTURE_H * 3);

    // ----- Run loop -----
    // Start the battle simulation immediately (skip the Inactive pause).
    battle = BattlePhase::BridgeUp;
    battleTimer = 0.0f;
    float dt = 1.0f / 30.0f;        // 30 fps capture - keeps the video small
    const int kMaxFrames = 30 * 60;  // up to 60 seconds of footage
    int frameIdx = 0;
    bool battleFinished = false;
    while (frameIdx < kMaxFrames && !battleFinished) {
        // ---- Input: simulate cannon auto-fire during Advance ----
        if (battle == BattlePhase::Advance) {
            cannonAutoFireTimer += dt;
            if (cannonAutoFireTimer >= cannonAutoFirePeriod) {
                cannonAutoFireTimer = 0.0f;
                Cannon*  cs[3]    = { &leftCannon, &centreCannon, &rightCannon };
                FireSequence* sq[3] = { &fireLeft, &fireCentre, &fireRight };
                for (int i = 0; i < 3; i++) {
                    if (sq[i]->state == FireState::Idle) {
                        sq[i]->state = FireState::CrewWalking;
                        sq[i]->timer = 0.0f;
                        break;
                    }
                }
            }
        }

        // ---- Tick cannons ----
        tickCannon(leftCannon,   fireLeft,   dt);
        tickCannon(centreCannon, fireCentre, dt);
        tickCannon(rightCannon,  fireRight,  dt);
        leftCannon  .Update(dt);
        centreCannon.Update(dt);
        rightCannon .Update(dt);

        // ---- Update projectiles ----
        for (Projectile& ball : projectiles) {
            ball.Update(dt, Projectile::Gravity);
            castle.CheckHit(ball.GetPosition(), ball.GetRadius());
            if (castle.HitsStatic(ball.GetPosition(), ball.GetRadius())) {
                ball.Kill();
            }
        }
        projectiles.erase(
            remove_if(projectiles.begin(), projectiles.end(),
                     [](const Projectile& b) { return b.IsDead(); }),
            projectiles.end());
        castle.Update(dt);

        // ---- Battle sim state machine ----
        if (battle != BattlePhase::Inactive && !battlePaused) {
            battleTimer += dt;
        }
        if (battle == BattlePhase::BridgeUp) {
            castle.SetBridgeRaised(true);
            if (battleTimer > 2.0f) { battle = BattlePhase::Defending; battleTimer = 0.0f; }
        } else if (battle == BattlePhase::Defending) {
            castle.SetBridgeRaised(true);
            int livingAttackers = 0; for (bool a : armyAlive)   if (a) ++livingAttackers;
            int livingArchers   = 0; for (bool a : archerAlive) if (a) ++livingArchers;
            if (battleTimer > 5.0f || livingAttackers <= 0 || livingArchers <= 0) {
                battle = BattlePhase::Advance; battleTimer = 0.0f;
                castle.SetBridgeRaised(false);
            }
        } else if (battle == BattlePhase::Advance) {
            castle.SetBridgeRaised(false);
            bool doorBroken = (castle.AliveDoorPanelCount() == 0);
            if (battleTimer > 20.0f && !doorBroken) {
                doorBroken = true;
            }

            // Trigger the army march in strictly TWO LINES across the drawbridge
            if (doorBroken && !armyMarchStarted) {
                armyMarchStarted = true;
                leadMarchDistance = 0.0f;
                armyMarchOrder.clear();
                for (size_t si = 0; si < army.size(); si++) {
                    if (armyAlive[si]) {
                        armyMarchOrder.push_back((int)si);
                    }
                }
            }

            if (armyMarchStarted) {
                leadMarchDistance += kArmyMarchSpeed * dt;
                float desiredLeadX = -15.0f + leadMarchDistance;

                for (size_t r = 0; r < armyMarchOrder.size(); r++) {
                    int si = armyMarchOrder[r];
                    if (!armyAlive[si]) continue;
                    int file = int(r % 2);       // 0 = left file, 1 = right file
                    int pair = int(r / 2);       // 0, 1, 2, ...
                    float targetZ = (file == 0) ? -0.85f : +0.85f;

                    float desiredX = desiredLeadX - float(pair) * 2.2f;
                    desiredX = std::max(armyOriginalPos[si].x, std::min(desiredX, 7.0f));

                    // Funnel into bridge, then strictly two lines on bridge!
                    float currentZ = targetZ;
                    if (desiredX < -11.5f) {
                        float tZ = (desiredX - armyOriginalPos[si].x) / (-11.5f - armyOriginalPos[si].x);
                        tZ = glm::clamp(tZ, 0.0f, 1.0f);
                        currentZ = glm::mix(armyOriginalPos[si].z, targetZ, tZ);
                    } else if (desiredX > 2.0f) {
                        float fanOut = (desiredX - 2.0f) / 5.0f;
                        currentZ = targetZ * (1.0f + fanOut * 0.8f);
                    }

                    army[si].SetPosition(vec3(desiredX, 0.0f, currentZ));
                    army[si].SetYaw(0.0f);
                }

                if (desiredLeadX >= 5.5f) {
                    battle = BattlePhase::Melee;
                    battleTimer = 0.0f;
                    meleeTimer = 0.0f;
                    meleeHitTimer = 0.0f;
                }
            }
        } else if (battle == BattlePhase::Melee) {
            meleeTimer += dt;

            // Defenders (blue) charge forward from crest toward attackers
            for (size_t di = 0; di < defenders.size(); di++) {
                if (!defenderAlive[di]) continue;
                vec3 cur = defenders[di].GetPosition();
                if (cur.x > 7.5f) {
                    cur.x -= 3.2f * dt;
                    defenders[di].SetPosition(cur);
                    defenders[di].SetYaw(180.0f);
                }
            }

            // Attackers advance to meet defenders
            for (size_t si = 0; si < army.size(); si++) {
                if (!armyAlive[si]) continue;
                vec3 cur = army[si].GetPosition();
                if (cur.x < 6.5f) {
                    cur.x += 2.2f * dt;
                    army[si].SetPosition(cur);
                    army[si].SetYaw(0.0f);
                }
            }

            // Sword strike lunges!
            for (size_t si = 0; si < army.size(); si++) {
                if (armyAlive[si]) {
                    army[si].SetAttackOffset(std::sin(meleeTimer * 12.0f + float(si)) * 0.22f);
                } else {
                    army[si].SetAttackOffset(0.0f);
                }
            }
            for (size_t di = 0; di < defenders.size(); di++) {
                if (defenderAlive[di]) {
                    defenders[di].SetAttackOffset(-std::sin(meleeTimer * 12.0f + float(di) * 1.5f) * 0.22f);
                } else {
                    defenders[di].SetAttackOffset(0.0f);
                }
            }

            // Periodic melee combat damage: soldiers fight and DIE!
            meleeHitTimer += dt;
            if (meleeHitTimer >= 0.85f) {
                meleeHitTimer = 0.0f;
                int livingAttacker = -1;
                for (size_t si = 0; si < army.size(); si++) {
                    if (armyAlive[si]) { livingAttacker = (int)si; break; }
                }
                int livingDefender = -1;
                for (size_t di = 0; di < defenders.size(); di++) {
                    if (defenderAlive[di]) { livingDefender = (int)di; break; }
                }

                if (livingAttacker >= 0 && livingDefender >= 0) {
                    defenders[livingDefender].TakeDamage(35.0f);
                    if (defenders[livingDefender].IsDead()) {
                        defenderAlive[livingDefender] = false;
                        defenders[livingDefender].SetDead();
                    }
                    for (size_t si = livingAttacker; si < army.size(); si++) {
                        if (armyAlive[si]) {
                            army[si].TakeDamage(40.0f);
                            if (army[si].IsDead()) {
                                armyAlive[si] = false;
                                army[si].SetDead();
                            }
                            break;
                        }
                    }
                }
            }

            int livingDefenders = 0;
            for (bool d : defenderAlive) if (d) ++livingDefenders;
            int livingAttackers = 0;
            for (bool a : armyAlive) if (a) ++livingAttackers;

            if (livingDefenders <= 0 || livingAttackers <= 0 || meleeTimer > 8.0f) {
                battle = BattlePhase::End;
                battleTimer = 0.0f;
                bool attackersWin = (livingAttackers > 0);
                goldCrest.SetVictorious(attackersWin);
                for (Soldier& s : army) s.SetAttackOffset(0.0f);
                for (Soldier& d : defenders) d.SetAttackOffset(0.0f);
            }
        } else if (battle == BattlePhase::End) {
            // Surviving attackers advance to surround the gold crest
            for (size_t si = 0; si < army.size(); si++) {
                if (!armyAlive[si]) continue;
                vec3 cur = army[si].GetPosition();
                if (cur.x < 14.5f) {
                    cur.x += 1.5f * dt;
                    army[si].SetPosition(cur);
                }
            }
            if (battleTimer > 6.0f) battleFinished = true;
        }

        // ---- Archers shoot arrows ----
        if (battle == BattlePhase::Defending || battle == BattlePhase::Advance) {
            archerFireTimer += dt;
            if (archerFireTimer >= kArcherFirePeriod) {
                archerFireTimer = 0.0f;
                for (size_t ai = 0; ai < archers.size(); ai++) {
                    if (!archerAlive[ai]) continue;
                    int bestIdx = -1; float bestD2 = 1e9f;
                    vec3 archerPos = archers[ai].GetPosition();
                    for (size_t si = 0; si < army.size(); si++) {
                        if (!armyAlive[si]) continue;
                        vec3 sp = army[si].GetPosition();
                        float d2 = glm::dot(sp - archerPos, sp - archerPos);
                        if (d2 < bestD2) { bestD2 = d2; bestIdx = (int)si; }
                    }
                    if (bestIdx < 0) continue;
                    vec3 target = army[bestIdx].GetPosition();
                    vec3 origin = archerPos + vec3(0.0f, 1.5f, 0.0f);
                    vec3 toT = target - origin;
                    float horiz = length(vec2(toT.x, toT.z));
                    float vy = 5.0f, vh = 16.0f;
                    vec3 vhVec = (horiz > 1e-3f)
                        ? vec3(toT.x, 0.0f, toT.z) / horiz * vh
                        : vec3(0.0f, 0.0f, 0.0f);
                    arrows.emplace_back(origin, vhVec + vec3(0.0f, vy, 0.0f));
                }
            }
        }
        // ---- Arrow + soldier hit detection (soldiers survive 2 hits, die on 3rd) ----
        for (Arrow& a : arrows) {
            a.Update(dt);
            if (a.IsDead()) continue;
            vec3 ap = a.GetPosition();
            for (size_t si = 0; si < army.size(); si++) {
                if (!armyAlive[si]) continue;
                vec3 sp = army[si].GetPosition();
                vec3 d = ap - sp;
                vec3 clamped(
                    std::fmax(-0.25f, std::fmin(d.x, 0.25f)),
                    std::fmax(-1.20f, std::fmin(d.y, 1.20f)),
                    std::fmax(-0.20f, std::fmin(d.z, 0.20f)));
                vec3 delta = d - clamped;
                if (glm::dot(delta, delta) <= 0.05f * 0.05f) {
                    army[si].TakeDamage(35.0f);
                    if (army[si].IsDead()) {
                        armyAlive[si] = false;
                        army[si].SetDead();
                    }
                    a.Kill();
                    break;
                }
            }
        }
        arrows.erase(
            remove_if(arrows.begin(), arrows.end(),
                      [](const Arrow& a) { return a.IsDead(); }),
            arrows.end());

        // Update 2: If front wall segment breaks, front wall soldiers die
        if (!castle.IsFrontWallPieceAlive(0)) {
            wallSoldierAlive[0] = false;
            wallSoldierAlive[1] = false;
        }
        if (!castle.IsFrontWallPieceAlive(1)) {
            wallSoldierAlive[2] = false;
            wallSoldierAlive[3] = false;
        }

        // Update 3: If a tower breaks, the tower archer dies
        for (size_t ai = 0; ai < archers.size(); ai++) {
            if (!castle.IsTowerAlive((int)ai)) {
                archerAlive[ai] = false;
            }
        }

        // Replacement: archer dies on intact tower -> defender walks up to take its place.
        for (size_t ai = 0; ai < archers.size(); ai++) {
            if (archerAlive[ai] || !castle.IsTowerAlive((int)ai)) continue;
            while (nextReplacementDefender < (int)defenders.size() &&
                   !defenderAlive[nextReplacementDefender]) ++nextReplacementDefender;
            if (nextReplacementDefender < (int)defenders.size()) {
                defenders[nextReplacementDefender].SetPosition(archers[ai].GetPosition());
                defenderAlive[nextReplacementDefender] = false;
                archerAlive[ai] = true;
                ++nextReplacementDefender;
            }
        }

        goldCrest.Update(dt);
        skyClouds.Update(dt);
        birds.Update(dt);

        // ---- Render the frame ----
        mat4 view = computeView();
        if (tod == TimeOfDay::Day) {
            glClearColor(0.55f, 0.72f, 0.87f, 1.0f);
        } else {
            glClearColor(Palette::NightSky.r, Palette::NightSky.g,
                         Palette::NightSky.b, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderProgram.Activate();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projMatrix));
        vec3 lightDir = (tod == TimeOfDay::Day)
            ? normalize(vec3(-0.4f, -1.0f, -0.5f))
            : normalize(vec3(0.6f, -0.2f, -0.4f));
        glUniform3fv(lightDirLoc, 1, value_ptr(lightDir));

        // Draw Sun and radiant rays
        GLint isSunLoc = glGetUniformLocation(shaderProgram.ID, "isSun");
        if (isSunLoc != -1) glUniform1i(isSunLoc, 1);
        vec3 lightSourcePos = camTarget + (-lightDir * 120.0f);
        mat4 sunMat = translate(mat4(1.0f), lightSourcePos);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(sunMat));
        sunCore.Draw();

        mat4 coronaMat = translate(mat4(1.0f), lightSourcePos);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(coronaMat));
        sunCorona.Draw();

        for (int r = 0; r < 8; r++) {
            float angle = float(r) * (3.14159265f / 4.0f);
            mat4 rayMat = translate(mat4(1.0f), lightSourcePos)
                        * rotate(mat4(1.0f), angle, vec3(0.0f, 0.0f, 1.0f))
                        * rotate(mat4(1.0f), radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(rayMat));
            sunRays[r].Draw();
        }
        if (isSunLoc != -1) glUniform1i(isSunLoc, 0);

        mat4 groundMatrix = translate(mat4(1.0f), vec3(0.0f, -0.01f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(groundMatrix));
        ground.Draw();
        river.Draw(shaderProgram);
        scenery.Draw(shaderProgram);
        for (SignalTower& t : signalTowers) t.Draw(shaderProgram);
        skyClouds.Draw(shaderProgram);
        castle.Draw(shaderProgram);
        robot.Draw(shaderProgram);
        for (size_t i = 0; i < archers.size(); i++)
            if (archerAlive[i]) archers[i].Draw(shaderProgram);
        for (size_t i = 0; i < wallSoldiers.size(); i++)
            if (wallSoldierAlive[i]) wallSoldiers[i].Draw(shaderProgram);
        for (size_t i = 0; i < army.size(); i++)
            if (armyAlive[i] || army[i].GetPitch() != 0.0f) army[i].Draw(shaderProgram);
        for (size_t i = 0; i < defenders.size(); i++)
            if (defenderAlive[i] || defenders[i].GetPitch() != 0.0f) defenders[i].Draw(shaderProgram);
        for (CampTent& t : tents) t.Draw(shaderProgram);
        for (Tree& t : trees) t.Draw(shaderProgram);
        goldCrest.Draw(shaderProgram);
        crewLeft.Draw(shaderProgram);
        crewCentre.Draw(shaderProgram);
        crewRight.Draw(shaderProgram);
        leftCannon.Draw(shaderProgram);
        centreCannon.Draw(shaderProgram);
        rightCannon.Draw(shaderProgram);
        for (Projectile& ball : projectiles) ball.Draw(shaderProgram);
        for (Arrow& a : arrows) a.Draw(shaderProgram);
        birds.Draw(shaderProgram);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // ---- Save the framebuffer as BMP ----
        glReadPixels(0, 0, CAPTURE_W, CAPTURE_H, GL_RGB, GL_UNSIGNED_BYTE,
                     framebuffer.data());
        char fname[256];
        std::snprintf(fname, sizeof(fname), "%s%05d.bmp",
                      kFramePattern.c_str(), frameIdx);
        if (!SaveBMP(fname, CAPTURE_W, CAPTURE_H, framebuffer)) {
            cerr << "Failed to save " << fname << endl;
        }

        ++frameIdx;
        if (frameIdx % 30 == 0) {
            cout << "Frame " << frameIdx << " / " << kMaxFrames
                 << "  (battle = " << (int)battle << ")" << endl;
        }
    }

    cout << "Done.  Captured " << frameIdx << " frames into '" << kFrameDir << "/'" << endl;
    cout << "Next: powershell -NoProfile -ExecutionPolicy Bypass -File tools/encode_video.ps1" << endl;

    // ---- Cleanup ----
    ground.Delete();
    river.Delete();
    leftCannon.Delete();
    centreCannon.Delete();
    rightCannon.Delete();
    castle.Delete();
    for (Archer& a : archers) a.Delete();
    for (Soldier& s : wallSoldiers) s.Delete();
    crewLeft.Delete();
    crewCentre.Delete();
    crewRight.Delete();
    for (Soldier& s : army) s.Delete();
    for (Soldier& d : defenders) d.Delete();
    for (CampTent& t : tents) t.Delete();
    for (Tree& t : trees) t.Delete();
    scenery.Delete();
    skyClouds.Delete();
    birds.Delete();
    for (SignalTower& t : signalTowers) t.Delete();
    goldCrest.Delete();
    for (Projectile& b : projectiles) b.Delete();
    for (Arrow& a : arrows) a.Delete();
    sunCore.Delete();
    sunCorona.Delete();
    for (Mesh& m : sunRays) m.Delete();
    shaderProgram.Delete();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}