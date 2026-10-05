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
#include "SkyClouds.h"
#include "Birds.h"
#include "SignalTower.h"
#include "Arrow.h"
#include "Tree.h"
#include "ShadowMap.h"
#include "ParticleSystem.h"

using namespace std;
using namespace glm;

// === Window / camera globals =================================================
// The window is fullscreen by default on the primary monitor.  Press F11 to
// toggle windowed <-> fullscreen.  The camera is an ORBIT camera around the
// compound centre: drag with the left mouse button to rotate (yaw / pitch),
// and scroll the wheel to zoom in / out.

static const vec3 kCastleCentre(12.0f, 4.0f, 0.0f);
static const float kOrbitMinRadius = 12.0f;
static const float kOrbitMaxRadius = 240.0f;

struct OrbitCamera {
    float yaw   = 0.7f;     // radians around Y, 0 = looking down +X
    float pitch = 0.40f;    // radians above horizon (clamped to (-0.2, 1.4))
    float radius = 55.0f;   // distance from target
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
    // dir is the unit vector pointing from the eye TOWARD the
    // target (i.e. the looking direction).  Eye = target - dir*R
    // sits on the opposite side of dir from the target.  So:
    //   * dir.y > 0  -> camera is below target looking up
    //   * dir.y < 0  -> camera is above target looking down
    // We want the latter, so dir.y = -sp for positive pitch.
    vec3 dir = vec3(cp * cy, -sp, cp * sy);
    vec3 eye = g_cam.target - dir * g_cam.radius;
    return lookAt(eye, g_cam.target, vec3(0.0f, 1.0f, 0.0f));
}

static vec3 ComputeCameraPosition() {
    float cy = cos(g_cam.yaw),  sy = sin(g_cam.yaw);
    float cp = cos(g_cam.pitch), sp = sin(g_cam.pitch);
    vec3 dir = vec3(cp * cy, -sp, cp * sy);
    return g_cam.target - dir * g_cam.radius;
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
// it's down) -> Melee (courtyard hand-to-hand fight) -> End.
enum class BattlePhase { Inactive, BridgeUp, Defending, Advance, Melee, End };

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

	// Shaders: Phong (per-fragment Blinn-Phong + ray-traced shadows) and Gouraud (per-vertex).
	// Phong is active by default. Press 'G' in real time to toggle between them.
	Shader phongShader("lit.vert", "lit.frag");
	Shader gouraudShader("gouraud.vert", "gouraud.frag");
	Shader shadowShader("shadow.vert", "shadow.frag");
	ShadowMap shadowMap;
	shadowMap.Init(2048);
	bool useShadowMap = true;     // REAL-TIME PCF SHADOW MAPPING BY DEFAULT!

	ParticleSystem particleSystem;
	particleSystem.Init();

	bool usePhong = true;         // PHONG SHADING BY DEFAULT!
	bool useRayTracing = false;

	// Light source 3D objects: Sun (day) and Moon (night)
	Mesh sunCore   = Primitives::CreateSphere(4.5f, 20, 20, Palette::Sun);
	Mesh sunCorona = Primitives::CreateSphere(7.0f, 16, 16, glm::vec3(1.0f, 0.96f, 0.60f));
	Mesh moonCore  = Primitives::CreateSphere(4.0f, 20, 20, Palette::Moon);
	std::vector<Mesh> sunRays;
	for (int r = 0; r < 8; r++) {
		sunRays.push_back(Primitives::CreateCylinder(0.25f, 95.0f, 8, glm::vec3(1.0f, 0.94f, 0.50f), /*centered=*/false));
	}

	Mesh ground = Primitives::CreatePlane(400.0f, 400.0f, Palette::Grass);

	// --- Phase 7: a Z-running river in front of the castle -----------
	// The river is now a tall thin strip running along Z, sitting just
	// past the moat on the -X side.  It visually continues the moat
	// outward in both directions (the moat itself is the strip across
	// the west face of the compound).  Drawn AFTER the ground so its
	// surface overwrites the grass in the river footprint.
	//
	// Centre X = 12 - 16 = -4 (one compound-halfX past the western
	// gatehouse).  Width 12 m along X, length 480 m along Z so the
	// river fills the max-zoom-out view in both +Z and -Z
	// directions (half the visible ground plane).
	Water river(vec3(-4.0f, 0.0f, 0.0f), /*sizeX=*/12.0f, /*sizeZ=*/480.0f);

	// --- Phase 5: three cannons lined up BEHIND the moat ----------------
	const float cannonX = -15.0f;                  // ~8 m back from the moat's far bank
	const float cannonSpacing = 2.5f;              // Z spacing between adjacent cannons
	Cannon centreCannon(vec3(cannonX, 0.0f,  0.0f));
	Cannon leftCannon  (vec3(cannonX, 0.0f,  cannonSpacing));
	Cannon rightCannon (vec3(cannonX, 0.0f, -cannonSpacing));

	// Centre cannon aims directly at the wooden door (x = 0, z = 0)
	centreCannon.Elevate(10.0f);

	// Left cannon aims at the front left wall (z = +7.5)
	leftCannon.Elevate(13.0f);
	leftCannon.Yaw(19.0f);

	// Right cannon aims at the front right wall (z = -7.5)
	rightCannon.Elevate(13.0f);
	rightCannon.Yaw(-19.0f);

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
	archers.emplace_back(vec3(12.0f - halfCompound, cornerTowerTopY, 0.0f - halfCompound)); // cornerNW archer [0]
	archers.emplace_back(vec3(12.0f + halfCompound, cornerTowerTopY, 0.0f - halfCompound)); // cornerNE archer [1]
	archers.emplace_back(vec3(12.0f - halfCompound, cornerTowerTopY, 0.0f + halfCompound)); // cornerSW archer [2]
	archers.emplace_back(vec3(12.0f + halfCompound, cornerTowerTopY, 0.0f + halfCompound)); // cornerSE archer [3]
	// Gatehouse flanking towers at (x = 0, z = ±3.75).
	archers.emplace_back(vec3(0.0f, gatehouseTowerTopY, -3.75f));                            // gatehouse NW [4]
	archers.emplace_back(vec3(0.0f, gatehouseTowerTopY,  3.75f));                            // gatehouse SW [5]

	// Phase 7: archers face the camera (default view is south of the
	// castle looking toward +Z), so each archer yaws 90° to face +Z.
	// The gatehouse archers get the same yaw.
	for (Archer& a : archers) a.SetYaw(90.0f);

	// --- Front curtain wall defenders (Update 4) -----------------------
	// 4 defender soldiers standing on the front western curtain wall (corridor y = 2.9, x = 0.8):
	// 2 on the north front wall (z ∈ [-12, -3.75]), 2 on the south front wall (z ∈ [+3.75, +12]).
	// They look out through the battlements facing the cannons (-X, yaw = 180°).
	std::vector<Soldier> wallSoldiers;
	wallSoldiers.emplace_back(vec3(0.8f, 2.9f, -8.5f), Palette::Defender);
	wallSoldiers.emplace_back(vec3(0.8f, 2.9f, -5.5f), Palette::Defender);
	wallSoldiers.emplace_back(vec3(0.8f, 2.9f, +5.5f), Palette::Defender);
	wallSoldiers.emplace_back(vec3(0.8f, 2.9f, +8.5f), Palette::Defender);
	for (Soldier& s : wallSoldiers) s.SetYaw(180.0f); // face the battlefield (-X)
	std::vector<bool> wallSoldierAlive(4, true);

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

	// Fix 1: army and cannon crew face the castle door (+X, yaw = 0.0f).
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

	// --- Phase 8: camp tents moved off the marching path --------------
	// Phase 9: tents used to be a 2x3 block directly behind the army
	// grid, but that puts them in the way of the army's march toward
	// the castle once the door is broken.  Move them out to the
	// -Z flank (z = -28..-16) so the army's column at z = [-4..4]
	// can march unimpeded toward +X.  We also spread them along a
	// longer Z span so they look like a small encampment rather
	// than a tight cluster.
	std::vector<CampTent> tents;
	for (int r = 0; r < 2; r++) {
		for (int c = 0; c < 3; c++) {
			float tx = cannonX - 6.0f + float(r) * 4.0f;     // x = -21 .. -17
			float tz = -16.0f - float(r) * 6.0f
			            - float(c) * 4.0f;                  // z = -28 .. -16
			tents.emplace_back(vec3(tx, 0.0f, tz),
			                    /*baseRadius=*/1.5f, /*roofHeight=*/2.2f,
			                    Palette::TentCloth, Palette::TentBase);
		}
	}

	// --- Phase 8: gold crest inside the castle -------------------------
	GoldCrest goldCrest(vec3(crestX, 0.0f, crestZ));

	// --- Phase 9: distant mountain / valley scenery --------------------
	// Bigger mountains (20..35 m) clustered on the +Z / -Z sides of
	// the scene (where the camera is most likely to look) so the
	// canvas reads as "infinite" rather than truncated by the ground
	// edge.  We push them slightly further out so the camera can
	// never fly into one.
	Scenery scenery(vec3(0.0f, 0.0f, 0.0f),
	                /*innerRadius=*/110.0f, /*outerRadius=*/200.0f,
	                /*count=*/40);

	// --- Phase 9 (extra): moving clouds in the sky --------------------
	// A handful of big cloud blobs above the scene that drift slowly
	// along +X and wrap around, so the sky feels alive.  We keep
	// them at 28..42 m altitude (well above the castle towers at
	// ~18 m) so they're clearly "sky" but still visible in the
	// default camera view (which sits at pitch ≈ 0.4 rad, so the
	// camera-to-sky line is shallow).
	SkyClouds skyClouds(/*numClouds=*/10,
	                     /*xSpan=*/140.0f, /*zSpan=*/140.0f,
	                     /*skyLow=*/28.0f,  /*skyHigh=*/42.0f);

	// --- Phase 9 (extra): a flock of birds circling overhead ----------
	// 6 birds orbiting the castle at ~50 m altitude, with their
	// own radii + angular speeds.  Each bird is two thin "wings"
	// that beat up and down.
	Birds birds(/*numBirds=*/6,
	            /*centre=*/vec3(crestX, 0.0f, crestZ),
	            /*alt=*/50.0f,
	            /*radiusMin=*/70.0f, /*radiusMax=*/110.0f);

	// --- Phase 9 (extra): riverside signal towers ---------------------
	// Small stone watchtowers spaced along the river bank so the
	// max-zoom-out view reads as "fortified river line" instead of
	// "empty water + nothing".  We put towers on BOTH sides of the
	// river at z intervals of 35 m, alternating slightly in height
	// for visual variety.  River centre x = -4, so we place towers
	// at x = -16 (cannon-side bank) and x = +8 (castle-side bank,
	// past the moat).
	std::vector<SignalTower> signalTowers;
	for (int side = 0; side < 2; side++) {
		float towerX = (side == 0) ? -16.0f : +8.0f;
		for (int i = 0; i < 6; i++) {
			float tz = -150.0f + float(i) * 60.0f;   // -150 .. +150
			// Skip towers that sit on or near the bridge
			// (which is at z = 0, x = -4).  If a tower would be
			// inside the moat's z range or under the bridge,
			// drop it.
			if (std::abs(tz) < 15.0f) continue;
			float h = 5.5f + float((i + side) % 3) * 0.8f;   // 5.5..7.1 m
			signalTowers.emplace_back(vec3(towerX, 0.0f, tz), h);
		}
	}

	// --- Phase 9: trees brought back, but ONLY in the safe zones ----
	// Phase 8 removed trees entirely (the user said "remove them
	// where the fight is happening") but Phase 9 explicitly wants
	// trees on the sides and back.  So we scatter trees everywhere
	// EXCEPT in the fight zone between the cannons and the river /
	// castle:
	//
	//   * The fight zone is x in [-22, 8], z in [-15, 15] (covers
	//     the cannons, army, tents, river, moat, gatehouse).
	//   * Outside that rectangle, trees are fine.
	//   * We also keep trees away from the mountain ring (the inner
	//     radius of the scenery is 110 m, so 100 m is safe).
	//
	// Deterministic LCG-style PRNG so the layout is the same every
	// run.
	std::vector<Tree> trees;
	{
		unsigned int seed = 4711u;
		auto rnd = [&]() {
			seed = seed * 1103515245u + 12345u;
			return float((seed >> 8) & 0xFFFFFFu) / float(0xFFFFFFu);
		};
		const int kTreeCount = 70;
		int placed = 0;
		int attempts = 0;
		const int kMaxAttempts = kTreeCount * 10;
		while (placed < kTreeCount && attempts < kMaxAttempts) {
			++attempts;
			// Sample a candidate in a wide ring around the scene
			// centre so we don't get a perfect uniform blob.
			float angle  = rnd() * 6.2831853f;
			float radius = 18.0f + rnd() * 75.0f;        // 18..93 m
			float x = 12.0f + std::cos(angle) * radius;
			float z = 0.0f  + std::sin(angle) * radius;
			// Reject anything in the fight zone.
			if (x > -22.0f && x <  8.0f &&
			    z > -15.0f && z < 15.0f) continue;
			// Vary tree size a little.
			float trunkH = 1.4f + rnd() * 1.0f;        // 1.4..2.4 m
			float trunkR = 0.12f + rnd() * 0.08f;       // 0.12..0.20 m
			float crownH = 1.8f + rnd() * 1.5f;        // 1.8..3.3 m
			float crownR = 0.9f + rnd() * 0.6f;        // 0.9..1.5 m
			trees.emplace_back(vec3(x, 0.0f, z),
			                   trunkH, trunkR, crownH, crownR);
			++placed;
		}
	}

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

	// Original rest positions for restart (T key)
	std::vector<vec3> armyOriginalPos(army.size());
	for (size_t i = 0; i < army.size(); i++) armyOriginalPos[i] = army[i].GetPosition();
	std::vector<vec3> defenderOriginalPos(defenders.size());
	for (size_t i = 0; i < defenders.size(); i++) defenderOriginalPos[i] = defenders[i].GetPosition();

	// Army marching: strictly TWO LINES (max capacity of the drawbridge).
	// Marching in pairs across the bridge, maintaining file separation so
	// no soldier ever walks on water.
	std::vector<int> armyMarchOrder;
	bool armyMarchStarted = false;
	float leadMarchDistance = 0.0f;
	static constexpr float kArmyMarchSpeed = 2.8f;

	// Courtyard melee combat state
	float meleeTimer = 0.0f;
	float meleeHitTimer = 0.0f;

	float archerFireTimer = 0.0f;
	static constexpr float kArcherFirePeriod = 1.6f;

	float cannonAutoFireTimer = 0.0f;
	static constexpr float cannonAutoFirePeriod = 1.2f;
	int   cannonFireStep = 0;

	std::vector<Arrow> arrows;

	// Recompute the projection matrix whenever the window is resized.
	auto updateProjection = [&]() -> mat4 {
		int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);
		if (fbw == 0 || fbh == 0) fbw = 1, fbh = 1;
		return perspective(radians(55.0f), float(fbw) / float(fbh), 0.1f, 400.0f);
	};
	mat4 projMatrix = updateProjection();

	// Lighting and shader uniforms are recomputed per-frame on the active shader (Phong / Gouraud).

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

		// --- Input: G toggles Phong <-> Gouraud shading in real time (Update 5) ---
		static bool gPrev = false;
		bool gNow = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
		if (gNow && !gPrev) {
			usePhong = !usePhong;
			std::cout << "[Shading Mode] " << (usePhong ? "Phong (Per-Fragment)" : "Gouraud (Per-Vertex)") << std::endl;
		}
		gPrev = gNow;

		// --- Input: X or Y toggles Soft PCF Shadow Mapping in real time ---
		static bool xPrev = false, yPrev = false;
		bool xNow = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
		bool yNow = glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS;
		if ((xNow && !xPrev) || (yNow && !yPrev)) {
			useShadowMap = !useShadowMap;
			std::cout << "[Shadow Mapping] " << (useShadowMap ? "Soft PCF Shadows (Enabled)" : "Disabled") << std::endl;
		}
		xPrev = xNow;
		yPrev = yNow;

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
			armyMarchStarted = false;
			leadMarchDistance = 0.0f;
			meleeTimer = 0.0f;
			meleeHitTimer = 0.0f;
			for (size_t i = 0; i < army.size(); i++) {
				army[i].SetPosition(armyOriginalPos[i]);
				army[i].health = 100.0f;
				army[i].SetPitch(0.0f);
				army[i].SetAttackOffset(0.0f);
				army[i].SetYaw(0.0f);
				armyAlive[i] = true;
			}
			for (size_t i = 0; i < defenders.size(); i++) {
				defenders[i].SetPosition(defenderOriginalPos[i]);
				defenders[i].health = 100.0f;
				defenders[i].SetPitch(0.0f);
				defenders[i].SetAttackOffset(0.0f);
				defenders[i].SetYaw(180.0f);
				defenderAlive[i] = true;
			}
			std::fill(wallSoldierAlive.begin(), wallSoldierAlive.end(), true);
			std::fill(archerAlive.begin(), archerAlive.end(), true);
			nextReplacementDefender = 0;
			goldCrest.SetVictorious(false);
			cannonFireStep = 0;
			cannonAutoFireTimer = 0.0f;
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
						particleSystem.EmitSmoke(muzzle, forward, 25);
						particleSystem.EmitSparks(muzzle, forward, 35);
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
			if (castle.CheckHit(ball.GetPosition(), ball.GetRadius())) {
				particleSystem.EmitDebris(ball.GetPosition(), 30);
				particleSystem.EmitSmoke(ball.GetPosition(), vec3(0.0f, 1.0f, 0.0f), 12);
			}
			if (castle.HitsStatic(ball.GetPosition(), ball.GetRadius())) {
				particleSystem.EmitDebris(ball.GetPosition(), 30);
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
		// Bridge: during the BridgeUp phase the defenders raise the
		// drawbridge.  During Advance, the bridge is dropped so the
		// attackers can cross.  When Inactive we leave the bridge
		// alone (the user keeps manual control with R).
		//
		// Phase 9 choreography tweaks:
		//   * Each phase has a clearer cue: BridgeUp plays for a
		//     moment so the player sees the bridge rise before the
		//     fighting starts.
		//   * Defending ends as soon as either side is wiped out, OR
		//     after 12 s (whichever comes first).
		//   * Advance ends as soon as the door is broken OR after
		//     18 s of auto-firing, so the simulation finishes
		//     decisively instead of stalling.
		if (battle == BattlePhase::BridgeUp) {
			castle.SetBridgeRaised(true);
			if (battleTimer > 2.0f) {
				battle = BattlePhase::Defending;
				battleTimer = 0.0f;
			}
		} else if (battle == BattlePhase::Defending) {
			castle.SetBridgeRaised(true);
			int livingAttackers = 0;
			for (bool a : armyAlive) if (a) ++livingAttackers;
			int livingArchers   = 0;
			for (bool a : archerAlive) if (a) ++livingArchers;
			bool attackersWiped = (livingAttackers <= 0);
			bool archersWiped   = (livingArchers <= 0);
			// Defend for 5 seconds, then attackers advance
			if (battleTimer > 5.0f || attackersWiped || archersWiped) {
				battle = BattlePhase::Advance;
				battleTimer = 0.0f;
				castle.SetBridgeRaised(false);
			}
		} else if (battle == BattlePhase::Advance) {
			castle.SetBridgeRaised(false);
			cannonAutoFireTimer += deltaTime;
			if (cannonAutoFireTimer >= 1.2f) {
				cannonAutoFireTimer = 0.0f;
				// One cannon shoots door (centre, 1), one shoots left wall (0), one shoots right wall (2)
				static const int kFireOrder[3] = { 1, 0, 2 };
				FireSequence* seqs[3] = { &fireLeft, &fireCentre, &fireRight };
				for (int attempt = 0; attempt < 3; attempt++) {
					int targetIdx = kFireOrder[(cannonFireStep + attempt) % 3];
					if (seqs[targetIdx]->state == FireState::Idle) {
						seqs[targetIdx]->state = FireState::CrewWalking;
						seqs[targetIdx]->timer = 0.0f;
						cannonFireStep = (cannonFireStep + attempt + 1) % 3;
						break;
					}
				}
			}

			bool doorBroken = (castle.AliveDoorPanelCount() == 0);
			if (battleTimer > 22.0f && !doorBroken) {
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
				leadMarchDistance += kArmyMarchSpeed * deltaTime;
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

				// When lead pair enters the courtyard, transition to Melee combat!
				if (desiredLeadX >= 5.5f) {
					battle = BattlePhase::Melee;
					battleTimer = 0.0f;
					meleeTimer = 0.0f;
					meleeHitTimer = 0.0f;
				}
			}
		} else if (battle == BattlePhase::Melee) {
			meleeTimer += deltaTime;

			// Defenders (blue) charge forward from crest toward attackers
			for (size_t di = 0; di < defenders.size(); di++) {
				if (!defenderAlive[di]) continue;
				vec3 cur = defenders[di].GetPosition();
				if (cur.x > 7.5f) {
					cur.x -= 3.2f * deltaTime;
					defenders[di].SetPosition(cur);
					defenders[di].SetYaw(180.0f);
				}
			}

			// Attackers advance to meet defenders
			for (size_t si = 0; si < army.size(); si++) {
				if (!armyAlive[si]) continue;
				vec3 cur = army[si].GetPosition();
				if (cur.x < 6.5f) {
					cur.x += 2.2f * deltaTime;
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
			meleeHitTimer += deltaTime;
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
					cur.x += 1.5f * deltaTime;
					army[si].SetPosition(cur);
				}
			}
		}

		// Archer arrow fire: every kArcherFirePeriod seconds each
		// living archer fires one arrow at the closest living army
		// soldier. (Stops when door is broken and melee starts).
		if (battle != BattlePhase::Inactive && !battlePaused &&
		    (battle == BattlePhase::Defending || battle == BattlePhase::Advance)) {
			archerFireTimer += deltaTime;
			if (archerFireTimer >= kArcherFirePeriod) {
				archerFireTimer = 0.0f;
				for (size_t ai = 0; ai < archers.size(); ai++) {
					if (!archerAlive[ai]) continue;
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

		// Update arrows + check army hits (soldiers take 3 hits to die).
		for (Arrow& a : arrows) {
			a.Update(deltaTime);
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
			std::remove_if(arrows.begin(), arrows.end(),
				[](const Arrow& a) { return a.IsDead(); }),
			arrows.end());

		// Update 2: If the front wall breaks, the wall soldiers die!
		if (!castle.IsFrontWallPieceAlive(0)) {
			wallSoldierAlive[0] = false;
			wallSoldierAlive[1] = false;
		}
		if (!castle.IsFrontWallPieceAlive(1)) {
			wallSoldierAlive[2] = false;
			wallSoldierAlive[3] = false;
		}

		// Update 3: If a tower breaks, the tower soldier/archer dies!
		for (size_t ai = 0; ai < archers.size(); ai++) {
			if (!castle.IsTowerAlive((int)ai)) {
				archerAlive[ai] = false;
			}
		}

		// Replacement: if an archer is dead on an intact tower, promote the next
		// available defender.  Visual: defender is teleported to the
		// tower top.
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

		// Update gold crest (pulse when victorious).
		goldCrest.Update(deltaTime);

		// Update sky clouds + birds (they drift + flap each frame).
		skyClouds.Update(deltaTime);
		birds.Update(deltaTime);

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

		// Switch between Phong (per-fragment + ray-traced shadows) and Gouraud (per-vertex)
		Shader& activeShader = usePhong ? phongShader : gouraudShader;
		activeShader.Activate();

		GLint modelLoc    = glGetUniformLocation(activeShader.ID, "model");
		GLint viewLoc     = glGetUniformLocation(activeShader.ID, "view");
		GLint projLoc     = glGetUniformLocation(activeShader.ID, "proj");
		GLint lightDirLoc = glGetUniformLocation(activeShader.ID, "lightDir");
		GLint viewPosLoc  = glGetUniformLocation(activeShader.ID, "viewPos");
		GLint ambStrLoc   = glGetUniformLocation(activeShader.ID, "ambientStrength");
		GLint isSunLoc    = glGetUniformLocation(activeShader.ID, "isSun");

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projMatrix));

		vec3 camPos = ComputeCameraPosition();
		if (viewPosLoc != -1) {
			glUniform3fv(viewPosLoc, 1, value_ptr(camPos));
		}

		// Light direction: low moon in night, high sun in day.
		vec3 lightDirCurrent = (tod == TimeOfDay::Day)
			? normalize(vec3(-0.4f, -1.0f, -0.5f))
			: normalize(vec3(0.6f, -0.2f, -0.4f));

		// Lambda to render scene actors and structures for both shadow depth and main pass
		auto renderSceneGeometry = [&](Shader& shader) {
			castle.Draw(shader);
			robot.Draw(shader);
			for (SignalTower& t : signalTowers) t.Draw(shader);
			for (size_t i = 0; i < archers.size(); i++) {
				if (archerAlive[i]) archers[i].Draw(shader);
			}
			for (size_t i = 0; i < wallSoldiers.size(); i++) {
				if (wallSoldierAlive[i]) wallSoldiers[i].Draw(shader);
			}
			for (size_t i = 0; i < army.size(); i++) {
				if (armyAlive[i] || army[i].GetPitch() != 0.0f) army[i].Draw(shader);
			}
			for (size_t i = 0; i < defenders.size(); i++) {
				if (defenderAlive[i] || defenders[i].GetPitch() != 0.0f) defenders[i].Draw(shader);
			}
			for (CampTent& t : tents) t.Draw(shader);
			for (Tree& t : trees) t.Draw(shader);
			goldCrest.Draw(shader);
			crewLeft  .Draw(shader);
			crewCentre.Draw(shader);
			crewRight .Draw(shader);
			leftCannon  .Draw(shader);
			centreCannon.Draw(shader);
			rightCannon .Draw(shader);
			for (Projectile& ball : projectiles) {
				ball.Draw(shader);
			}
		};

		// =====================================================================
		// Pass 1: Real-time Soft Shadow Mapping (2048x2048 FBO)
		// =====================================================================
		mat4 lightSpaceMatrix = shadowMap.ComputeLightSpaceMatrix(lightDirCurrent, kCastleCentre);
		if (useShadowMap && usePhong) {
			shadowMap.BindForWriting();
			glUseProgram(shadowShader.ID);
			glUniformMatrix4fv(glGetUniformLocation(shadowShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));

			renderSceneGeometry(shadowShader);

			int fbw, fbh;
			glfwGetFramebufferSize(window, &fbw, &fbh);
			shadowMap.Unbind(fbw, fbh);
		}

		// =====================================================================
		// Pass 2: Main Render Pass (Hemispheric Ambient, PCF, Multi-Lights, Materials)
		// =====================================================================
		glUseProgram(activeShader.ID);

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projMatrix));
		glUniform3fv(lightDirLoc, 1, value_ptr(lightDirCurrent));
		if (viewPosLoc != -1) {
			glUniform3fv(viewPosLoc, 1, value_ptr(camPos));
		}

		float ambStr = (tod == TimeOfDay::Day) ? 0.35f : 0.15f;
		if (ambStrLoc != -1) {
			glUniform1f(ambStrLoc, ambStr);
		}
		if (isSunLoc != -1) {
			glUniform1i(isSunLoc, 0);
		}

		// Bind shadow map texture to slot 1
		shadowMap.BindDepthTexture(GL_TEXTURE1);
		glUniform1i(glGetUniformLocation(activeShader.ID, "shadowMap"), 1);
		glUniform1i(glGetUniformLocation(activeShader.ID, "useShadowMap"), useShadowMap ? 1 : 0);
		glUniformMatrix4fv(glGetUniformLocation(activeShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));

		// Multi-Light System: Point Lights (gatehouse torches, signal towers, cannon muzzle flashes)
		std::vector<vec3> pLightPos;
		std::vector<vec3> pLightCol;

		float flickerA = 0.85f + 0.15f * std::sin(float(glfwGetTime()) * 11.0f);
		float flickerB = 0.85f + 0.15f * std::sin(float(glfwGetTime()) * 13.5f + 1.4f);
		float flameIntensity = (tod == TimeOfDay::Night ? 2.2f : 1.0f);

		// Castle gatehouse braziers
		pLightPos.push_back(vec3(0.0f, 5.8f, -3.75f));
		pLightCol.push_back(vec3(1.0f, 0.55f, 0.15f) * flickerA * flameIntensity);
		pLightPos.push_back(vec3(0.0f, 5.8f, +3.75f));
		pLightCol.push_back(vec3(1.0f, 0.55f, 0.15f) * flickerB * flameIntensity);

		// Riverside signal tower braziers
		if (signalTowers.size() >= 2) {
			pLightPos.push_back(vec3(-16.0f, 6.2f, -30.0f));
			pLightCol.push_back(vec3(1.0f, 0.50f, 0.12f) * flickerA * flameIntensity);
			pLightPos.push_back(vec3(+8.0f, 6.2f, +30.0f));
			pLightCol.push_back(vec3(1.0f, 0.50f, 0.12f) * flickerB * flameIntensity);
		}

		// Cannon muzzle flashes
		if (fireLeft.state == FireState::CrewReturning || fireLeft.state == FireState::Lighting) {
			pLightPos.push_back(leftCannon.GetMuzzleWorldPosition());
			pLightCol.push_back(vec3(2.5f, 1.8f, 0.5f));
		}
		if (fireCentre.state == FireState::CrewReturning || fireCentre.state == FireState::Lighting) {
			pLightPos.push_back(centreCannon.GetMuzzleWorldPosition());
			pLightCol.push_back(vec3(2.5f, 1.8f, 0.5f));
		}
		if (fireRight.state == FireState::CrewReturning || fireRight.state == FireState::Lighting) {
			pLightPos.push_back(rightCannon.GetMuzzleWorldPosition());
			pLightCol.push_back(vec3(2.5f, 1.8f, 0.5f));
		}

		int numPL = std::min((int)pLightPos.size(), 8);
		glUniform1i(glGetUniformLocation(activeShader.ID, "numPointLights"), numPL);
		if (numPL > 0) {
			glUniform3fv(glGetUniformLocation(activeShader.ID, "pointLightPos"), numPL, value_ptr(pLightPos[0]));
			glUniform3fv(glGetUniformLocation(activeShader.ID, "pointLightColor"), numPL, value_ptr(pLightCol[0]));
		}
		glUniform1i(glGetUniformLocation(activeShader.ID, "isNight"), (tod == TimeOfDay::Night) ? 1 : 0);

		// Upload ray tracing obstacles when using Phong shader
		if (usePhong) {
			GLint rtLoc = glGetUniformLocation(activeShader.ID, "useRayTracing");
			if (rtLoc != -1) glUniform1i(rtLoc, useRayTracing ? 1 : 0);

			const auto& boxes = castle.GetSolidBoxes();
			int numB = std::min((int)boxes.size(), 36);
			GLint numBoxesLoc = glGetUniformLocation(activeShader.ID, "numBoxes");
			if (numBoxesLoc != -1) glUniform1i(numBoxesLoc, numB);

			std::vector<vec3> bMinArr(numB);
			std::vector<vec3> bMaxArr(numB);
			for (int bi = 0; bi < numB; bi++) {
				bMinArr[bi] = boxes[bi].centre - boxes[bi].half;
				bMaxArr[bi] = boxes[bi].centre + boxes[bi].half;
			}
			if (numB > 0) {
				GLint bMinLoc = glGetUniformLocation(activeShader.ID, "boxMin");
				GLint bMaxLoc = glGetUniformLocation(activeShader.ID, "boxMax");
				if (bMinLoc != -1) glUniform3fv(bMinLoc, numB, value_ptr(bMinArr[0]));
				if (bMaxLoc != -1) glUniform3fv(bMaxLoc, numB, value_ptr(bMaxArr[0]));
			}
		}

		// Draw visible light source: Sun with corona & rays (day) or Moon (night)
		if (isSunLoc != -1) glUniform1i(isSunLoc, 1);
		vec3 lightSourcePos = kCastleCentre + (-lightDirCurrent * 120.0f);
		if (tod == TimeOfDay::Day) {
			mat4 sunMat = translate(mat4(1.0f), lightSourcePos);
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(sunMat));
			sunCore.Draw();

			mat4 coronaMat = translate(mat4(1.0f), lightSourcePos);
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(coronaMat));
			sunCorona.Draw();

			// 8 radiant rays around the sun
			for (int r = 0; r < 8; r++) {
				float angle = float(r) * (3.14159265f / 4.0f);
				mat4 rayMat = translate(mat4(1.0f), lightSourcePos)
				            * rotate(mat4(1.0f), angle, vec3(0.0f, 0.0f, 1.0f))
				            * rotate(mat4(1.0f), radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(rayMat));
				sunRays[r].Draw();
			}
		} else {
			mat4 moonMat = translate(mat4(1.0f), lightSourcePos);
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(moonMat));
			moonCore.Draw();
		}
		if (isSunLoc != -1) glUniform1i(isSunLoc, 0);

		mat4 groundMatrix = translate(mat4(1.0f), vec3(0.0f, -0.01f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(groundMatrix));
		ground.Draw();

		river.Draw(activeShader);
		scenery.Draw(activeShader);
		skyClouds.Draw(activeShader);

		// Draw all scene actors & structures
		renderSceneGeometry(activeShader);

		// Dynamic FX: Smoke, sparks, debris
		particleSystem.Update(deltaTime);
		particleSystem.Draw(activeShader, view, projMatrix);

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



		// Phase 8: arrows in flight (only used during battle sim).
		for (Arrow& a : arrows) a.Draw(activeShader);

		// Phase 9 (extra): birds circling overhead.  Drawn late so
		// they sit on top of the sky background (no z-fighting
		// issues because they're small + far above the scene).
		birds.Draw(activeShader);

		// Phase 9: XYZ coordinate map as a SIDE panel in the
		// bottom-right corner.  Drawn LAST (no depth test) so it sits
		// on top of everything else.
		glDisable(GL_DEPTH_TEST);
		static Mesh coordX = Primitives::CreateCylinder(0.05f, 1.4f, 6,
		                                                glm::vec3(0.90f, 0.20f, 0.20f),
		                                                /*centered=*/false);
		static Mesh coordY = Primitives::CreateCylinder(0.05f, 1.4f, 6,
		                                                glm::vec3(0.25f, 0.85f, 0.25f),
		                                                /*centered=*/false);
		static Mesh coordZ = Primitives::CreateCylinder(0.05f, 1.4f, 6,
		                                                glm::vec3(0.25f, 0.45f, 0.95f),
		                                                /*centered=*/false);
		// Tiny tip cones so the gizmo looks like an "arrow" rather
		// than a plain stick.
		static Mesh tipX = Primitives::CreateCone(0.13f, 0.0f, 0.30f, 10,
		                                          glm::vec3(0.90f, 0.20f, 0.20f),
		                                          /*centered=*/false);
		static Mesh tipY = Primitives::CreateCone(0.13f, 0.0f, 0.30f, 10,
		                                          glm::vec3(0.25f, 0.85f, 0.25f),
		                                          /*centered=*/false);
		static Mesh tipZ = Primitives::CreateCone(0.13f, 0.0f, 0.30f, 10,
		                                          glm::vec3(0.25f, 0.45f, 0.95f),
		                                          /*centered=*/false);
		int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);
		if (fbw > 0 && fbh > 0) {
			if (isSunLoc != -1) glUniform1i(isSunLoc, 1);
			// Overlay in screen space with an ortho projection.
			mat4 hudView = mat4(1.0f);
			mat4 hudProj = ortho(0.0f, float(fbw), 0.0f, float(fbh),
			                     -1.0f, 1.0f);
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(hudView));
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(hudProj));

			// Panel anchor: bottom-right corner of the canvas, with
			// a small margin so the gizmo doesn't sit on the very
			// edge.
			const float cx = float(fbw) - 130.0f;
			const float cy = 130.0f;
			const float ax = cx - 30.0f;   // x-axis tip target
			const float ay = cy;          // y-axis tip target (vertical)
			const float az_x = cx + 30.0f; // z-axis tip target (diagonal down-right)
			const float az_y = cy - 30.0f;

			// +X axis (red) - horizontal, pointing to the right.
			// Cylinder grows along +Y locally; rotate +90 around Z
			// so its top ends up at +X.
			mat4 hudMx = translate(mat4(1.0f),
			                     vec3(cx, cy, 0.0f))
			            * rotate(mat4(1.0f), radians(90.0f), vec3(0.0f, 0.0f, 1.0f))
			            * scale(mat4(1.0f), vec3(1.0f, std::fabs(ax - cx), 1.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(hudMx));
			coordX.Draw();
			// +X arrow tip at (ax, cy).
			mat4 hudMxTip = translate(mat4(1.0f),
			                          vec3(ax, cy, 0.0f))
			                 * rotate(mat4(1.0f), radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(hudMxTip));
			tipX.Draw();

			// +Y axis (green) - vertical, pointing up.  Cylinder grows from
			// local origin to local +Y so we just translate to the
			// base and scale by the desired length.
			mat4 hudMy = translate(mat4(1.0f),
			                     vec3(cx, cy, 0.0f))
			            * scale(mat4(1.0f), vec3(1.0f, std::fabs(ay - cy), 1.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(hudMy));
			coordY.Draw();
			// +Y arrow tip at (cx, ay).
			mat4 hudMyTip = translate(mat4(1.0f),
			                          vec3(cx, ay, 0.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(hudMyTip));
			tipY.Draw();

			// +Z axis (blue) - diagonal, pointing to the
			// bottom-right of the gizmo (the conventional "into the
			// screen" direction on a top-down map).  Cylinder grows
			// along +Y locally; we rotate around Z so its +Y points
			// toward (az_x, az_y).
			{
				float zdx = az_x - cx;
				float zdy = az_y - cy;
				float zlen = std::sqrt(zdx * zdx + zdy * zdy);
				float zang = std::atan2(zdx, zdy);  // angle from +Y
				mat4 hudMz = translate(mat4(1.0f),
				                     vec3(cx, cy, 0.0f))
				            * rotate(mat4(1.0f), zang, vec3(0.0f, 0.0f, 1.0f))
				            * scale(mat4(1.0f), vec3(1.0f, zlen, 1.0f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(hudMz));
				coordZ.Draw();
				// Tip at the +Z end.
				mat4 hudMzTip = translate(mat4(1.0f),
				                          vec3(az_x, az_y, 0.0f))
				                 * rotate(mat4(1.0f), zang, vec3(0.0f, 0.0f, 1.0f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(hudMzTip));
				tipZ.Draw();
			}

			// A small label strip below the gizmo so the player
			// knows what they're looking at.
			static Mesh anchor = Primitives::CreateSphere(0.10f, 8, 8,
			                                              glm::vec3(1.0f, 1.0f, 1.0f));
			mat4 hudMa = translate(mat4(1.0f), vec3(cx, cy, 0.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(hudMa));
			anchor.Draw();

			// Restore the regular view/proj for the next frame.
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projMatrix));
			if (isSunLoc != -1) glUniform1i(isSunLoc, 0);
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
	for (Soldier& s : wallSoldiers) s.Delete();
	crewLeft.Delete();
	crewCentre.Delete();
	crewRight.Delete();
	for (Soldier& s : army) s.Delete();
	for (Soldier& d : defenders) d.Delete();
	for (CampTent& t : tents) t.Delete();
	for (Tree& t : trees) t.Delete();
	scenery.Delete();
	goldCrest.Delete();
	skyClouds.Delete();
	birds.Delete();
	for (SignalTower& t : signalTowers) t.Delete();
	robot.Delete();
	for (Projectile& ball : projectiles) ball.Delete();
	for (Arrow& a : arrows) a.Delete();
	sunCore.Delete();
	sunCorona.Delete();
	moonCore.Delete();
	for (Mesh& m : sunRays) m.Delete();
	phongShader.Delete();
	gouraudShader.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}