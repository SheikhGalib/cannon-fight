#include <iostream>
#include <vector>
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
#include "Wall.h"
#include "FortGate.h"
#include "CompoundCastle.h"
#include "Tower.h"
#include "Door.h"
#include "Tree.h"
#include "Robot.h"

using namespace std;
using namespace glm;

const unsigned int width = 1000;
const unsigned int height = 700;

// Four camera presets.  Press 1/2/3/4 to switch.
//   1: behind the cannons (default) - looking down the barrels at the gate
//   2: in front of the castle     - looking back at the cannons across the moat
//   3: side view                  - looking sideways at the whole compound
//   4: top-down                   - looking straight down at the scene
struct CameraPreset {
    vec3 eye;
    vec3 target;
    const char* label;
};

static const CameraPreset kPresets[4] = {
    { vec3(-18.0f,  5.5f,  8.0f), vec3( 12.0f, 2.5f,  0.0f), "Behind cannons" },
    { vec3( 32.0f,  9.0f, 22.0f), vec3(-18.0f, 2.5f,  0.0f), "Front of castle" },
    { vec3( 12.0f,  8.0f, 32.0f), vec3( 12.0f, 1.0f,  0.0f), "Side view" },
    { vec3( 12.0f, 55.0f,  0.5f), vec3( 12.0f, 0.0f,  0.0f), "Top-down" },
};

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "Medieval Cannon - Phase 5", NULL, NULL);
	if (window == NULL) {
		cout << "Failed to create window!" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	gladLoadGL();
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);

	Shader shaderProgram("lit.vert", "lit.frag");

	Mesh ground = Primitives::CreatePlane(120.0f, 120.0f, Palette::Grass);

	// --- Phase 5: three cannons lined up BEHIND the moat ----------------
	// The compound centre is at (12, 0, 0).  The moat occupies x ∈ [-7, -1]
	// and the bridge crosses it at z = 0.  We want the cannons a few
	// metres beyond the moat on the -X side, all facing the +X direction
	// so they look straight at the gate from across the moat (this is the
	// "cannons first, facing the castle up front" layout from the brief).
	const float cannonX = -10.0f;                  // 3 m beyond the moat's far bank
	const float cannonSpacing = 2.5f;              // Z spacing between adjacent cannons
	Cannon centreCannon(vec3(cannonX, 0.0f,  0.0f));
	Cannon leftCannon  (vec3(cannonX, 0.0f,  cannonSpacing));
	Cannon rightCannon (vec3(cannonX, 0.0f, -cannonSpacing));

	// Default angle: every Cannon's barrel points along +X.  The constructor
	// tips the barrel up 12 deg (Phase 3 default), so we lower it a bit
	// so the default arc drops balls onto the door (3 m tall, ~9 m away
	// from the muzzle at x=-8).  Net elevation: ~10 deg.
	centreCannon.Elevate(-2.0f);   // 12 -> 10 deg total

	// --- Phase 5: full compound castle (4 corners + curtains + gate) ---
	// centreWorld = (12, 0, 0), so the -X face (where the gate lives) is
	// at x = 0.  The moat is a 6x12 strip centred at x = -4, the bridge
	// is 6 m long centred at x = -3.
	CompoundCastle castle(vec3(12.0f, 0.0f, 0.0f));

	// --- A few trees scattered around the scene -------------------------
	// We have a 24x24 m compound centred at (12, 0, 0), so there's room
	// along the +X side and a bit of the +/-Z flanks to put trees.  Avoid
	// the moat footprint (x ∈ [-7, -1]) and the cannon line (x ≈ -10).
	std::vector<Tree> trees;
	// Right side of the compound (+X face) - back where the trees used to be.
	for (int i = 0; i < 6; i++) {
		float x = 28.0f + float(i) * 3.0f;
		float z = -8.0f + float(i % 2) * 4.0f;
		trees.emplace_back(vec3(x, 0.0f, z),
		                   /*trunkH=*/2.0f, /*trunkR=*/0.20f,
		                   /*crownH=*/3.0f, /*crownR=*/1.5f);
	}
	// Some trees behind the cannons on the -X side.
	for (int i = 0; i < 4; i++) {
		float z = -10.0f + float(i) * 6.0f;
		trees.emplace_back(vec3(-15.0f, 0.0f, z),
		                   2.0f, 0.20f, 3.0f, 1.5f);
	}

	// --- The wooden dummy robot inside the castle ------------------------
	// Compound centre is at (12, 0, 0).  Gate is at x=0, door at x ≈ -0.4
	// (i.e. a little past the gatehouse towers).  The robot stands a few
	// metres inside the gate, at x = 3.
	Robot robot(vec3(3.0f, 0.0f, 0.0f));

	std::vector<Projectile> projectiles;

	mat4 projMatrix = perspective(radians(50.0f), float(width) / float(height), 0.1f, 200.0f);
	// Start with preset 0 (behind cannons).  Updated each frame from the
	// current preset slot, so 1/2/3/4 keys can swap the view live.
	int currentPreset = 0;
	mat4 view = lookAt(kPresets[currentPreset].eye,
	                   kPresets[currentPreset].target,
	                   vec3(0.0f, 1.0f, 0.0f));

	GLuint viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
	GLuint projLoc = glGetUniformLocation(shaderProgram.ID, "proj");
	GLuint modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
	GLuint lightDirLoc = glGetUniformLocation(shaderProgram.ID, "lightDir");

	vec3 lightDir = normalize(vec3(-0.4f, -1.0f, -0.5f));

	const float elevationSpeedDegPerSec = 30.0f;
	const float driveSpeed = 2.0f;
	double lastFrameTime = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		double currentTime = glfwGetTime();
		float deltaTime = float(currentTime - lastFrameTime);
		lastFrameTime = currentTime;

		// --- Input: number keys 1-4 swap camera preset -------------------
		static bool key1Prev = false, key2Prev = false, key3Prev = false, key4Prev = false;
		bool key1Now = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
		bool key2Now = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
		bool key3Now = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
		bool key4Now = glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS;
		if (key1Now && !key1Prev) currentPreset = 0;
		if (key2Now && !key2Prev) currentPreset = 1;
		if (key3Now && !key3Prev) currentPreset = 2;
		if (key4Now && !key4Prev) currentPreset = 3;
		key1Prev = key1Now; key2Prev = key2Now; key3Prev = key3Now; key4Prev = key4Now;
		view = lookAt(kPresets[currentPreset].eye,
		              kPresets[currentPreset].target,
		              vec3(0.0f, 1.0f, 0.0f));

		// --- Input: Up/Down elevate the CENTRE cannon only ---------------
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			centreCannon.Elevate(elevationSpeedDegPerSec * deltaTime);
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			centreCannon.Elevate(-elevationSpeedDegPerSec * deltaTime);
		}

		// --- Input: Left/Right drive the CENTRE cannon forward/back along
		// the barrel's local X axis.  With the barrel facing +X (towards
		// the gate) this slides the cannon toward or away from the moat,
		// so the player can fine-tune their firing range.
		float drive = 0.0f;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) drive += driveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) drive -= driveSpeed * deltaTime;
		if (drive != 0.0f) {
			centreCannon.MoveForward(drive);
		}

		// --- Input: Spacebar fires from the CENTRE cannon -----------------
		static bool spacePrev = false;
		bool spaceNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
		if (spaceNow && !spacePrev) {
			vec3 muzzle = centreCannon.GetMuzzleWorldPosition();
			vec3 forward = centreCannon.GetForwardWorldDirection();
			projectiles.emplace_back(
				muzzle,
				forward * Projectile::DefaultSpeed,
				Projectile::DefaultRadius);
		}
		spacePrev = spaceNow;

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

		for (Tree& t : trees) t.Draw(shaderProgram);
		castle.Draw(shaderProgram);
		robot.Draw(shaderProgram);

		// Three cannons side by side on the moat-far side.
		centreCannon.Draw(shaderProgram);
		leftCannon.Draw(shaderProgram);
		rightCannon.Draw(shaderProgram);

		for (Projectile& ball : projectiles) {
			ball.Draw(shaderProgram);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ground.Delete();
	centreCannon.Delete();
	leftCannon.Delete();
	rightCannon.Delete();
	castle.Delete();
	for (Tree& t : trees) t.Delete();
	robot.Delete();
	for (Projectile& ball : projectiles) ball.Delete();
	shaderProgram.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
