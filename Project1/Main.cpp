#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
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
#include "Wall.h"
#include "FortGate.h"
#include "CompoundCastle.h"
#include "Tower.h"
#include "Door.h"
#include "Tree.h"
#include "Robot.h"
#include "Archer.h"
#include "Soldier.h"
#include "Bridge.h"

using namespace std;
using namespace glm;

// === Window / camera globals =================================================
// The window is fullscreen by default on the primary monitor.  Press F11 to
// toggle windowed <-> fullscreen.  The camera is an ORBIT camera around the
// compound centre: drag with the left mouse button to rotate (yaw / pitch),
// and scroll the wheel to zoom in / out.

static const vec3 kCastleCentre(12.0f, 1.5f, 0.0f);
static const float kOrbitMinRadius = 8.0f;
static const float kOrbitMaxRadius = 120.0f;

struct OrbitCamera {
    float yaw   = 0.7f;     // radians around Y, 0 = looking down +X
    float pitch = 0.25f;    // radians above horizon (clamped to (-1.4, 1.4))
    float radius = 38.0f;   // distance from target
    vec3  target = kCastleCentre;
    bool  dragging = false;
    double lastX = 0.0, lastY = 0.0;
};

static OrbitCamera g_cam;

// Fullscreen state: we cache the windowed-mode size so F11 can go back.
static bool g_fullscreen = true;
static int  g_winWidth  = 1280;
static int  g_winHeight = 800;

static void OnMouseButton(GLFWwindow* w, int button, int action, int /*mods*/) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_cam.dragging = true;
            double mx, my;
            glfwGetCursorPos(w, &mx, &my);
            g_cam.lastX = mx; g_cam.lastY = my;
        } else {
            g_cam.dragging = false;
        }
    }
}
static void OnCursorPos(GLFWwindow* /*w*/, double mx, double my) {
    if (!g_cam.dragging) return;
    double dx = mx - g_cam.lastX;
    double dy = my - g_cam.lastY;
    g_cam.lastX = mx; g_cam.lastY = my;
    // 0.005 rad / pixel feels good for a 1080p screen.
    g_cam.yaw   -= float(dx) * 0.005f;
    g_cam.pitch -= float(dy) * 0.005f;
    if (g_cam.pitch >  1.4f) g_cam.pitch =  1.4f;
    if (g_cam.pitch < -0.2f) g_cam.pitch = -0.2f;
}
static void OnScroll(GLFWwindow* /*w*/, double /*xoff*/, double yoff) {
    g_cam.radius -= float(yoff) * 1.5f;
    if (g_cam.radius < kOrbitMinRadius) g_cam.radius = kOrbitMinRadius;
    if (g_cam.radius > kOrbitMaxRadius) g_cam.radius = kOrbitMaxRadius;
}

static mat4 ComputeView() {
    float cy = cos(g_cam.yaw),  sy = sin(g_cam.yaw);
    float cp = cos(g_cam.pitch), sp = sin(g_cam.pitch);
    vec3 dir = vec3(cp * cy, sp, cp * sy);
    vec3 eye = g_cam.target - dir * g_cam.radius;
    return lookAt(eye, g_cam.target, vec3(0.0f, 1.0f, 0.0f));
}

static void ToggleFullscreen(GLFWwindow* window) {
    g_fullscreen = !g_fullscreen;
    if (g_fullscreen) {
        GLFWmonitor* mon = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(mon);
        glfwSetWindowMonitor(window, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
        glViewport(0, 0, mode->width, mode->height);
    } else {
        glfwSetWindowMonitor(window, nullptr,
                             80, 80, g_winWidth, g_winHeight, GLFW_DONT_CARE);
        glViewport(0, 0, g_winWidth, g_winHeight);
    }
}

// === Phase 6: cannon-firing state machine ===================================
// A cannon is Idle by default. Pressing Space (with the cannon selected)
// drives it through three timed stages:
//
//   CrewWalking (0.5 s)  - the cannon's crew soldier lerps from its rest
//                          position to the cannon, then stands still
//                          "lighting the fuse".
//   Lighting    (1.0 s)   - the cannon barrel visually flashes (its shaft
//                          material lerps toward brass).
//   Firing     (1 frame) - projectile is spawned at the muzzle, recoil
//                          applied to the carriage, then state resets to
//                          Idle and the crew soldier lerps back to rest
//                          over 0.3 s.
//
// All per-cannon state lives here so the Cannon class itself stays
// oblivious to the fact that a Soldier exists.

enum class FireState { Idle, CrewWalking, Lighting, Firing, CrewReturning };

struct FireSequence {
    FireState state = FireState::Idle;
    float timer = 0.0f;            // counts up during timed stages
    Soldier* crew = nullptr;        // cannon's assigned crew soldier
    vec3 crewRestPos;               // where the soldier stands between fires
    vec3 crewFirePos;                // where the soldier stands at the cannon
};

// Where the crew soldier stands "next to the cannon" while lighting it.
// Just behind and to the side of the carriage, so they don't clip the
// wheels. Z offset is -0.7 so they're slightly south of the cannon's
// firing line (visible to the player camera).
static vec3 CrewFirePosForCannon(const Cannon& cannon, float zSide) {
    vec3 c = cannon.GetMuzzleWorldPosition();
    return vec3(c.x - 1.5f, 0.0f, c.z + zSide);
}

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open fullscreen on the primary monitor from the start.
	GLFWmonitor* primaryMon = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode  = glfwGetVideoMode(primaryMon);
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height,
	                                      "Medieval Cannon - Phase 7",
	                                      primaryMon, NULL);
	if (window == NULL) {
		cout << "Failed to create window!" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glViewport(0, 0, mode->width, mode->height);
	glEnable(GL_DEPTH_TEST);

	glfwSetMouseButtonCallback(window, OnMouseButton);
	glfwSetCursorPosCallback(window, OnCursorPos);
	glfwSetScrollCallback(window, OnScroll);

	Shader shaderProgram("lit.vert", "lit.frag");

	Mesh ground = Primitives::CreatePlane(160.0f, 160.0f, Palette::Grass);

	// --- Phase 7: a Z-running river in front of the castle -----------
	// The river is now a tall thin strip running along Z, sitting just
	// past the moat on the -X side.  It visually continues the moat
	// outward in both directions (the moat itself is the strip across
	// the west face of the compound).  Drawn AFTER the ground so its
	// surface overwrites the grass in the river footprint.
	//
	// Centre X = 12 - 16 = -4 (one compound-halfX past the western
	// gatehouse).  Width 12 m along X, length 240 m along Z.
	Water river(vec3(-4.0f, 0.0f, 0.0f), /*sizeX=*/12.0f, /*sizeZ=*/240.0f);

	// --- Phase 5: three cannons lined up BEHIND the moat ----------------
	const float cannonX = -15.0f;                  // ~8 m back from the moat's far bank
	const float cannonSpacing = 2.5f;              // Z spacing between adjacent cannons
	Cannon centreCannon(vec3(cannonX, 0.0f,  0.0f));
	Cannon leftCannon  (vec3(cannonX, 0.0f,  cannonSpacing));
	Cannon rightCannon (vec3(cannonX, 0.0f, -cannonSpacing));

	// Phase 5: 12 deg default + 8 deg tweak = 20 deg total. At v=14 m/s
	// and 20 deg, range is ~14.4 m which lines up with cannon-to-door.
	centreCannon.Elevate(8.0f);

	// --- Phase 5: full compound castle (4 corners + curtains + gate) ---
	CompoundCastle castle(vec3(12.0f, 0.0f, 0.0f));

	// --- Phase 6: archers on top of the castle towers -----------------
	// 4 corner towers (each cornerHeight=8 + parapetH=0.7 = 8.7 m up).
	// 2 gatehouse flanking towers (each bodyH=5 + parapetH=0.6 = 5.6 m up).
	const float cornerParapetY = 8.0f + 0.7f;
	const float gatehouseParapetY = 5.0f + 0.6f;
	const float halfCompound = 12.0f;

	std::vector<Archer> archers;
	archers.emplace_back(vec3(12.0f - halfCompound, cornerParapetY, 0.0f - halfCompound)); // cornerNW archer
	archers.emplace_back(vec3(12.0f + halfCompound, cornerParapetY, 0.0f - halfCompound)); // cornerNE
	archers.emplace_back(vec3(12.0f - halfCompound, cornerParapetY, 0.0f + halfCompound)); // cornerSW
	archers.emplace_back(vec3(12.0f + halfCompound, cornerParapetY, 0.0f + halfCompound)); // cornerSE
	// Gatehouse flanking towers at (x = 0, z = ±3.75).
	archers.emplace_back(vec3(0.0f, gatehouseParapetY, -3.75f));
	archers.emplace_back(vec3(0.0f, gatehouseParapetY,  3.75f));

	// Phase 7: archers face the camera (default view is south of the
	// castle looking toward +Z), so each archer yaws 90° to face +Z.
	// The gatehouse archers get the same yaw.
	for (Archer& a : archers) a.SetYaw(90.0f);

	// --- Phase 6: cannon crew + army ---------------------------------
	// 3 crew (one per cannon, in warm Copper) plus 15 army (Iron) in a
	// 5x3 grid behind the cannons.
	const float crewZOffset = -0.7f;
	Soldier crewLeft (vec3(cannonX - 1.8f, 0.0f,  cannonSpacing + crewZOffset),
	                  Palette::Copper);
	Soldier crewCentre(vec3(cannonX - 1.8f, 0.0f,  0.0f + crewZOffset),
	                  Palette::Copper);
	Soldier crewRight(vec3(cannonX - 1.8f, 0.0f, -cannonSpacing + crewZOffset),
	                  Palette::Copper);

	std::vector<Soldier> army;
	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 5; col++) {
			float x = cannonX - 6.0f - float(row) * 1.5f;   // 5 columns -> 5 * 1.5 = 7.5 m
			float z = (float(col) - 2.0f) * 2.0f;          // centred on z = 0
			army.emplace_back(vec3(x, 0.0f, z), Palette::Iron);
		}
	}

	// Phase 7: army faces the castle door (default 0° yaw already
	// points them along +X, but make it explicit).  Cannon crew sits
	// next to the cannon's centre facing the same direction.
	for (Soldier& s : army) s.SetYaw(0.0f);
	crewLeft  .SetYaw(0.0f);
	crewCentre.SetYaw(0.0f);
	crewRight .SetYaw(0.0f);

	// --- Phase 6: per-cannon fire sequence state ---------------------
	FireSequence fireLeft, fireCentre, fireRight;
	fireLeft.crew   = &crewLeft;
	fireCentre.crew = &crewCentre;
	fireRight.crew  = &crewRight;
	fireLeft.crewRestPos   = vec3(cannonX - 1.8f, 0.0f,  cannonSpacing + crewZOffset);
	fireCentre.crewRestPos = vec3(cannonX - 1.8f, 0.0f,  0.0f + crewZOffset);
	fireRight.crewRestPos  = vec3(cannonX - 1.8f, 0.0f, -cannonSpacing + crewZOffset);
	// The "fire" position is slightly further toward the cannon (a bit
	// closer to the carriage) and offset on Z so the soldier can reach
	// the touch-hole without clipping the wheel.
	fireLeft.crewFirePos   = vec3(cannonX - 0.8f, 0.0f,  cannonSpacing + crewZOffset);
	fireCentre.crewFirePos = vec3(cannonX - 0.8f, 0.0f,  0.0f + crewZOffset);
	fireRight.crewFirePos  = vec3(cannonX - 0.8f, 0.0f, -cannonSpacing + crewZOffset);

	// --- Cannon selection (1/2/3 toggle, A selects all) -----------------
	bool cannonSelected[3] = { false, false, false };
	// Index of the most-recently-selected cannon.  Arrow keys / W / S
	// apply only to this cannon so multi-cannon firing still feels
	// single-player-controlled.  -1 = none selected yet.
	int   activeCannonIdx = -1;

	// --- Phase 6: ~60 trees scattered around the scene ---------------
	// Uses a deterministic seeded RNG so the layout is reproducible for
	// screenshots.  Three zones: foreground (between cannons and moat),
	// behind the castle, far flanks.
	std::vector<Tree> trees;
	std::mt19937 rng(12345);
	auto scatterIn = [&](float xLo, float xHi, float zLo, float zHi,
	                     int count, float trunkMin, float trunkMax,
	                     float trunkRMin, float trunkRMax,
	                     float crownHMin, float crownHMax,
	                     float crownRMin, float crownRMax) {
		std::uniform_real_distribution<float> uX(xLo, xHi);
		std::uniform_real_distribution<float> uZ(zLo, zHi);
		std::uniform_real_distribution<float> uTrunkH(trunkMin, trunkMax);
		std::uniform_real_distribution<float> uTrunkR(trunkRMin, trunkRMax);
		std::uniform_real_distribution<float> uCrownH(crownHMin, crownHMax);
		std::uniform_real_distribution<float> uCrownR(crownRMin, crownRMax);
		for (int i = 0; i < count; i++) {
			trees.emplace_back(vec3(uX(rng), 0.0f, uZ(rng)),
			                   uTrunkH(rng), uTrunkR(rng),
			                   uCrownH(rng), uCrownR(rng));
		}
	};
	scatterIn(-35.0f, -5.0f, -22.0f, 22.0f, 15,  // foreground (small/med)
	          1.5f, 3.0f, 0.15f, 0.28f, 2.0f, 3.5f, 1.0f, 1.7f);
	scatterIn(20.0f, 60.0f, -28.0f, 28.0f, 25,    // behind castle
	          2.0f, 3.5f, 0.18f, 0.32f, 2.5f, 4.0f, 1.2f, 2.0f);
	scatterIn(-70.0f, -35.0f, -30.0f, 30.0f, 12,  // far flank left
	          1.8f, 3.2f, 0.16f, 0.28f, 2.0f, 3.5f, 1.0f, 1.8f);
	scatterIn(30.0f, 75.0f, -30.0f, 30.0f, 12,    // far flank right
	          1.8f, 3.2f, 0.16f, 0.28f, 2.0f, 3.5f, 1.0f, 1.8f);

	// --- The wooden dummy robot inside the castle ------------------------
	Robot robot(vec3(3.0f, 0.0f, 0.0f));

	std::vector<Projectile> projectiles;

	// Recompute the projection matrix whenever the window is resized.
	auto updateProjection = [&]() -> mat4 {
		int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);
		if (fbw == 0 || fbh == 0) fbw = 1, fbh = 1;
		return perspective(radians(55.0f), float(fbw) / float(fbh), 0.1f, 300.0f);
	};
	mat4 projMatrix = updateProjection();

	GLuint viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
	GLuint projLoc = glGetUniformLocation(shaderProgram.ID, "proj");
	GLuint modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
	GLuint lightDirLoc = glGetUniformLocation(shaderProgram.ID, "lightDir");

	vec3 lightDir = normalize(vec3(-0.4f, -1.0f, -0.5f));

	const float elevationSpeedDegPerSec = 30.0f;
	const float driveSpeed = 2.0f;
	double lastFrameTime = glfwGetTime();

	// Helper: lerp a soldier smoothly between two positions over a
	// duration `seconds`, advancing `timer` by `deltaTime`. When the
	// timer reaches the duration, the soldier is at `to` and the
	// function returns true (so the caller can transition state).
	auto LerpSoldier = [](Soldier& soldier,
	                      const vec3& from, const vec3& to,
	                      float timer, float seconds) {
		float t = (seconds <= 0.0f) ? 1.0f : timer / seconds;
		if (t > 1.0f) t = 1.0f;
		vec3 pos = from * (1.0f - t) + to * t;
		soldier.SetPosition(pos);
	};

	while (!glfwWindowShouldClose(window)) {
		double currentTime = glfwGetTime();
		float deltaTime = float(currentTime - lastFrameTime);
		lastFrameTime = currentTime;

		// --- Input: F11 toggles fullscreen --------------------------------
		static bool f11Prev = false;
		bool f11Now = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;
		if (f11Now && !f11Prev) ToggleFullscreen(window);
		f11Prev = f11Now;

		// --- Input: 1/2/3 toggle individual cannon selection -------------
		static bool selPrev[3] = { false, false, false };
		bool selNow[3];
		selNow[0] = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
		selNow[1] = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
		selNow[2] = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
		for (int i = 0; i < 3; i++) {
			if (selNow[i] && !selPrev[i]) {
				cannonSelected[i] = !cannonSelected[i];
				// Track most-recently-toggled cannon so the arrow/W/S
				// controls have something to drive.
				if (cannonSelected[i]) activeCannonIdx = i;
			}
			selPrev[i] = selNow[i];
		}
		// A (press) selects every cannon at once.  The centre cannon
		// becomes the "active" one for arrow / W / S input.
		static bool aPrev = false;
		bool aNow = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
		if (aNow && !aPrev) {
			for (int i = 0; i < 3; i++) cannonSelected[i] = true;
			activeCannonIdx = 1;
		}
		aPrev = aNow;

		// --- Input: R (HOLD) raises the drawbridge -----------------------
		bool rNow = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
		castle.SetBridgeRaised(rNow);

		// --- Input: arrow / W / S drive the active cannon (if any) ------
		if (activeCannonIdx >= 0) {
			Cannon* activeCannons[3] = { &leftCannon, &centreCannon, &rightCannon };
			Cannon& c = *activeCannons[activeCannonIdx];
			// Left / Right arrows yaw the active cannon.
			const float yawSpeedDegPerSec = 60.0f;
			if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) c.Yaw(-yawSpeedDegPerSec * deltaTime);
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) c.Yaw(+yawSpeedDegPerSec * deltaTime);
			// Up / Down arrows drive the active cannon along its
			// forward direction (not just world +X).
			float drive = 0.0f;
			if (glfwGetKey(window, GLFW_KEY_UP)   == GLFW_PRESS) drive += driveSpeed * deltaTime;
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) drive -= driveSpeed * deltaTime;
			if (drive != 0.0f) c.MoveForward(drive);
			// W / S elevate the barrel.
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) c.Elevate(elevationSpeedDegPerSec * deltaTime);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) c.Elevate(-elevationSpeedDegPerSec * deltaTime);
		}

		// --- Input: Spacebar starts the firing sequence on every -----
		// selected cannon that's currently Idle. Other selected cannons
		// (already firing) are skipped.
		static bool spacePrev = false;
		bool spaceNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
		if (spaceNow && !spacePrev) {
			Cannon*  cannons[3]   = { &leftCannon, &centreCannon, &rightCannon };
			FireSequence* seqs[3] = { &fireLeft,   &fireCentre,   &fireRight  };
			for (int i = 0; i < 3; i++) {
				if (cannonSelected[i] && seqs[i]->state == FireState::Idle) {
					seqs[i]->state = FireState::CrewWalking;
					seqs[i]->timer = 0.0f;
				}
			}
		}
		spacePrev = spaceNow;

		// --- Advance each cannon's fire sequence -----------------------
		auto tickCannon = [&](Cannon& cannon, FireSequence& seq, const char* /*name*/) {
			switch (seq.state) {
				case FireState::Idle:
					// Ensure the crew soldier is at rest.
					seq.crew->SetPosition(seq.crewRestPos);
					break;
				case FireState::CrewWalking: {
					seq.timer += deltaTime;
					LerpSoldier(*seq.crew, seq.crewRestPos, seq.crewFirePos,
					            seq.timer, /*seconds=*/0.5f);
					if (seq.timer >= 0.5f) {
						seq.state = FireState::Lighting;
						seq.timer = 0.0f;
					}
					break;
				}
				case FireState::Lighting: {
					seq.timer += deltaTime;
					// Soldier stands still at fire position.
					seq.crew->SetPosition(seq.crewFirePos);
					if (seq.timer >= 1.0f) {
						// Fire! Spawn projectile, apply recoil, go to
						// CrewReturning so the soldier walks home.
						vec3 muzzle  = cannon.GetMuzzleWorldPosition();
						vec3 forward = cannon.GetForwardWorldDirection();
						projectiles.emplace_back(
							muzzle,
							forward * Projectile::DefaultSpeed,
							Projectile::DefaultRadius);
						cannon.Fire();
						seq.state = FireState::CrewReturning;
						seq.timer = 0.0f;
					}
					break;
				}
				case FireState::Firing: {
					// Unused — Firing is a one-frame state inside Lighting;
					// the actual projectile spawn happens in the transition.
					seq.state = FireState::CrewReturning;
					seq.timer = 0.0f;
					break;
				}
				case FireState::CrewReturning: {
					seq.timer += deltaTime;
					LerpSoldier(*seq.crew, seq.crewFirePos, seq.crewRestPos,
					            seq.timer, /*seconds=*/0.3f);
					if (seq.timer >= 0.3f) {
						seq.state = FireState::Idle;
						seq.timer = 0.0f;
					}
					break;
				}
			}
		};
		tickCannon(leftCannon,   fireLeft,   "left");
		tickCannon(centreCannon, fireCentre, "centre");
		tickCannon(rightCannon,  fireRight,  "right");

		// --- Update cannon recoil decay (carriage springs back) ----------
		leftCannon  .Update(deltaTime);
		centreCannon.Update(deltaTime);
		rightCannon .Update(deltaTime);

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		// --- Update projectiles and check the gate ------------------------
		for (Projectile& ball : projectiles) {
			ball.Update(deltaTime, Projectile::Gravity);
			castle.CheckHit(ball.GetPosition(), ball.GetRadius());
		}
		projectiles.erase(
			std::remove_if(projectiles.begin(), projectiles.end(),
				[](const Projectile& b) { return b.IsDead(); }),
			projectiles.end());

		// --- Update castle (advances door break physics + bridge retract)
		castle.Update(deltaTime);

		// --- Camera matrix ----------------------------------------------
		mat4 view = ComputeView();
		projMatrix = updateProjection();

		// --- Draw ----------------------------------------------------------
		glClearColor(0.55f, 0.72f, 0.87f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderProgram.Activate();

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projMatrix));
		glUniform3fv(lightDirLoc, 1, value_ptr(lightDir));

		mat4 groundMatrix = translate(mat4(1.0f), vec3(0.0f, -0.01f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(groundMatrix));
		ground.Draw();

		// River goes on top of the ground so its surface is visible in
		// the river footprint. Drawn before the castle so the moat can
		// still occlude the bits inside the compound.
		river.Draw(shaderProgram);

		for (Tree& t : trees) t.Draw(shaderProgram);
		castle.Draw(shaderProgram);
		robot.Draw(shaderProgram);

		// Archers stand on top of towers - draw after the castle so
		// they appear in front of any tower silhouette behind them.
		for (Archer& a : archers) a.Draw(shaderProgram);

		// Army in the back.
		for (Soldier& s : army) s.Draw(shaderProgram);

		// Cannon crew next to their cannons - draw before the cannons
		// so the cannon carriage hides their legs (they're standing
		// behind it).
		crewLeft  .Draw(shaderProgram);
		crewCentre.Draw(shaderProgram);
		crewRight .Draw(shaderProgram);

		// Three cannons side by side on the moat-far side.
		leftCannon  .Draw(shaderProgram);
		centreCannon.Draw(shaderProgram);
		rightCannon .Draw(shaderProgram);

		// Visual feedback for selected cannons: a thin yellow line on the
		// ground showing where the cannon is pointing.  Phase 7 makes
		// this an aim line rather than a sphere on the muzzle so the
		// player can predict where their shot will land.
		//
		// The line is a long thin box (0.05 x 0.01 x ~25 m) positioned
		// just above the ground (y = 0.02) starting from the cannon's
		// muzzle XZ and extending along the cannon's forward direction
		// for 25 m.  Drawn once per selected cannon.
		static Mesh aimLine = Primitives::CreateBox(0.05f, 0.01f, 25.0f,
                                                    Palette::Indicator);
		for (int i = 0; i < 3; i++) {
			if (!cannonSelected[i]) continue;
			Cannon* cs[3] = { &leftCannon, &centreCannon, &rightCannon };
			vec3 muzzle = cs[i]->GetMuzzleWorldPosition();
			vec3 fwd    = cs[i]->GetForwardWorldDirection();
			// Drop the aim line down to ground height (cannon
			// elevation tips the barrel but we want the indicator to
			// sit on the grass, not up in the air).
			muzzle.y = 0.02f;
			// Build a translation+rotation matrix that places the box
			// at `muzzle` and points it along `fwd` (XZ projection).
			// The box is built along Z so we need to rotate so its +Z
			// aligns with `fwd`.
			vec3 fwdFlat = normalize(vec3(fwd.x, 0.0f, fwd.z));
			float ang = atan2(fwdFlat.x, fwdFlat.z);   // rotation around Y
			mat4 aimM = translate(mat4(1.0f), muzzle)
			          * rotate(mat4(1.0f), ang, vec3(0.0f, 1.0f, 0.0f))
			          * translate(mat4(1.0f), vec3(0.0f, 0.0f, 12.5f));  // half of 25 m so it starts at muzzle
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(aimM));
			aimLine.Draw();
		}

		for (Projectile& ball : projectiles) {
			ball.Draw(shaderProgram);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ground.Delete();
	river.Delete();
	leftCannon.Delete();
	centreCannon.Delete();
	rightCannon.Delete();
	castle.Delete();
	for (Tree& t : trees) t.Delete();
	for (Archer& a : archers) a.Delete();
	crewLeft.Delete();
	crewCentre.Delete();
	crewRight.Delete();
	for (Soldier& s : army) s.Delete();
	robot.Delete();
	for (Projectile& ball : projectiles) ball.Delete();
	shaderProgram.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}