#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
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
#include "Robot.h"
#include "Archer.h"
#include "Soldier.h"
#include "Bridge.h"
#include "CampTent.h"
#include "Scenery.h"
#include "GoldCrest.h"
#include "Arrow.h"

using namespace std;
using namespace glm;

// === Window / camera globals =================================================
// The window is fullscreen by default on the primary monitor.  Press F11 to
// toggle windowed <-> fullscreen.  The camera is an ORBIT camera around the
// compound centre: drag with the left mouse button to rotate (yaw / pitch),
// and scroll the wheel to zoom in / out.

static const vec3 kCastleCentre(12.0f, 1.5f, 0.0f);
static const float kOrbitMinRadius = 8.0f;
static const float kOrbitMaxRadius = 160.0f;

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

// === Phase 8: day/night + battle simulation + combat ======================
// Day/night mode: just two booleans toggled by N.  In night mode the
// sky and the ambient are darker and the light direction tilts toward
// the horizon (so the moon is low).
enum class TimeOfDay { Day, Night };

// Battle simulation: a state machine for the auto-siege.  Pressing B
// (start) walks through Inactive -> BridgeUp (the defenders raise the
// drawbridge) -> Defending (archers shoot arrows at the army) ->
// Advance (cannons fire on the walls, attackers cross the bridge once
// it's down) -> End.  P toggles pause; R resets back to Inactive.
enum class BattlePhase { Inactive, BridgeUp, Defending, Advance, End };

// Combat: each archer / defender / army soldier has a tiny health
// pool.  When health reaches 0 they're "dead" and disappear from the
// scene.  Arrows check against defender army soldier AABBs.  Bridge
// breakage is a count of hits remaining.

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

	Shader shaderProgram("gouraud.vert", "gouraud.frag");

	Mesh ground = Primitives::CreatePlane(400.0f, 400.0f, Palette::Grass);

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
	// Phase 8 fix: archers should stand on the FLAT tower top (y =
	// bodyH = 8.0), not on top of the merlons. Merlons sit on the
	// outside of the parapet, so the centre of the tower top is flat
	// at y = bodyH. Previously the y was set to bodyH + parapetH which
	// made the archer float 0.7 m above the flat tower surface.
	const float cornerTowerTopY = 8.0f;             // flat top, below the merlons
	const float gatehouseTowerTopY = 5.0f;
	const float halfCompound = 12.0f;

	std::vector<Archer> archers;
	archers.emplace_back(vec3(12.0f - halfCompound, cornerTowerTopY, 0.0f - halfCompound)); // cornerNW archer
	archers.emplace_back(vec3(12.0f + halfCompound, cornerTowerTopY, 0.0f - halfCompound)); // cornerNE
	archers.emplace_back(vec3(12.0f - halfCompound, cornerTowerTopY, 0.0f + halfCompound)); // cornerSW
	archers.emplace_back(vec3(12.0f + halfCompound, cornerTowerTopY, 0.0f + halfCompound)); // cornerSE
	// Gatehouse flanking towers at (x = 0, z = ±3.75).
	archers.emplace_back(vec3(0.0f, gatehouseTowerTopY, -3.75f));
	archers.emplace_back(vec3(0.0f, gatehouseTowerTopY,  3.75f));

	// Phase 7: archers face the camera (default view is south of the
	// castle looking toward +Z), so each archer yaws 90° to face +Z.
	// The gatehouse archers get the same yaw.
	for (Archer& a : archers) a.SetYaw(90.0f);

	// --- Phase 6: cannon crew + army ---------------------------------
	// 3 crew (one per cannon) plus 15 army in a 5x3 grid behind the
	// cannons.  Phase 8: army / crew now wear the ATTACKER uniform
	// (dark red) to distinguish them from the DEFENDER soldiers
	// (dark blue) that the castle archers + interior guards wear.
	const float crewZOffset = -0.7f;
	Soldier crewLeft (vec3(cannonX - 1.8f, 0.0f,  cannonSpacing + crewZOffset),
	                  Palette::Attacker);
	Soldier crewCentre(vec3(cannonX - 1.8f, 0.0f,  0.0f + crewZOffset),
	                  Palette::Attacker);
	Soldier crewRight(vec3(cannonX - 1.8f, 0.0f, -cannonSpacing + crewZOffset),
	                  Palette::Attacker);

	std::vector<Soldier> army;
	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 5; col++) {
			float x = cannonX - 6.0f - float(row) * 1.5f;   // 5 columns -> 5 * 1.5 = 7.5 m
			float z = (float(col) - 2.0f) * 2.0f;          // centred on z = 0
			army.emplace_back(vec3(x, 0.0f, z), Palette::Attacker);
		}
	}

	// Phase 7: army faces the castle door (default 0° yaw already
	// points them along +X, but make it explicit).  Cannon crew sits
	// next to the cannon's centre facing the same direction.
	for (Soldier& s : army) s.SetYaw(0.0f);
	crewLeft  .SetYaw(0.0f);
	crewCentre.SetYaw(0.0f);
	crewRight .SetYaw(0.0f);

	// --- Phase 8: castle interior defenders ----------------------------
	// A small detachment of defender soldiers (dark blue) standing
	// inside the compound, near the gold crest.  Used as "backup" by
	// the auto-replace logic: when an archer on a tower dies, one of
	// these soldiers walks to the tower and climbs up.
	std::vector<Soldier> defenders;
	const float crestX = 12.0f + 4.0f;     // gold crest inside the compound
	const float crestZ = 0.0f;
	for (int i = 0; i < 4; i++) {
		float a = float(i) * 1.7f;
		defenders.emplace_back(
			vec3(crestX + std::cos(a) * 2.0f, 0.0f, crestZ + std::sin(a) * 2.0f),
			Palette::Defender);
		defenders.back().SetYaw(180.0f);  // face the camera (default south)
	}

	// --- Phase 8: camp tents behind the army ---------------------------
	// 6 tents in two rows behind the army formation.  Canvas roof in
	// TentCloth, wooden base in TentBase.
	std::vector<CampTent> tents;
	for (int r = 0; r < 2; r++) {
		for (int c = 0; c < 3; c++) {
			float tx = cannonX - 14.0f - float(r) * 2.0f;
			float tz = (float(c) - 1.0f) * 5.0f;
			tents.emplace_back(vec3(tx, 0.0f, tz),
			                    /*baseRadius=*/1.5f, /*roofHeight=*/2.2f,
			                    Palette::TentCloth, Palette::TentBase);
		}
	}

	// --- Phase 8: gold crest inside the castle -------------------------
	GoldCrest goldCrest(vec3(crestX, 0.0f, crestZ));

	// --- Phase 8: distant mountain / valley scenery --------------------
	// A ring of low-poly mountains + valleys around the castle so the
	// canvas reads as "infinite" rather than truncated by the ground
	// edge.
	Scenery scenery(vec3(0.0f, 0.0f, 0.0f),
	                /*innerRadius=*/90.0f, /*outerRadius=*/150.0f,
	                /*count=*/48);

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

	// --- The wooden dummy robot inside the castle ------------------------
	Robot robot(vec3(3.0f, 0.0f, 0.0f));

	std::vector<Projectile> projectiles;

	// --- Phase 8: day/night toggle + battle simulation -----------------
	TimeOfDay tod = TimeOfDay::Day;
	BattlePhase battle = BattlePhase::Inactive;
	bool battlePaused = false;
	float battleTimer = 0.0f;       // seconds since B was pressed (or since resume)
	// Per-archer / defender soldier "alive" flags.  True = in scene;
	// false = dead, draw skipped.  When an archer dies a defender
	// soldier walks over and climbs the tower.
	std::vector<bool> archerAlive(archers.size(), true);
	std::vector<bool> defenderAlive(defenders.size(), true);
	std::vector<bool> armyAlive(army.size(), true);
	// Index of the next defender to be promoted to archer when a
	// tower soldier dies.
	int nextReplacementDefender = 0;
	// Cooldown between arrow shots from the archers during the battle
	// sim.  Each archer fires one arrow every `kArcherFirePeriod`
	// seconds while alive and the sim is in Defending or Advance.
	float archerFireTimer = 0.0f;
	static constexpr float kArcherFirePeriod = 1.6f;

	std::vector<Arrow> arrows;

	// Recompute the projection matrix whenever the window is resized.
	auto updateProjection = [&]() -> mat4 {
		int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);
		if (fbw == 0 || fbh == 0) fbw = 1, fbh = 1;
		return perspective(radians(55.0f), float(fbw) / float(fbh), 0.1f, 400.0f);
	};
	mat4 projMatrix = updateProjection();

	GLuint viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
	GLuint projLoc = glGetUniformLocation(shaderProgram.ID, "proj");
	GLuint modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
	GLuint lightDirLoc = glGetUniformLocation(shaderProgram.ID, "lightDir");

	// Phase 8: light direction is recomputed per-frame based on
	// day/night mode (was a single static value before).

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

		// --- Input: N toggles day / night --------------------------------
		static bool nPrev = false;
		bool nNow = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
		if (nNow && !nPrev) {
			tod = (tod == TimeOfDay::Day) ? TimeOfDay::Night : TimeOfDay::Day;
		}
		nPrev = nNow;

		// --- Input: B starts the battle simulation -----------------------
		static bool bPrev = false;
		bool bNow = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
		if (bNow && !bPrev && battle == BattlePhase::Inactive) {
			battle = BattlePhase::BridgeUp;
			battleTimer = 0.0f;
			battlePaused = false;
		}
		bPrev = bNow;

		// --- Input: P toggles pause on the battle sim --------------------
		static bool pPrev = false;
		bool pNow = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
		if (pNow && !pPrev && battle != BattlePhase::Inactive) {
			battlePaused = !battlePaused;
		}
		pPrev = pNow;

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

		// --- Input: R (HOLD) raises the drawbridge (R for "raise") ------
		bool rNow = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
		castle.SetBridgeRaised(rNow);

		// --- Input: T restarts the battle sim (R was taken by bridge) -
		static bool tPrev = false;
		bool tNow = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
		if (tNow && !tPrev && battle != BattlePhase::Inactive) {
			battle = BattlePhase::Inactive;
			battleTimer = 0.0f;
			battlePaused = false;
		}
		tPrev = tNow;

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
			// Phase 8: stop on contact with any non-breakable stone
			// (curtain wall body, corner tower, gatehouse tower).
			// Previously the ball would happily pass through these
			// because only the door + FortGate bricks were in the
			// collision list.
			if (castle.HitsStatic(ball.GetPosition(), ball.GetRadius())) {
				ball.Kill();
			}
		}
		projectiles.erase(
			std::remove_if(projectiles.begin(), projectiles.end(),
				[](const Projectile& b) { return b.IsDead(); }),
			projectiles.end());

		// --- Update castle (advances door break physics + bridge retract)
		castle.Update(deltaTime);

		// --- Phase 8: battle simulation + combat ------------------------
		if (battle != BattlePhase::Inactive && !battlePaused) {
			battleTimer += deltaTime;
		}
		// Bridge: during the BattleUp phase the defenders raise the
		// drawbridge.  During Advance, the bridge is dropped so the
		// attackers can cross.  When Inactive we leave the bridge
		// alone (the user keeps manual control with R).
		if (battle == BattlePhase::BridgeUp) {
			castle.SetBridgeRaised(true);
			// After 2 seconds the archers start shooting.
			if (battleTimer > 2.0f) {
				battle = BattlePhase::Defending;
				battleTimer = 0.0f;
			}
		} else if (battle == BattlePhase::Defending) {
			castle.SetBridgeRaised(true);
			// 12 seconds of archers shooting arrows at the army.
			if (battleTimer > 12.0f) {
				battle = BattlePhase::Advance;
				battleTimer = 0.0f;
				castle.SetBridgeRaised(false);
			}
		} else if (battle == BattlePhase::Advance) {
			castle.SetBridgeRaised(false);
			// 15 seconds of advancing then End.
			if (battleTimer > 15.0f) {
				battle = BattlePhase::End;
				battleTimer = 0.0f;
				// Decide the winner by who's still standing.
				int livingAttackers = 0;
				for (bool a : armyAlive) if (a) ++livingAttackers;
				int livingDefenders = 0;
				for (bool a : archerAlive) if (a) ++livingDefenders;
				for (bool d : defenderAlive) if (d) ++livingDefenders;
				goldCrest.SetVictorious(livingAttackers > livingDefenders);
			}
		} else if (battle == BattlePhase::End) {
			// Sit on the End state until the user presses T to restart.
		}

		// Archer arrow fire: every kArcherFirePeriod seconds each
		// living archer fires one arrow at the closest living army
		// soldier.
		if (battle != BattlePhase::Inactive && !battlePaused &&
		    (battle == BattlePhase::Defending || battle == BattlePhase::Advance)) {
			archerFireTimer += deltaTime;
			if (archerFireTimer >= kArcherFirePeriod) {
				archerFireTimer = 0.0f;
				for (size_t ai = 0; ai < archers.size(); ai++) {
					if (!archerAlive[ai]) continue;
					// Find closest living army soldier.
					int bestIdx = -1;
					float bestD2 = 1e9f;
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
					// Aim: simple ballistic - launch at fixed speed and
					// solve the angle.  We just use a fixed 30° elevation
					// plus a horizontal aim, which is good enough for the
					// "feel" the user is asking for.
					vec3 toT = target - origin;
					float horiz = length(vec2(toT.x, toT.z));
					float vy = 5.0f;
					float vh = 16.0f;
					vec3 vhVec = (horiz > 1e-3f)
						? vec3(toT.x, 0.0f, toT.z) / horiz * vh
						: vec3(0.0f, 0.0f, 0.0f);
					arrows.emplace_back(origin, vhVec + vec3(0.0f, vy, 0.0f));
				}
			}
		}

		// Update arrows + check army hits.
		for (Arrow& a : arrows) {
			a.Update(deltaTime);
			if (a.IsDead()) continue;
			// Sphere-vs-soldier-AABB for each living army soldier.
			vec3 ap = a.GetPosition();
			for (size_t si = 0; si < army.size(); si++) {
				if (!armyAlive[si]) continue;
				vec3 sp = army[si].GetPosition();
				// Soldier AABB: 0.5 x 2.4 x 0.4 centred on sp.
				vec3 d = ap - sp;
				vec3 clamped(
					std::fmax(-0.25f, std::fmin(d.x, 0.25f)),
					std::fmax(-1.20f, std::fmin(d.y, 1.20f)),
					std::fmax(-0.20f, std::fmin(d.z, 0.20f)));
				vec3 delta = d - clamped;
				if (glm::dot(delta, delta) <= 0.04f * 0.04f) {
					armyAlive[si] = false;
					a.Kill();
					break;
				}
			}
		}
		arrows.erase(
			std::remove_if(arrows.begin(), arrows.end(),
				[](const Arrow& a) { return a.IsDead(); }),
			arrows.end());

		// Replacement: if an archer is dead, promote the next
		// available defender.  Visual: defender is teleported to the
		// tower top.
		for (size_t ai = 0; ai < archers.size(); ai++) {
			if (archerAlive[ai]) continue;
			while (nextReplacementDefender < (int)defenders.size() &&
			       !defenderAlive[nextReplacementDefender]) ++nextReplacementDefender;
			if (nextReplacementDefender < (int)defenders.size()) {
				defenders[nextReplacementDefender].SetPosition(archers[ai].GetPosition());
				defenderAlive[nextReplacementDefender] = false;
				archerAlive[ai] = true;
				++nextReplacementDefender;
			}
		}

		// Update gold crest (pulse when victorious).
		goldCrest.Update(deltaTime);

		// --- Camera matrix ----------------------------------------------
		mat4 view = ComputeView();
		projMatrix = updateProjection();

		// --- Draw ----------------------------------------------------------
		// Phase 8: sky colour depends on day/night.
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
		// Light direction: low moon in night, high sun in day.
		vec3 lightDirCurrent = (tod == TimeOfDay::Day)
			? normalize(vec3(-0.4f, -1.0f, -0.5f))
			: normalize(vec3(0.6f, -0.2f, -0.4f));
		glUniform3fv(lightDirLoc, 1, value_ptr(lightDirCurrent));

		mat4 groundMatrix = translate(mat4(1.0f), vec3(0.0f, -0.01f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(groundMatrix));
		ground.Draw();

		// River goes on top of the ground so its surface is visible in
		// the river footprint. Drawn before the castle so the moat can
		// still occlude the bits inside the compound.
		river.Draw(shaderProgram);

		// Distant scenery: drawn before the castle so the castle's
		// towers + walls occlude the parts that fall inside the
		// compound's footprint (otherwise the mountains visually
		// "stick through" the castle).
		scenery.Draw(shaderProgram);

		castle.Draw(shaderProgram);
		robot.Draw(shaderProgram);

		// Archers stand on top of towers - draw after the castle so
		// they appear in front of any tower silhouette behind them.
		// Phase 8: skip dead archers.
		for (size_t i = 0; i < archers.size(); i++) {
			if (archerAlive[i]) archers[i].Draw(shaderProgram);
		}

		// Army in the back.  Phase 8: skip dead army soldiers.
		for (size_t i = 0; i < army.size(); i++) {
			if (armyAlive[i]) army[i].Draw(shaderProgram);
		}

		// Castle interior defenders.  Skip dead ones.
		for (size_t i = 0; i < defenders.size(); i++) {
			if (defenderAlive[i]) defenders[i].Draw(shaderProgram);
		}

		// Camp tents in the distance.
		for (CampTent& t : tents) t.Draw(shaderProgram);

		// Gold crest inside the castle compound.
		goldCrest.Draw(shaderProgram);

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

		// Visual feedback for selected cannons: a thin yellow parabolic
		// arc + a marker at the predicted hit point.  Phase 8 replaces
		// the old "ground line" with an actual projectile preview
		// computed by stepping the same ballistic equation the real
		// projectile uses (semi-implicit Euler under gravity).  When
		// the simulated ball hits a static wall or the ground, the
		// last point becomes the hit point and a small ring is drawn
		// there so the player can see where the cannonball is going
		// to land before they fire.
		//
		// The arc is drawn as 24 small yellow box segments, each
		// connecting two adjacent sample points along the trajectory.
		// The hit marker is a small bright torus.
		static Mesh aimSegMesh = Primitives::CreateBox(0.04f, 0.04f, 1.0f,
                                                       Palette::Indicator);
		static Mesh aimDotMesh = Primitives::CreateSphere(0.18f, 10, 10,
                                                          Palette::Flash);
		const int kAimSteps = 28;          // segments along the arc
		const float kAimStepDt = 0.08f;   // seconds between samples
		for (int i = 0; i < 3; i++) {
			if (!cannonSelected[i]) continue;
			Cannon* cs[3] = { &leftCannon, &centreCannon, &rightCannon };
			vec3 muzzle = cs[i]->GetMuzzleWorldPosition();
			vec3 fwd    = cs[i]->GetForwardWorldDirection();
			vec3 vel    = fwd * Projectile::DefaultSpeed;
			vec3 pos    = muzzle;
			// Simulate forward; break on the first frame the
			// simulated ball is below the ground OR inside any static
			// wall AABB.
			vec3 prev = pos;
			bool hitStatic = false;
			for (int s = 0; s < kAimSteps; s++) {
				prev = pos;
				vel.y -= Projectile::Gravity * kAimStepDt;
				pos   += vel * kAimStepDt;
				if (pos.y < 0.0f) {
					pos.y = 0.0f;
					hitStatic = true;
					break;
				}
				if (castle.HitsStatic(pos, Projectile::DefaultRadius)) {
					hitStatic = true;
					break;
				}
			}
			// Draw a small line segment between `prev` and `pos` for
			// each step.  The box mesh is 1 m long along +Z so we
			// translate to the segment midpoint and rotate to align
			// the +Z axis with the (pos - prev) direction.
			for (int s = 1; s < kAimSteps; s++) {
				// Re-simulate step s to get its position; cheaper
				// than storing the full trajectory because the cost
				// is trivial and it keeps the code straightforward.
				vec3 v2 = fwd * Projectile::DefaultSpeed;
				vec3 p2 = muzzle;
				vec3 prev2 = p2;
				for (int t = 0; t < s; t++) {
					prev2 = p2;
					v2.y -= Projectile::Gravity * kAimStepDt;
					p2   += v2 * kAimStepDt;
				}
				if (p2.y < 0.0f || castle.HitsStatic(p2, Projectile::DefaultRadius)) break;
				vec3 mid  = (prev2 + p2) * 0.5f;
				vec3 dvec = p2 - prev2;
				float len = length(dvec);
				if (len < 1e-3f) continue;
				vec3 dirN = dvec / len;
				// AimSegMesh is along +Z; align with dirN.
				float pitch = asin(-dirN.y);
				float yaw   = atan2(dirN.x, dirN.z);
				mat4 segM = translate(mat4(1.0f), mid)
				          * rotate(mat4(1.0f), yaw,   vec3(0.0f, 1.0f, 0.0f))
				          * rotate(mat4(1.0f), pitch, vec3(1.0f, 0.0f, 0.0f))
				          * scale(mat4(1.0f), vec3(1.0f, 1.0f, len));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(segM));
				aimSegMesh.Draw();
			}
			// Hit-point marker: a bright sphere at the last sample
			// (or at the cannon position if the simulation didn't
			// advance at all - shouldn't happen, but be safe).
			mat4 dotM = translate(mat4(1.0f), pos);
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(dotM));
			aimDotMesh.Draw();
		}

		for (Projectile& ball : projectiles) {
			ball.Draw(shaderProgram);
		}

		// Phase 8: arrows in flight (only used during battle sim).
		for (Arrow& a : arrows) a.Draw(shaderProgram);

		// Phase 8: XYZ coordinate map in the bottom-left corner.  A
		// small overlay that always reads world +X (red), +Y (green),
		// +Z (blue) arrows so the player can orient themselves.
		// Drawn LAST (no depth test) so it sits on top of everything
		// else.  We disable depth test, draw the three coloured axes
		// in screen space, then re-enable depth test.
		glDisable(GL_DEPTH_TEST);
		static Mesh coordX = Primitives::CreateCylinder(0.04f, 0.9f, 6,
		                                                glm::vec3(0.85f, 0.15f, 0.15f),
		                                                /*centered=*/false);
		static Mesh coordY = Primitives::CreateCylinder(0.04f, 0.9f, 6,
		                                                glm::vec3(0.20f, 0.80f, 0.20f),
		                                                /*centered=*/false);
		static Mesh coordZ = Primitives::CreateCylinder(0.04f, 0.9f, 6,
		                                                glm::vec3(0.20f, 0.40f, 0.90f),
		                                                /*centered=*/false);
		int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);
		if (fbw > 0 && fbh > 0) {
			// Overlay in screen space: the camera's view matrix is
			// the orbit camera, and the projection is perspective.
			// For a simple overlay we render with a fixed pixel-scale
			// orthographic projection at the bottom-left corner.
			mat4 hudView = mat4(1.0f);    // identity (camera at origin)
			mat4 hudProj = ortho(0.0f, float(fbw), 0.0f, float(fbh),
			                     -1.0f, 1.0f);
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(hudView));
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(hudProj));
			// Draw +X axis (red) along screen X.
			mat4 hudMx = translate(mat4(1.0f),
			                     vec3(40.0f, 40.0f, 0.0f))
			            * rotate(mat4(1.0f), radians(-90.0f), vec3(0.0f, 0.0f, 1.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(hudMx));
			coordX.Draw();
			// Draw +Y axis (green) along screen Y.
			mat4 hudMy = translate(mat4(1.0f),
			                     vec3(40.0f, 40.0f, 0.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(hudMy));
			coordY.Draw();
			// Draw +Z axis (blue) going into the screen - use a
			// diagonal.
			mat4 hudMz = translate(mat4(1.0f),
			                     vec3(40.0f, 40.0f, 0.0f))
			            * rotate(mat4(1.0f), radians(45.0f), vec3(0.0f, 0.0f, 1.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(hudMz));
			coordZ.Draw();
			// Restore the regular view/proj for the next frame.
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projMatrix));
		}
		glEnable(GL_DEPTH_TEST);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ground.Delete();
	river.Delete();
	leftCannon.Delete();
	centreCannon.Delete();
	rightCannon.Delete();
	castle.Delete();
	for (Archer& a : archers) a.Delete();
	crewLeft.Delete();
	crewCentre.Delete();
	crewRight.Delete();
	for (Soldier& s : army) s.Delete();
	for (Soldier& d : defenders) d.Delete();
	for (CampTent& t : tents) t.Delete();
	scenery.Delete();
	goldCrest.Delete();
	robot.Delete();
	for (Projectile& ball : projectiles) ball.Delete();
	for (Arrow& a : arrows) a.Delete();
	shaderProgram.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}