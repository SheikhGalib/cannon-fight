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
#include "Projectile.h"
#include "Wall.h"

using namespace std;
using namespace glm;

const unsigned int width = 1000;
const unsigned int height = 700;

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "Medieval Cannon - Phase 2", NULL, NULL);
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

	Carriage carriage;

	Wheel leftWheel(Dim::WheelRadius, Dim::WheelWidth, Dim::SpokeCount,
	                vec3(0.0f, Dim::WheelRadius, -Dim::WheelTrack));
	Wheel rightWheel(Dim::WheelRadius, Dim::WheelWidth, Dim::SpokeCount,
	                 vec3(0.0f, Dim::WheelRadius, Dim::WheelTrack));

	Shaft shaft(vec3(Dim::PivotX, Dim::PivotY, Dim::PivotZ));
	shaft.Elevate(12.0f);

	// --- Phase 2: the breakable wall ---------------------------------------
	// It sits 12 m in front of the gun (positive X), 5 bricks wide and 4
	// bricks tall. Each brick is 0.4 m on a side, so the wall is 2 m x 1.6 m
	// - big enough to be a satisfying target, small enough to break through
	// in a few hits.
	const vec3 wallCentre(12.0f, 0.0f, 0.0f);
	const vec3 brickSize(0.40f, 0.40f, 0.40f);
	Wall wall(wallCentre, /*rows=*/4, /*cols=*/5, brickSize);

	// --- Phase 2: live cannon balls in flight ------------------------------
	std::vector<Projectile> projectiles;

	mat4 projMatrix = perspective(radians(45.0f), float(width) / float(height), 0.1f, 100.0f);
	// Camera behind and to the right of the cannon, looking down the barrel
	// toward the wall. The wall is at +X, so the camera must be at -X to see
	// the cannon and the wall in one shot.
	mat4 view = lookAt(vec3(-4.5f, 2.5f, 5.5f), vec3(8.0f, 0.7f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

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

		// --- Input: Up/Down elevate the barrel, clamped inside Shaft itself ---
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			shaft.Elevate(elevationSpeedDegPerSec * deltaTime);
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			shaft.Elevate(-elevationSpeedDegPerSec * deltaTime);
		}

		// --- Input: Left/Right drive the whole gun ---------------------------
		float drive = 0.0f;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) drive += driveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  drive -= driveSpeed * deltaTime;
		if (drive != 0.0f) {
			carriage.MoveForward(drive);
			leftWheel.Roll(drive);
			rightWheel.Roll(drive);
		}

		// --- Input: Spacebar fires a cannon ball ----------------------------
		// We need the carriage's matrix to be able to compute the world-space
		// muzzle tip and the world-space forward direction. The ball spawns
		// with muzzle speed along that forward direction - this is the
		// "current aim" of the cannon at the instant of fire.
		static bool spacePrev = false;
		bool spaceNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
		if (spaceNow && !spacePrev) {
			mat4 carriageM = carriage.GetMatrix();
			vec3 muzzleWorld = shaft.GetMuzzleWorldPosition(carriageM);
			vec3 forwardWorld = shaft.GetForwardWorldDirection(carriageM);
			projectiles.emplace_back(
				muzzleWorld,
				forwardWorld * Projectile::DefaultSpeed,
				Projectile::DefaultRadius);
		}
		spacePrev = spaceNow;

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		// --- Update: advance balls, check collisions -------------------------
		// First: advance every ball and ask the wall if any of them hit. Then
		// drop the ones that Update() flagged as dead (below the ground or
		// aged out).
		for (Projectile& ball : projectiles) {
			ball.Update(deltaTime, Projectile::Gravity);
			wall.CheckHit(ball.GetPosition(), ball.GetRadius());
		}
		projectiles.erase(
			std::remove_if(projectiles.begin(), projectiles.end(),
				[](const Projectile& b) { return b.IsDead(); }),
			projectiles.end());

		// --- Draw -----------------------------------------------------------
		glClearColor(0.55f, 0.72f, 0.87f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderProgram.Activate();

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projMatrix));
		glUniform3fv(lightDirLoc, 1, value_ptr(lightDir));

		mat4 groundMatrix = translate(mat4(1.0f), vec3(0.0f, -0.01f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(groundMatrix));
		ground.Draw();

		wall.Draw(shaderProgram);

		mat4 carriageMatrix = carriage.GetMatrix();
		carriage.Draw(shaderProgram, mat4(1.0f));
		leftWheel.Draw(shaderProgram, carriageMatrix);
		rightWheel.Draw(shaderProgram, carriageMatrix);
		shaft.Draw(shaderProgram, carriageMatrix);

		for (Projectile& ball : projectiles) {
			ball.Draw(shaderProgram);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ground.Delete();
	carriage.Delete();
	leftWheel.Delete();
	rightWheel.Delete();
	shaft.Delete();
	wall.Delete();
	for (Projectile& ball : projectiles) ball.Delete();
	shaderProgram.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}