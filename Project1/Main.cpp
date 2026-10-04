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
#include "Tree.h"
#include "Robot.h"

using namespace std;
using namespace glm;

const unsigned int width = 1000;
const unsigned int height = 700;

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "Medieval Cannon - Phase 3", NULL, NULL);
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

	// --- Phase 3: three cannons side by side ----------------------------
	// Centre cannon is keyboard-controlled; the two flankers are parked
	// (their elevations and positions are baked in at construction).
	const float cannonSpacing = 2.5f;     // distance between adjacent cannons (along Z)
	Cannon centreCannon(vec3(0.0f, 0.0f,  0.0f));
	Cannon leftCannon  (vec3(0.0f, 0.0f,  cannonSpacing));
	Cannon rightCannon (vec3(0.0f, 0.0f, -cannonSpacing));

	// --- Phase 3: fort gate (replace the simple Phase-2 wall) ------------
	// Centre cannon fires at the middle of the gate; the gate's doorway is
	// wide enough (2.0 m) that the cannon ball fits even with a small aim
	// error. Width/height/depth picked so the gate has the chunky proportions
	// of a real medieval front wall.
	FortGate wall(vec3(12.0f, 0.0f, 0.0f),
	             /*width=*/5.0f, /*height=*/3.0f, /*depth=*/0.8f,
	             /*gateWidth=*/2.0f, /*rows=*/4);

	// --- Phase 3: a row of trees behind the fort ------------------------
	std::vector<Tree> trees;
	for (int i = 0; i < 6; i++) {
		float x = 18.0f + float(i) * 3.0f;
		float z = -8.0f + float(i % 2) * 4.0f;   // alternate front/back for a less-row look
		trees.emplace_back(vec3(x, 0.0f, z),
		                   /*trunkH=*/2.0f, /*trunkR=*/0.20f,
		                   /*crownH=*/3.0f, /*crownR=*/1.5f);
	}

	// --- Phase 3: the wooden dummy robot inside the fort, past the gate --
	// Gate's bricks sit at x = 11.6 to x = 12.4; the robot is placed
	// further past that, so it stands INSIDE the fort rather than in
	// front of it. A cannon ball aimed through the doorway will hit it.
	Robot robot(vec3(13.0f, 0.0f, 0.0f));

	std::vector<Projectile> projectiles;

	mat4 projMatrix = perspective(radians(50.0f), float(width) / float(height), 0.1f, 100.0f);
	// Camera behind and to the right of the cannons, looking down the
	// centre cannon's barrel toward the fort gate. Three cannons are
	// arranged along Z, so a slightly wider FOV (50 deg instead of 45)
	// keeps the leftmost and rightmost cannons in view.
	mat4 view = lookAt(vec3(-5.0f, 3.0f, 7.0f), vec3(8.0f, 0.7f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

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

		// --- Input: Up/Down elevate the CENTRE cannon only ---------------
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			centreCannon.Elevate(elevationSpeedDegPerSec * deltaTime);
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			centreCannon.Elevate(-elevationSpeedDegPerSec * deltaTime);
		}

		// --- Input: Left/Right drive the CENTRE cannon only ---------------
		float drive = 0.0f;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) drive += driveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  drive -= driveSpeed * deltaTime;
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

		// --- Update projectiles and check the gate -------------------------
		for (Projectile& ball : projectiles) {
			ball.Update(deltaTime, Projectile::Gravity);
			wall.CheckHit(ball.GetPosition(), ball.GetRadius());
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
		wall.Draw(shaderProgram);
		robot.Draw(shaderProgram);

		// Three cannons side by side.
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
	wall.Delete();
	for (Tree& t : trees) t.Delete();
	robot.Delete();
	for (Projectile& ball : projectiles) ball.Delete();
	shaderProgram.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}