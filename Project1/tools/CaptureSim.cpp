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
#include "Ladder.h"
#include "ParticleSystem.h"

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
enum class BattlePhase {
    Inactive,
    Patrol,
    Alarm,
    Barrage,
    ShootHinges,
    CannonSiege,
    Charge,
    Melee,
    End
};
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

    Shader shaderProgram("lit.vert", "lit.frag");

    // ----- Scene (a copy of Main.cpp's main setup) -----
    Mesh ground = Primitives::CreatePlane(400.0f, 400.0f, Palette::Grass);
    Water river(vec3(-4.0f, 0.0f, 0.0f), 12.0f, 480.0f);
    const float kCannonStartX  = -23.0f;
    const float kCannonBattleX = -15.0f;
    const float cannonSpacing  = 2.5f;
    Cannon leftCannon  (vec3(kCannonStartX, 0.0f,  cannonSpacing));
    Cannon centreCannon(vec3(kCannonStartX, 0.0f,  0.0f));
    Cannon rightCannon (vec3(kCannonStartX, 0.0f, -cannonSpacing));

    centreCannon.Elevate(11.5f);
    leftCannon.Elevate(12.5f);
    leftCannon.Yaw(-18.5f);
    rightCannon.Elevate(12.5f);
    rightCannon.Yaw(+18.5f);

    CompoundCastle castle(vec3(12.0f, 0.0f, 0.0f));

    const float cornerTowerTopY = 8.0f;
    const float gatehouseTowerTopY = 5.0f;
    const float halfCompound = 12.0f;
    std::vector<Archer> archers;
    archers.emplace_back(vec3(12.0f - halfCompound, cornerTowerTopY, -halfCompound));
    archers.emplace_back(vec3(12.0f + halfCompound, cornerTowerTopY, -halfCompound));
    archers.emplace_back(vec3(12.0f - halfCompound, cornerTowerTopY,  halfCompound));
    archers.emplace_back(vec3(0.0f, gatehouseTowerTopY, -3.75f));
    archers.emplace_back(vec3(0.0f, gatehouseTowerTopY,  3.75f));
    // Archers face the battlefield (-X)
    for (Archer& a : archers) a.SetYaw(180.0f);

    // Wooden rampart access ladder leaning against inner North side wall
    Ladder ladder(vec3(6.5f, 0.0f, -10.2f), vec3(6.5f, 2.95f, -11.3f), 0.50f, 8);
    ParticleSystem particleSystem;
    particleSystem.Init();

    // Sentry placement across front, side, and back walls
    std::vector<Soldier> wallSoldiers;
    wallSoldiers.emplace_back(vec3(0.8f,  2.9f, -7.0f),  Palette::Defender); // [0] West front wall (North segment)
    wallSoldiers.emplace_back(vec3(0.8f,  2.9f, +7.0f),  Palette::Defender); // [1] West front wall (South segment)
    wallSoldiers.emplace_back(vec3(6.5f,  2.9f, -11.8f), Palette::Defender); // [2] North side wall (front half)
    wallSoldiers.emplace_back(vec3(16.0f, 2.9f, -11.8f), Palette::Defender); // [3] North side wall (rear half)
    wallSoldiers.emplace_back(vec3(6.5f,  2.9f, +11.8f), Palette::Defender); // [4] South side wall (front half)
    wallSoldiers.emplace_back(vec3(16.0f, 2.9f, +11.8f), Palette::Defender); // [5] South side wall (rear half)
    wallSoldiers.emplace_back(vec3(23.8f, 2.9f, -5.0f),  Palette::Defender); // [6] East back wall (North half)
    wallSoldiers.emplace_back(vec3(23.8f, 2.9f, +5.0f),  Palette::Defender); // [7] East back wall (South half)
    wallSoldiers[0].SetYaw(180.0f);
    wallSoldiers[1].SetYaw(180.0f);
    wallSoldiers[2].SetYaw(90.0f);
    wallSoldiers[3].SetYaw(90.0f);
    wallSoldiers[4].SetYaw(270.0f);
    wallSoldiers[5].SetYaw(270.0f);
    wallSoldiers[6].SetYaw(0.0f);
    wallSoldiers[7].SetYaw(0.0f);
    std::vector<bool> wallSoldierAlive(8, true);

    Mesh sunCore   = Primitives::CreateSphere(4.5f, 20, 20, Palette::Sun);
    Mesh sunCorona = Primitives::CreateSphere(7.0f, 16, 16, glm::vec3(1.0f, 0.96f, 0.60f));
    std::vector<Mesh> sunRays;
    for (int r = 0; r < 8; r++) {
        sunRays.push_back(Primitives::CreateCylinder(0.25f, 95.0f, 8, glm::vec3(1.0f, 0.94f, 0.50f), /*centered=*/false));
    }

    const float crewZOffset = -0.7f;
    Soldier crewLeft (vec3(kCannonStartX - 1.8f, 0.0f,  cannonSpacing + crewZOffset), Palette::Attacker);
    Soldier crewCentre(vec3(kCannonStartX - 1.8f, 0.0f,  0.0f + crewZOffset),       Palette::Attacker);
    Soldier crewRight(vec3(kCannonStartX - 1.8f, 0.0f, -cannonSpacing + crewZOffset),Palette::Attacker);

    std::vector<Soldier> army;
    std::vector<vec3> armyCampPos;
    std::vector<vec3> armyBattlePos;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 5; col++) {
            float bx = kCannonBattleX - 6.0f - float(row) * 1.5f;
            float bz = (float(col) - 2.0f) * 2.0f;
            float cx = kCannonStartX - 5.0f - float(row) * 1.5f;
            float cz = bz;
            armyBattlePos.emplace_back(bx, 0.0f, bz);
            armyCampPos.emplace_back(cx, 0.0f, cz);
            army.emplace_back(vec3(cx, 0.0f, cz), Palette::Attacker);
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
            float tx = kCannonBattleX - 6.0f + float(r) * 4.0f;   // x = -21 .. -17
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
    fireLeft.crewRestPos   = vec3(kCannonStartX - 1.8f, 0.0f,  cannonSpacing + crewZOffset);
    fireCentre.crewRestPos = vec3(kCannonStartX - 1.8f, 0.0f,  0.0f + crewZOffset);
    fireRight.crewRestPos  = vec3(kCannonStartX - 1.8f, 0.0f, -cannonSpacing + crewZOffset);
    fireLeft.crewFirePos   = vec3(kCannonStartX - 0.8f, 0.0f,  cannonSpacing + crewZOffset);
    fireCentre.crewFirePos = vec3(kCannonStartX - 0.8f, 0.0f,  0.0f + crewZOffset);
    fireRight.crewFirePos  = vec3(kCannonStartX - 0.8f, 0.0f, -cannonSpacing + crewZOffset);

    std::vector<bool> archerAlive(archers.size(), true);
    std::vector<bool> defenderAlive(defenders.size(), true);
    std::vector<bool> armyAlive(army.size(), true);
    int nextReplacementDefender = 0;
    float archerFireTimer = 0.0f;
    const float kArcherFirePeriod = 1.6f;
    float cannonAutoFireTimer = 0.0f;
    const float cannonAutoFirePeriod = 1.2f;
    int   cannonFireStep = 0;

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

    bool  cannoneerHit = false;
    bool  cannoneerReplaced = false;
    bool  hingeShotFired = false;
    bool  hingeDestroyed = false;
    int   cannonSiegeStep = 0;
    float cannonSiegeTimer = 0.0f;
    int   winnerOutcome = rand() % 2; // randomized each run!
    float spotterSmokeTimer = 0.0f;

    std::vector<Arrow> arrows;
    std::vector<Projectile> projectiles;

    BattlePhase battle = BattlePhase::Inactive;
    float battleTimer = 0.0f;
    TimeOfDay tod = TimeOfDay::Day;
    bool battlePaused = false;

    // Camera: lock to a cinematic top-down-ish angle for the video.
    float camYaw   = 0.7f;
    float camPitch = 0.40f;
    float camRadius = 52.0f;
    vec3  camTarget(3.5f, 3.5f, 0.0f);
    auto computeView = [&]() {
        float cy = std::cos(camYaw),  sy = std::sin(camYaw);
        float cp = std::cos(camPitch), sp = std::sin(camPitch);
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
    GLuint viewPosLoc  = glGetUniformLocation(shaderProgram.ID, "viewPos");
    GLuint ambStrLoc   = glGetUniformLocation(shaderProgram.ID, "ambientStrength");

    auto lerpSoldier = [](Soldier& s, const vec3& from, const vec3& to,
                          float timer, float seconds) {
        float t = (seconds <= 0.0f) ? 1.0f : timer / seconds;
        if (t > 1.0f) t = 1.0f;
        s.SetPosition(from * (1.0f - t) + to * t);
    };

    auto tickCannon = [&](Cannon& cannon, FireSequence& seq, float dt) {
        if (!seq.crew || seq.crew->IsDead() || seq.crew->health <= 0.0f) {
            seq.state = FireState::Idle;
            return;
        }
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
    // Start with Patrol to show normal sentry patrolling and ladder relief,
    // then attackers marching in, spotter red smoke, bridge drawn up, cannon fight, etc.
    battle = BattlePhase::Patrol;
    battleTimer = 0.0f;
    float dt = 1.0f / 30.0f;        // 30 fps capture - keeps the video small
    const int kMaxFrames = 30 * 60;  // up to 60 seconds of footage
    int frameIdx = 0;
    bool battleFinished = false;
    while (frameIdx < kMaxFrames && !battleFinished) {
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
            if (castle.CheckHit(ball.GetPosition(), ball.GetRadius() + 0.8f)) {
                particleSystem.EmitDebris(ball.GetPosition(), 30);
                particleSystem.EmitSmoke(ball.GetPosition(), vec3(0.0f, 1.0f, 0.0f), 12);
                ball.Kill();
            } else if (castle.HitsStatic(ball.GetPosition(), ball.GetRadius())) {
                particleSystem.EmitDebris(ball.GetPosition(), 30);
                ball.Kill();
            }
        }
        projectiles.erase(
            remove_if(projectiles.begin(), projectiles.end(),
                     [](const Projectile& b) { return b.IsDead(); }),
            projectiles.end());
        castle.Update(dt);

        // ---- Update character animations every frame ----
        for (Soldier& s : army) s.Update(dt);
        for (Soldier& s : wallSoldiers) s.Update(dt);
        for (Soldier& s : defenders) s.Update(dt);
        crewLeft.Update(dt);
        crewCentre.Update(dt);
        crewRight.Update(dt);
        for (Archer& a : archers) a.Update(dt);

        // ---- Battle sim state machine ----
        if (battle != BattlePhase::Inactive && !battlePaused) {
            battleTimer += dt;
        }

        // Phase 1: Sentry patrol on all ramparts + slow ladder relief on North side wall
        if (battle == BattlePhase::Patrol) {
            float pTime = battleTimer;

            // Front West ramparts sentries [0] and [1] patrol along Z (no ladder replacement!)
            float z0 = -7.0f + 1.8f * std::sin(pTime * 0.8f);
            wallSoldiers[0].SetPosition(vec3(0.8f, 2.9f, z0));
            wallSoldiers[0].SetYaw(180.0f);
            wallSoldiers[0].SetMarching(true);

            float z1 = +7.0f + 1.8f * std::cos(pTime * 0.8f);
            wallSoldiers[1].SetPosition(vec3(0.8f, 2.9f, z1));
            wallSoldiers[1].SetYaw(180.0f);
            wallSoldiers[1].SetMarching(true);

            // North side wall rear sentry [3] patrols along X
            float x3 = 16.0f + 2.0f * std::sin(pTime * 0.7f);
            wallSoldiers[3].SetPosition(vec3(x3, 2.9f, -11.8f));
            wallSoldiers[3].SetYaw(90.0f);
            wallSoldiers[3].SetMarching(true);

            // South side wall sentries [4] and [5] patrol along X
            float x4 = 6.5f + 2.0f * std::sin(pTime * 0.7f);
            wallSoldiers[4].SetPosition(vec3(x4, 2.9f, +11.8f));
            wallSoldiers[4].SetYaw(270.0f);
            wallSoldiers[4].SetMarching(true);

            float x5 = 16.0f - 2.0f * std::sin(pTime * 0.7f);
            wallSoldiers[5].SetPosition(vec3(x5, 2.9f, +11.8f));
            wallSoldiers[5].SetYaw(270.0f);
            wallSoldiers[5].SetMarching(true);

            // East back wall sentries [6] and [7] patrol along Z
            float z6 = -5.0f + 2.0f * std::cos(pTime * 0.7f);
            wallSoldiers[6].SetPosition(vec3(23.8f, 2.9f, z6));
            wallSoldiers[6].SetYaw(0.0f);
            wallSoldiers[6].SetMarching(true);

            float z7 = +5.0f + 2.0f * std::sin(pTime * 0.7f);
            wallSoldiers[7].SetPosition(vec3(23.8f, 2.9f, z7));
            wallSoldiers[7].SetYaw(0.0f);
            wallSoldiers[7].SetMarching(true);

            // Unhurried ladder relief on North side wall (14.0s cycle, between wallSoldiers[2] and defenders[0]):
            float loopT = std::fmod(pTime, 14.0f);
            if (loopT < 4.0f) {
                float u = loopT / 4.0f;
                float xPatrol = 5.0f + 3.0f * std::sin(u * 3.14159f);
                wallSoldiers[2].SetPosition(vec3(xPatrol, 2.9f, -11.8f));
                wallSoldiers[2].SetYaw(90.0f);
                wallSoldiers[2].SetMarching(true);

                defenders[0].SetPosition(vec3(6.5f, 0.0f, -8.0f));
                defenders[0].SetYaw(90.0f);
                defenders[0].SetMarching(false);
            } else if (loopT < 6.0f) {
                float u = (loopT - 4.0f) / 2.0f;
                wallSoldiers[2].SetPosition(glm::mix(vec3(5.0f, 2.9f, -11.8f), vec3(6.5f, 2.95f, -11.3f), u));
                wallSoldiers[2].SetYaw(90.0f);
                wallSoldiers[2].SetMarching(true);

                defenders[0].SetPosition(glm::mix(vec3(6.5f, 0.0f, -8.0f), vec3(6.5f, 0.0f, -10.2f), u));
                defenders[0].SetYaw(90.0f);
                defenders[0].SetMarching(true);
            } else if (loopT < 10.0f) {
                float u = (loopT - 6.0f) / 4.0f;
                vec3 topLadder(6.5f, 2.95f, -11.3f);
                vec3 botLadder(6.5f, 0.0f, -10.2f);
                wallSoldiers[2].SetPosition(glm::mix(topLadder, botLadder, u));
                wallSoldiers[2].SetYaw(270.0f);
                wallSoldiers[2].SetMarching(true);

                defenders[0].SetPosition(glm::mix(botLadder, topLadder, u));
                defenders[0].SetYaw(90.0f);
                defenders[0].SetMarching(true);
            } else if (loopT < 12.0f) {
                float u = (loopT - 10.0f) / 2.0f;
                wallSoldiers[2].SetPosition(glm::mix(vec3(6.5f, 0.0f, -10.2f), vec3(6.5f, 0.0f, -7.0f), u));
                wallSoldiers[2].SetYaw(270.0f);
                wallSoldiers[2].SetMarching(true);

                defenders[0].SetPosition(glm::mix(vec3(6.5f, 2.95f, -11.3f), vec3(8.0f, 2.9f, -11.8f), u));
                defenders[0].SetYaw(90.0f);
                defenders[0].SetMarching(true);
            } else {
                float u = (loopT - 12.0f) / 2.0f;
                float xRelief = 8.0f - 3.0f * u;
                defenders[0].SetPosition(vec3(xRelief, 2.9f, -11.8f));
                defenders[0].SetYaw(270.0f);
                defenders[0].SetMarching(true);

                wallSoldiers[2].SetPosition(vec3(6.5f, 0.0f, -7.0f));
                wallSoldiers[2].SetYaw(90.0f);
                wallSoldiers[2].SetMarching(false);
            }

            if (battleTimer >= 6.0f) {
                battle = BattlePhase::Alarm;
                battleTimer = 0.0f;
            }
        } else if (battle == BattlePhase::Alarm) {
            // Attackers (cannons, crew, and army) all march forward together from rear camps to battle ground
            float marchT = std::min(1.0f, battleTimer / 4.0f);

            // Cannons roll forward from kCannonStartX (-23m) to kCannonBattleX (-15m)
            float curCannonX = glm::mix(kCannonStartX, kCannonBattleX, marchT);
            leftCannon.SetPosition(vec3(curCannonX, 0.0f,  cannonSpacing));
            centreCannon.SetPosition(vec3(curCannonX, 0.0f, 0.0f));
            rightCannon.SetPosition(vec3(curCannonX, 0.0f, -cannonSpacing));

            // Crews march alongside their cannons
            float curCrewX = curCannonX - 1.8f;
            crewLeft.SetPosition(vec3(curCrewX, 0.0f,  cannonSpacing + crewZOffset));
            crewCentre.SetPosition(vec3(curCrewX, 0.0f, 0.0f + crewZOffset));
            crewRight.SetPosition(vec3(curCrewX, 0.0f, -cannonSpacing + crewZOffset));
            crewLeft.SetMarching(marchT < 1.0f);
            crewCentre.SetMarching(marchT < 1.0f);
            crewRight.SetMarching(marchT < 1.0f);
            crewLeft.SetYaw(0.0f); crewCentre.SetYaw(0.0f); crewRight.SetYaw(0.0f);

            fireLeft.crewRestPos   = vec3(kCannonBattleX - 1.8f, 0.0f,  cannonSpacing + crewZOffset);
            fireCentre.crewRestPos = vec3(kCannonBattleX - 1.8f, 0.0f,  0.0f + crewZOffset);
            fireRight.crewRestPos  = vec3(kCannonBattleX - 1.8f, 0.0f, -cannonSpacing + crewZOffset);
            fireLeft.crewFirePos   = vec3(kCannonBattleX - 0.8f, 0.0f,  cannonSpacing + crewZOffset);
            fireCentre.crewFirePos = vec3(kCannonBattleX - 0.8f, 0.0f,  0.0f + crewZOffset);
            fireRight.crewFirePos  = vec3(kCannonBattleX - 0.8f, 0.0f, -cannonSpacing + crewZOffset);

            // Army infantry march forward in formation
            for (size_t si = 0; si < army.size(); si++) {
                army[si].SetPosition(glm::mix(armyCampPos[si], armyBattlePos[si], marchT));
                army[si].SetMarching(marchT < 1.0f);
                army[si].SetYaw(0.0f);
            }

            // Spotter atop NW gatehouse tower spots incoming attackers and triggers RED SMOKE!
            spotterSmokeTimer += dt;
            if (spotterSmokeTimer >= 0.22f) {
                spotterSmokeTimer = 0.0f;
                particleSystem.EmitRedSmoke(vec3(0.0f, 5.6f, -3.75f), 12);
            }

            // Castle alarm sounds: drawbridge raises!
            castle.SetBridgeRaised(true);

            if (battleTimer >= 4.5f) {
                battle = BattlePhase::Barrage;
                battleTimer = 0.0f;
                for (Soldier& s : army) s.SetMarching(false);
                crewLeft.SetMarching(false);
                crewCentre.SetMarching(false);
                crewRight.SetMarching(false);
            }
        } else if (battle == BattlePhase::Barrage) {
            castle.SetBridgeRaised(true);

            // Attacking infantry raise their shields!
            for (Soldier& s : army) s.SetShieldRaised(true);

            // Archers on castle towers loose arrows in volleys
            archerFireTimer += dt;
            if (archerFireTimer >= 1.0f) {
                archerFireTimer = 0.0f;
                for (size_t ai = 0; ai < archers.size(); ai++) {
                    if (!archerAlive[ai]) continue;
                    archers[ai].TriggerShootAnim();
                    vec3 archerPos = archers[ai].GetPosition() + vec3(0.0f, 1.4f, 0.0f);
                    vec3 target = (ai % 2 == 0) ? crewLeft.GetPosition() : army[ai % army.size()].GetPosition();
                    target.y += 1.0f;
                    vec3 diff = target - archerPos;
                    float dist = glm::length(diff);
                    if (dist > 0.1f) {
                        vec3 v = diff / 1.1f;
                        v.y += 0.5f * 9.81f * 1.1f; // ballistic arc
                        arrows.emplace_back(archerPos, v);
                    }
                }
            }

            // Cannoneer 1 (crewLeft) steps up to light cannon without a shield
            if (battleTimer < 2.0f) {
                float walkT = std::min(1.0f, battleTimer / 1.0f);
                crewLeft.SetPosition(glm::mix(fireLeft.crewRestPos, fireLeft.crewFirePos, walkT));
                crewLeft.SetYaw(0.0f);
            } else if (battleTimer >= 2.0f && !cannoneerHit) {
                cannoneerHit = true;
                crewLeft.SetDead();
                fireLeft.crew = nullptr; // CANNOT FIRE WITHOUT CANNONEER!
                particleSystem.EmitSparks(crewLeft.GetPosition() + vec3(0.0f, 1.2f, 0.0f), vec3(0, 1, 0), 20);
            }

            // Replacement soldier breaks formation from army and rushes to take over cannon 1!
            if (cannoneerHit && battleTimer >= 2.4f) {
                float repT = std::min(1.0f, (battleTimer - 2.4f) / 1.6f);
                vec3 repStart = armyBattlePos[0];
                vec3 repTarget = fireLeft.crewFirePos;
                army[0].SetPosition(glm::mix(repStart, repTarget, repT));
                army[0].SetShieldRaised(false);
                army[0].SetMarching(repT < 1.0f);
                army[0].SetYaw(0.0f);
                if (repT >= 1.0f && !cannoneerReplaced) {
                    cannoneerReplaced = true;
                    fireLeft.crew = &army[0]; // Cannoneer arrives! Now cannon 1 can fire!
                }
            }

            if (battleTimer >= 5.0f) {
                battle = BattlePhase::ShootHinges;
                battleTimer = 0.0f;
            }
        } else if (battle == BattlePhase::ShootHinges) {
            castle.SetBridgeRaised(true);

            // Replacement cannoneer fires centre cannon at bridge hinges!
            if (battleTimer >= 0.8f && !hingeShotFired) {
                if (fireCentre.crew != nullptr && !fireCentre.crew->IsDead() && fireCentre.crew->health > 0.0f) {
                    hingeShotFired = true;
                    centreCannon.SetYaw(0.0f);
                    centreCannon.SetElevation(14.5f);
                    centreCannon.Fire();
                    vec3 muzzle = centreCannon.GetMuzzleWorldPosition();
                    vec3 fwd = centreCannon.GetForwardWorldDirection();
                    particleSystem.EmitSmoke(muzzle, fwd, 25);
                    particleSystem.EmitSparks(muzzle, fwd, 35);
                    projectiles.emplace_back(muzzle, fwd * Projectile::DefaultSpeed, Projectile::DefaultRadius);
                }
            }

            // Ball strikes hinges at t = 1.4s
            if (battleTimer >= 1.4f && !hingeDestroyed) {
                hingeDestroyed = true;
                particleSystem.EmitDebris(vec3(0.0f, 2.5f, 0.0f), 35);
                particleSystem.EmitSparks(vec3(0.0f, 2.5f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 30);
                // Bridge hinges broken! Bridge drops flat across river!
                castle.SetBridgeRaised(false);
            }

            // Impact splash on river bank when bridge hits flat
            if (battleTimer >= 2.0f && battleTimer < 2.2f) {
                particleSystem.EmitSmoke(vec3(-8.0f, 0.15f, 0.0f), vec3(0, 1, 0), 20);
            }

            if (battleTimer >= 3.5f) {
                battle = BattlePhase::CannonSiege;
                battleTimer = 0.0f;
                cannonSiegeStep = 0;
                cannonSiegeTimer = 0.0f;
            }
        } else if (battle == BattlePhase::CannonSiege) {
            castle.SetBridgeRaised(false);
            cannonSiegeTimer += dt;

            // Cannons dynamically retarget and fire volleys destroying front walls, towers & doors!
            // Negative yaw turns toward +Z (left wall / SW tower); positive yaw turns toward -Z (right wall / NW tower)
            // Step 0: Left & Right cannons fire at front curtain walls
            if (cannonSiegeStep == 0 && cannonSiegeTimer >= 0.4f) {
                cannonSiegeStep = 1;
                if (fireLeft.crew && !fireLeft.crew->IsDead() && fireLeft.crew->health > 0.0f) {
                    leftCannon.SetYaw(-19.5f); leftCannon.SetElevation(12.5f); leftCannon.Fire();
                    particleSystem.EmitSmoke(leftCannon.GetMuzzleWorldPosition(), leftCannon.GetForwardWorldDirection(), 22);
                    projectiles.emplace_back(leftCannon.GetMuzzleWorldPosition(), leftCannon.GetForwardWorldDirection() * Projectile::DefaultSpeed, Projectile::DefaultRadius);
                }
                if (fireRight.crew && !fireRight.crew->IsDead() && fireRight.crew->health > 0.0f) {
                    rightCannon.SetYaw(+19.5f); rightCannon.SetElevation(12.5f); rightCannon.Fire();
                    particleSystem.EmitSmoke(rightCannon.GetMuzzleWorldPosition(), rightCannon.GetForwardWorldDirection(), 22);
                    projectiles.emplace_back(rightCannon.GetMuzzleWorldPosition(), rightCannon.GetForwardWorldDirection() * Projectile::DefaultSpeed, Projectile::DefaultRadius);
                }
            }
            // Step 1: Centre cannon fires at wooden gate doors
            else if (cannonSiegeStep == 1 && cannonSiegeTimer >= 1.8f) {
                cannonSiegeStep = 2;
                if (fireCentre.crew && !fireCentre.crew->IsDead() && fireCentre.crew->health > 0.0f) {
                    centreCannon.SetYaw(0.0f); centreCannon.SetElevation(11.5f); centreCannon.Fire();
                    particleSystem.EmitSmoke(centreCannon.GetMuzzleWorldPosition(), centreCannon.GetForwardWorldDirection(), 22);
                    projectiles.emplace_back(centreCannon.GetMuzzleWorldPosition(), centreCannon.GetForwardWorldDirection() * Projectile::DefaultSpeed, Projectile::DefaultRadius);
                }
            }
            // Step 2: Second volley demolishes front walls! Rubble flies!
            else if (cannonSiegeStep == 2 && cannonSiegeTimer >= 3.4f) {
                cannonSiegeStep = 3;
                if (fireLeft.crew && !fireLeft.crew->IsDead() && fireLeft.crew->health > 0.0f) {
                    leftCannon.Fire();
                    particleSystem.EmitSmoke(leftCannon.GetMuzzleWorldPosition(), leftCannon.GetForwardWorldDirection(), 22);
                    projectiles.emplace_back(leftCannon.GetMuzzleWorldPosition(), leftCannon.GetForwardWorldDirection() * Projectile::DefaultSpeed, Projectile::DefaultRadius);
                }
                if (fireRight.crew && !fireRight.crew->IsDead() && fireRight.crew->health > 0.0f) {
                    rightCannon.Fire();
                    particleSystem.EmitSmoke(rightCannon.GetMuzzleWorldPosition(), rightCannon.GetForwardWorldDirection(), 22);
                    projectiles.emplace_back(rightCannon.GetMuzzleWorldPosition(), rightCannon.GetForwardWorldDirection() * Projectile::DefaultSpeed, Projectile::DefaultRadius);
                }
                particleSystem.EmitDebris(vec3(0.0f, 2.0f, +7.875f), 45);
                particleSystem.EmitDebris(vec3(0.0f, 2.0f, -7.875f), 45);
                // Front wall sentries die on collapse
                wallSoldierAlive[0] = false; wallSoldiers[0].SetDead();
                wallSoldierAlive[1] = false; wallSoldiers[1].SetDead();
            }
            // Step 3: Cannons sweep barrels to retarget towers & doors!
            else if (cannonSiegeStep == 3 && cannonSiegeTimer >= 5.0f) {
                cannonSiegeStep = 4;
                // Left cannon turns to Front SW Corner Tower!
                leftCannon.SetYaw(-32.0f); leftCannon.SetElevation(14.5f);
                // Right cannon turns to Front NW Corner Tower!
                rightCannon.SetYaw(+32.0f); rightCannon.SetElevation(14.5f);
                // Centre cannon fires second shot, shattering the doors!
                if (fireCentre.crew && !fireCentre.crew->IsDead() && fireCentre.crew->health > 0.0f) {
                    centreCannon.Fire();
                    particleSystem.EmitSmoke(centreCannon.GetMuzzleWorldPosition(), centreCannon.GetForwardWorldDirection(), 25);
                    projectiles.emplace_back(centreCannon.GetMuzzleWorldPosition(), centreCannon.GetForwardWorldDirection() * Projectile::DefaultSpeed, Projectile::DefaultRadius);
                    castle.CheckHit(vec3(0.5f, 1.5f, 0.0f), 2.5f);
                    particleSystem.EmitDebris(vec3(0.5f, 1.5f, 0.0f), 40);
                }
            }
            // Step 4: Cannons fire at corner towers! Towers collapse!
            else if (cannonSiegeStep == 4 && cannonSiegeTimer >= 6.8f) {
                cannonSiegeStep = 5;
                if (fireLeft.crew && !fireLeft.crew->IsDead() && fireLeft.crew->health > 0.0f) {
                    leftCannon.Fire();
                    particleSystem.EmitSmoke(leftCannon.GetMuzzleWorldPosition(), leftCannon.GetForwardWorldDirection(), 25);
                    projectiles.emplace_back(leftCannon.GetMuzzleWorldPosition(), leftCannon.GetForwardWorldDirection() * Projectile::DefaultSpeed, Projectile::DefaultRadius);
                    castle.CheckHit(vec3(0.0f, 6.0f, +12.0f), 3.0f);
                    particleSystem.EmitDebris(vec3(0.0f, 6.0f, +12.0f), 50);
                    archerAlive[2] = false; archers[2].SetPosition(vec3(0, -100, 0));
                }
                if (fireRight.crew && !fireRight.crew->IsDead() && fireRight.crew->health > 0.0f) {
                    rightCannon.Fire();
                    particleSystem.EmitSmoke(rightCannon.GetMuzzleWorldPosition(), rightCannon.GetForwardWorldDirection(), 25);
                    projectiles.emplace_back(rightCannon.GetMuzzleWorldPosition(), rightCannon.GetForwardWorldDirection() * Projectile::DefaultSpeed, Projectile::DefaultRadius);
                    castle.CheckHit(vec3(0.0f, 6.0f, -12.0f), 3.0f);
                    particleSystem.EmitDebris(vec3(0.0f, 6.0f, -12.0f), 50);
                    archerAlive[0] = false; archers[0].SetPosition(vec3(0, -100, 0));
                }
            }
            // Step 5: Final volley clears remaining gatehouse towers
            else if (cannonSiegeStep == 5 && cannonSiegeTimer >= 8.5f) {
                cannonSiegeStep = 6;
                castle.CheckHit(vec3(0.0f, 5.0f, -3.75f), 3.0f);
                castle.CheckHit(vec3(0.0f, 5.0f, +3.75f), 3.0f);
                particleSystem.EmitDebris(vec3(0.0f, 5.0f, -3.75f), 40);
                particleSystem.EmitDebris(vec3(0.0f, 5.0f, +3.75f), 40);
                archerAlive[4] = false; archerAlive[5] = false;
                archers[4].SetPosition(vec3(0, -100, 0));
                archers[5].SetPosition(vec3(0, -100, 0));
            }

            if (cannonSiegeTimer >= 11.2f) {
                battle = BattlePhase::Charge;
                battleTimer = 0.0f;
                armyMarchStarted = true;
                leadMarchDistance = 0.0f;
                armyMarchOrder.clear();
                for (size_t si = 0; si < army.size(); si++) {
                    if (armyAlive[si]) armyMarchOrder.push_back((int)si);
                }
            }
        } else if (battle == BattlePhase::Charge) {
            castle.SetBridgeRaised(false);
            // Cannons STAY stationary at battery line (x = -15m); ONLY soldiers move inside!
            leadMarchDistance += 3.4f * dt;
            float desiredLeadX = -15.0f + leadMarchDistance;

            for (size_t r = 0; r < armyMarchOrder.size(); r++) {
                int si = armyMarchOrder[r];
                if (!armyAlive[si]) continue;
                int file = int(r % 2); // strictly 2 lines across bridge
                int pair = int(r / 2);
                float targetZ = (file == 0) ? -0.85f : +0.85f;
                float desiredX = desiredLeadX - float(pair) * 2.2f;
                desiredX = std::max(armyBattlePos[si].x, std::min(desiredX, 7.5f));

                float currentZ = targetZ;
                if (desiredX < -11.5f) {
                    float tZ = (desiredX - armyBattlePos[si].x) / (-11.5f - armyBattlePos[si].x);
                    tZ = glm::clamp(tZ, 0.0f, 1.0f);
                    currentZ = glm::mix(armyBattlePos[si].z, targetZ, tZ);
                } else if (desiredX > 2.0f) {
                    float fanOut = (desiredX - 2.0f) / 5.0f;
                    currentZ = targetZ * (1.0f + fanOut * 0.8f);
                }

                army[si].SetPosition(vec3(desiredX, 0.0f, currentZ));
                army[si].SetMarching(true);
                army[si].SetShieldRaised(true);
                army[si].SetYaw(0.0f);
            }

            if (desiredLeadX >= 6.0f) {
                battle = BattlePhase::Melee;
                battleTimer = 0.0f;
                meleeTimer = 0.0f;
                meleeHitTimer = 0.0f;
                for (Soldier& s : army) {
                    s.SetMarching(false);
                    s.SetShieldRaised(false);
                }
            }
        } else if (battle == BattlePhase::Melee) {
            meleeTimer += dt;

            // Defenders charge out to meet attackers
            for (size_t di = 0; di < defenders.size(); di++) {
                if (!defenderAlive[di]) continue;
                vec3 cur = defenders[di].GetPosition();
                if (cur.x > 7.5f) {
                    cur.x -= 3.2f * dt;
                    defenders[di].SetPosition(cur);
                    defenders[di].SetYaw(180.0f);
                }
            }
            // Attackers advance
            for (size_t si = 0; si < army.size(); si++) {
                if (!armyAlive[si]) continue;
                vec3 cur = army[si].GetPosition();
                if (cur.x < 6.8f) {
                    cur.x += 2.0f * dt;
                    army[si].SetPosition(cur);
                    army[si].SetYaw(0.0f);
                }
            }

            // Dynamic sword strike lunges
            for (size_t si = 0; si < army.size(); si++) {
                if (armyAlive[si]) {
                    army[si].SetAttackOffset(std::sin(meleeTimer * 14.0f + float(si)) * 0.25f);
                } else army[si].SetAttackOffset(0.0f);
            }
            for (size_t di = 0; di < defenders.size(); di++) {
                if (defenderAlive[di]) {
                    defenders[di].SetAttackOffset(-std::sin(meleeTimer * 14.0f + float(di) * 1.5f) * 0.25f);
                } else defenders[di].SetAttackOffset(0.0f);
            }

            // Combat clashes & sparks
            meleeHitTimer += dt;
            if (meleeHitTimer >= 0.75f) {
                meleeHitTimer = 0.0f;
                particleSystem.EmitSparks(vec3(7.0f, 1.2f, 0.0f), vec3(0, 1, 0), 10);
            }

            if (meleeTimer >= 6.0f) {
                battle = BattlePhase::End;
                battleTimer = 0.0f;
                for (Soldier& s : army) s.SetAttackOffset(0.0f);
                for (Soldier& d : defenders) d.SetAttackOffset(0.0f);

                // Decide random winner: 0 = Attackers, 1 = Defenders
                if (winnerOutcome == -1) {
                    winnerOutcome = rand() % 2;
                }
                if (winnerOutcome == 0) {
                    for (size_t di = 0; di < defenders.size(); di++) {
                        defenderAlive[di] = false;
                        defenders[di].SetDead();
                    }
                    goldCrest.SetVictorious(true);
                } else {
                    for (size_t si = 0; si < army.size(); si++) {
                        if (si % 2 == 0) {
                            armyAlive[si] = false;
                            army[si].SetDead();
                        }
                    }
                    goldCrest.SetVictorious(false);
                }
            }
        } else if (battle == BattlePhase::End) {
            if (winnerOutcome == 0) {
                for (size_t si = 0; si < army.size(); si++) {
                    if (!armyAlive[si]) continue;
                    vec3 cur = army[si].GetPosition();
                    if (cur.x < 14.5f) {
                        cur.x += 1.8f * dt;
                        army[si].SetPosition(cur);
                    }
                }
            } else {
                for (size_t di = 0; di < defenders.size(); di++) {
                    if (defenderAlive[di]) defenders[di].SetYaw(180.0f);
                }
            }
            if (battleTimer > 6.0f) battleFinished = true;
        }

        // ---- Update arrows + check shield blocks / soldier hits ----
        for (Arrow& a : arrows) {
            a.Update(dt);
            if (a.IsDead()) continue;
            vec3 ap = a.GetPosition();

            // Check cannoneer 1 (crewLeft) hit during Barrage
            if (battle == BattlePhase::Barrage && !cannoneerHit) {
				vec3 cp = crewLeft.GetPosition() + vec3(0.0f, 1.2f, 0.0f);
				if (glm::distance(ap, cp) < 0.9f) {
					cannoneerHit = true;
					crewLeft.SetDead();
					fireLeft.crew = nullptr;
					particleSystem.EmitSparks(cp, vec3(0, 1, 0), 20);
					a.Kill();
					continue;
				}
            }

            // Check army soldiers
            for (size_t si = 0; si < army.size(); si++) {
                if (!armyAlive[si]) continue;
                vec3 sp = army[si].GetPosition() + vec3(0.0f, 1.2f, 0.0f);
                if (glm::distance(ap, sp) < 0.85f) {
                    if (army[si].IsShieldRaised()) {
                        particleSystem.EmitSparks(ap, vec3(0, 1, 0), 8);
                        a.Kill();
                    } else {
                        army[si].TakeDamage(50.0f);
                        if (army[si].IsDead()) {
                            armyAlive[si] = false;
                            army[si].SetDead();
                        }
                        a.Kill();
                    }
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
            wallSoldiers[0].SetDead();
        }
        if (!castle.IsFrontWallPieceAlive(1)) {
            wallSoldierAlive[1] = false;
            wallSoldiers[1].SetDead();
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

        float cy = std::cos(camYaw),  sy = std::sin(camYaw);
        float cp = std::cos(camPitch), sp = std::sin(camPitch);
        vec3 dir = vec3(cp * cy, -sp, cp * sy);
        vec3 eye = camTarget - dir * camRadius;
        if (viewPosLoc != -1) glUniform3fv(viewPosLoc, 1, value_ptr(eye));
        if (ambStrLoc != -1) glUniform1f(ambStrLoc, 0.35f);

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
        ladder.Draw(shaderProgram);
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

        // Dynamic FX: Smoke, sparks, debris
        particleSystem.Update(dt);
        particleSystem.Draw(shaderProgram, view, projMatrix);

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
    ladder.Delete();
    particleSystem.Delete();
    shaderProgram.Delete();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}