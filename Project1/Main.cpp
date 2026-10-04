#include <iostream>
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

using namespace std;
using namespace glm;

const unsigned int width = 1000;
const unsigned int height = 700;

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "Medieval Cannon - Phase 1", NULL, NULL);
	if (window == NULL) {
		cout << "Failed to create window!" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	gladLoadGL();
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);

	// lit.vert/lit.frag add a single directional light (ambient floor +
	// Lambertian diffuse) on top of the flat per-vertex colors, so shapes
	// read as solid 3D forms instead of flat silhouettes. See lit.frag for
	// why this is a deliberately small slice of Phase 2's full lighting
	// requirement, pulled forward early because it turned out to matter
	// this much for legibility even in Phase 1.
	Shader shaderProgram("lit.vert", "lit.frag");

	// --- Build the scene ----------------------------------------------------
	// The cannon is a small scene graph with the CARRIAGE at its root:
	//
	//     Carriage                      <- the wooden body; moving it moves everything
	//       |-- Wheel (left)  at Z = -track    <- rolls about the axle
	//       |-- Wheel (right) at Z = +track
	//       +-- Shaft (barrel) at the pivot    <- tips up and down
	//
	// Wheels and barrel are drawn with the carriage's matrix as their parent,
	// which is what holds the gun together as one object: nothing here has to
	// re-state where anything is in the world.
	//
	// All the actual measurements live in Dimensions.h and all the colors in
	// Palette.h, so this file stays a description of the SCENE, not of shapes.

	// Big enough that its far edge falls outside the view, so what you see at
	// the top of the screen reads as a horizon rather than as the end of a mat.
	Mesh ground = Primitives::CreatePlane(120.0f, 120.0f, Palette::Grass);

	Carriage carriage;

	// Both wheels sit on the axle: centred at wheel-radius height (so they
	// just touch the ground) and one on each side of the centreline.
	Wheel leftWheel(Dim::WheelRadius, Dim::WheelWidth, Dim::SpokeCount,
	                vec3(0.0f, Dim::WheelRadius, -Dim::WheelTrack));
	Wheel rightWheel(Dim::WheelRadius, Dim::WheelWidth, Dim::SpokeCount,
	                 vec3(0.0f, Dim::WheelRadius, Dim::WheelTrack));

	// The barrel hangs on its trunnions, up between the carriage cheeks.
	Shaft shaft(vec3(Dim::PivotX, Dim::PivotY, Dim::PivotZ));
	shaft.Elevate(12.0f); // start tipped up a little, so the pivot is visible

	// --- Fixed camera, looking at the gun from the front-right --------------
	// (No camera controls yet - Phase 2 adds those. See docs/phase-2-plan.md.)
	mat4 view = lookAt(vec3(4.8f, 2.7f, 5.5f), vec3(0.05f, 0.70f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 proj = perspective(radians(45.0f), float(width) / float(height), 0.1f, 100.0f);

	GLuint viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
	GLuint projLoc = glGetUniformLocation(shaderProgram.ID, "proj");
	GLuint modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
	GLuint lightDirLoc = glGetUniformLocation(shaderProgram.ID, "lightDir");

	// A fixed "sun" direction: mostly downward, angled from front-left, so
	// every shape picks up a clear bright side and a clear shaded side.
	vec3 lightDir = normalize(vec3(-0.4f, -1.0f, -0.5f));

	const float elevationSpeedDegPerSec = 30.0f;
	const float driveSpeed = 2.0f; // metres per second
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
		// The carriage moves and the wheels are told the same distance, so they
		// roll exactly as far as the ground passes under them - no slipping.
		float drive = 0.0f;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) drive += driveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  drive -= driveSpeed * deltaTime;
		if (drive != 0.0f) {
			carriage.MoveForward(drive);
			leftWheel.Roll(drive);
			rightWheel.Roll(drive);
		}

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		// --- Draw ---------------------------------------------------------
		glClearColor(0.55f, 0.72f, 0.87f, 1.0f); // sky blue
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderProgram.Activate();

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(proj));
		glUniform3fv(lightDirLoc, 1, value_ptr(lightDir));

		// The ground sits a hair below y = 0 so that parts resting exactly on
		// the ground (the trail spade, the wheel rims) don't fight it for depth.
		mat4 groundMatrix = translate(mat4(1.0f), vec3(0.0f, -0.01f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(groundMatrix));
		ground.Draw();

		// Everything on the gun is drawn relative to the carriage.
		mat4 carriageMatrix = carriage.GetMatrix();
		carriage.Draw(shaderProgram, mat4(1.0f));
		leftWheel.Draw(shaderProgram, carriageMatrix);
		rightWheel.Draw(shaderProgram, carriageMatrix);
		shaft.Draw(shaderProgram, carriageMatrix);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ground.Delete();
	carriage.Delete();
	leftWheel.Delete();
	rightWheel.Delete();
	shaft.Delete();
	shaderProgram.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
