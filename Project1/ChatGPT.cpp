#include <iostream>
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaderClass.h"
#include "VBO.h"
#include "EBO.h"
#include "VAO.h"

using namespace std;
using namespace glm;

const unsigned int width = 800;
const unsigned int height = 800;


// ============================================================
// GENERATE SPHERE
// ============================================================

void generateSphere(
	vector<GLfloat>& vertices,
	vector<GLuint>& indices,
	float radius,
	int stacks,
	int sectors)
{
	const float PI = 3.14159265359f;

	for (int i = 0; i <= stacks; i++)
	{
		float stackAngle = PI / 2.0f - i * PI / stacks;

		float xy = radius * cos(stackAngle);
		float z = radius * sin(stackAngle);

		for (int j = 0; j <= sectors; j++)
		{
			float sectorAngle = j * 2.0f * PI / sectors;

			float x = xy * cos(sectorAngle);
			float y = xy * sin(sectorAngle);

			// Position
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);

			// Color
			vertices.push_back(0.2f);
			vertices.push_back(0.8f);
			vertices.push_back(0.8f);
		}
	}


	// Generate sphere indices
	for (int i = 0; i < stacks; i++)
	{
		int k1 = i * (sectors + 1);
		int k2 = k1 + sectors + 1;

		for (int j = 0; j < sectors; j++, k1++, k2++)
		{
			if (i != 0)
			{
				indices.push_back(k1);
				indices.push_back(k2);
				indices.push_back(k1 + 1);
			}

			if (i != stacks - 1)
			{
				indices.push_back(k1 + 1);
				indices.push_back(k2);
				indices.push_back(k2 + 1);
			}
		}
	}
}



int main()
{
	// ============================================================
	// INITIALIZE GLFW
	// ============================================================

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(
		GLFW_OPENGL_PROFILE,
		GLFW_OPENGL_CORE_PROFILE
	);



	// ============================================================
	// CUBE DATA
	// ============================================================

	GLfloat cubeVertices[] = {

		// Position              // Color
		// x      y      z        r     g     b

		-0.5f, -0.5f,  0.5f,     1.0f, 0.0f, 0.0f, // 0
		 0.5f, -0.5f,  0.5f,     0.0f, 1.0f, 0.0f, // 1
		 0.5f,  0.5f,  0.5f,     0.0f, 0.0f, 1.0f, // 2
		-0.5f,  0.5f,  0.5f,     1.0f, 1.0f, 0.0f, // 3

		-0.5f, -0.5f, -0.5f,     1.0f, 0.0f, 1.0f, // 4
		 0.5f, -0.5f, -0.5f,     0.0f, 1.0f, 1.0f, // 5
		 0.5f,  0.5f, -0.5f,     1.0f, 1.0f, 1.0f, // 6
		-0.5f,  0.5f, -0.5f,     0.5f, 0.5f, 0.5f  // 7
	};


	GLuint cubeIndices[] = {

		// Front
		0, 1, 2,
		2, 3, 0,

		// Right
		1, 5, 6,
		6, 2, 1,

		// Back
		5, 4, 7,
		7, 6, 5,

		// Left
		4, 0, 3,
		3, 7, 4,

		// Top
		3, 2, 6,
		6, 7, 3,

		// Bottom
		4, 5, 1,
		1, 0, 4
	};



	// ============================================================
	// PYRAMID DATA
	// ============================================================

	GLfloat pyramidVertices[] = {

		// Position              // Color

		-0.5f, -0.5f,  0.5f,     1.0f, 0.0f, 0.0f, // 0
		 0.5f, -0.5f,  0.5f,     0.0f, 1.0f, 0.0f, // 1
		 0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 1.0f, // 2
		-0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 0.0f, // 3

		 0.0f,  0.5f,  0.0f,     1.0f, 0.0f, 1.0f  // 4
	};


	GLuint pyramidIndices[] = {

		// Four sides
		0, 1, 4,
		1, 2, 4,
		2, 3, 4,
		3, 0, 4,

		// Bottom
		0, 3, 2,
		0, 2, 1
	};



	// ============================================================
	// SPHERE DATA
	// ============================================================

	vector<GLfloat> sphereVertices;
	vector<GLuint> sphereIndices;

	generateSphere(
		sphereVertices,
		sphereIndices,
		0.5f,       // Radius
		20,         // Stacks
		20          // Sectors
	);



	// ============================================================
	// CREATE WINDOW
	// ============================================================

	GLFWwindow* window =
		glfwCreateWindow(
			width,
			height,
			"Cube Sphere Pyramid",
			NULL,
			NULL
		);

	if (window == NULL)
	{
		cout << "Failed to create window!" << endl;

		glfwTerminate();

		return -1;
	}

	glfwMakeContextCurrent(window);



	// ============================================================
	// LOAD GLAD
	// ============================================================

	gladLoadGL();

	glViewport(
		0,
		0,
		width,
		height
	);



	// ============================================================
	// SHADER
	// ============================================================

	Shader shaderProgram(
		"default.vert",
		"default.frag"
	);



	// ============================================================
	// CUBE VAO, VBO, EBO
	// ============================================================

	VAO cubeVAO;
	cubeVAO.Bind();

	VBO cubeVBO(
		cubeVertices,
		sizeof(cubeVertices)
	);

	EBO cubeEBO(
		cubeIndices,
		sizeof(cubeIndices)
	);


	cubeVAO.LinkAttrib(
		cubeVBO,
		0,
		3,
		GL_FLOAT,
		6 * sizeof(GLfloat),
		(void*)0
	);


	cubeVAO.LinkAttrib(
		cubeVBO,
		1,
		3,
		GL_FLOAT,
		6 * sizeof(GLfloat),
		(void*)(3 * sizeof(GLfloat))
	);


	cubeVAO.Unbind();
	cubeVBO.Unbind();
	cubeEBO.Unbind();



	// ============================================================
	// PYRAMID VAO, VBO, EBO
	// ============================================================

	VAO pyramidVAO;
	pyramidVAO.Bind();

	VBO pyramidVBO(
		pyramidVertices,
		sizeof(pyramidVertices)
	);

	EBO pyramidEBO(
		pyramidIndices,
		sizeof(pyramidIndices)
	);


	pyramidVAO.LinkAttrib(
		pyramidVBO,
		0,
		3,
		GL_FLOAT,
		6 * sizeof(GLfloat),
		(void*)0
	);


	pyramidVAO.LinkAttrib(
		pyramidVBO,
		1,
		3,
		GL_FLOAT,
		6 * sizeof(GLfloat),
		(void*)(3 * sizeof(GLfloat))
	);


	pyramidVAO.Unbind();
	pyramidVBO.Unbind();
	pyramidEBO.Unbind();



	// ============================================================
	// SPHERE VAO, VBO, EBO
	// ============================================================

	VAO sphereVAO;
	sphereVAO.Bind();

	VBO sphereVBO(
		sphereVertices.data(),
		sphereVertices.size() * sizeof(GLfloat)
	);

	EBO sphereEBO(
		sphereIndices.data(),
		sphereIndices.size() * sizeof(GLuint)
	);


	sphereVAO.LinkAttrib(
		sphereVBO,
		0,
		3,
		GL_FLOAT,
		6 * sizeof(GLfloat),
		(void*)0
	);


	sphereVAO.LinkAttrib(
		sphereVBO,
		1,
		3,
		GL_FLOAT,
		6 * sizeof(GLfloat),
		(void*)(3 * sizeof(GLfloat))
	);


	sphereVAO.Unbind();
	sphereVBO.Unbind();
	sphereEBO.Unbind();



	// ============================================================
	// UNIFORM LOCATIONS
	// ============================================================

	GLuint uniID =
		glGetUniformLocation(
			shaderProgram.ID,
			"scale"
		);


	GLuint modelLoc =
		glGetUniformLocation(
			shaderProgram.ID,
			"model"
		);


	GLuint viewLoc =
		glGetUniformLocation(
			shaderProgram.ID,
			"view"
		);


	GLuint projLoc =
		glGetUniformLocation(
			shaderProgram.ID,
			"proj"
		);



	// ============================================================
	// ENABLE DEPTH TEST
	// ============================================================

	glEnable(GL_DEPTH_TEST);



	// ============================================================
	// ROTATION
	// ============================================================

	float rotation = 0.0f;

	double prevTime = glfwGetTime();



	// ============================================================
	// MAIN LOOP
	// ============================================================

	while (glfwWindowShouldClose(window) == false)
	{
		// Background color
		glClearColor(
			0.102f,
			0.137f,
			0.494f,
			1.0f
		);


		glClear(
			GL_COLOR_BUFFER_BIT |
			GL_DEPTH_BUFFER_BIT
		);


		shaderProgram.Activate();



		// ========================================================
		// ROTATION
		// ========================================================

		double currTime = glfwGetTime();

		if (currTime - prevTime >= 1.0 / 60.0)
		{
			rotation += 0.1f;

			prevTime = currTime;
		}



		// ========================================================
		// MODEL
		// ========================================================

		mat4 model = mat4(1.0f);



		// ========================================================
		// VIEW - DIAGONAL CAMERA
		// ========================================================

		mat4 view = mat4(1.0f);

		view = lookAt(

			// Camera position
			vec3(4.0f, 3.0f, 6.0f),

			// Camera looking at
			vec3(0.0f, 0.0f, 0.0f),

			// Up direction
			vec3(0.0f, 1.0f, 0.0f)
		);



		// ========================================================
		// PROJECTION
		// ========================================================

		mat4 proj = mat4(1.0f);

		proj = perspective(

			radians(45.0f),

			(float)width / (float)height,

			0.1f,

			100.0f
		);



		// ========================================================
		// SEND VIEW TO SHADER
		// ========================================================

		glUniformMatrix4fv(
			viewLoc,
			1,
			GL_FALSE,
			value_ptr(view)
		);



		// ========================================================
		// SEND PROJECTION TO SHADER
		// ========================================================

		glUniformMatrix4fv(
			projLoc,
			1,
			GL_FALSE,
			value_ptr(proj)
		);


		glUniform1f(
			uniID,
			0.5f
		);



		// ========================================================
		// DRAW CUBE
		// ========================================================

		model = mat4(1.0f);


		// Move cube to LEFT
		model = translate(
			model,
			vec3(-1.5f, 0.0f, 0.0f)
		);


		// Rotate cube
		model = rotate(
			model,
			radians(rotation),
			vec3(0.0f, 1.0f, 0.0f)
		);


		// Send cube model matrix
		glUniformMatrix4fv(
			modelLoc,
			1,
			GL_FALSE,
			value_ptr(model)
		);


		cubeVAO.Bind();


		glDrawElements(
			GL_TRIANGLES,
			sizeof(cubeIndices) / sizeof(GLuint),
			GL_UNSIGNED_INT,
			0
		);



		// ========================================================
		// DRAW SPHERE
		// ========================================================

		model = mat4(1.0f);


		// Sphere stays in CENTER
		model = translate(
			model,
			vec3(0.0f, 0.0f, 0.0f)
		);


		// Rotate sphere
		model = rotate(
			model,
			radians(rotation),
			vec3(0.0f, 1.0f, 0.0f)
		);


		// Send sphere model matrix
		glUniformMatrix4fv(
			modelLoc,
			1,
			GL_FALSE,
			value_ptr(model)
		);


		sphereVAO.Bind();


		glDrawElements(
			GL_TRIANGLES,
			static_cast<GLsizei>(
				sphereIndices.size()
				),
			GL_UNSIGNED_INT,
			0
		);



		// ========================================================
		// DRAW PYRAMID
		// ========================================================

		model = mat4(1.0f);


		// Move pyramid to RIGHT
		model = translate(
			model,
			vec3(1.5f, 0.0f, 0.0f)
		);


		// Rotate pyramid
		model = rotate(
			model,
			radians(rotation),
			vec3(0.0f, 1.0f, 0.0f)
		);


		// Send pyramid model matrix
		glUniformMatrix4fv(
			modelLoc,
			1,
			GL_FALSE,
			value_ptr(model)
		);


		pyramidVAO.Bind();


		glDrawElements(
			GL_TRIANGLES,
			sizeof(pyramidIndices) / sizeof(GLuint),
			GL_UNSIGNED_INT,
			0
		);



		// ========================================================
		// DISPLAY FRAME
		// ========================================================

		glfwSwapBuffers(window);

		glfwPollEvents();
	}



	// ============================================================
	// DELETE CUBE
	// ============================================================

	cubeVAO.Delete();
	cubeVBO.Delete();
	cubeEBO.Delete();



	// ============================================================
	// DELETE SPHERE
	// ============================================================

	sphereVAO.Delete();
	sphereVBO.Delete();
	sphereEBO.Delete();



	// ============================================================
	// DELETE PYRAMID
	// ============================================================

	pyramidVAO.Delete();
	pyramidVBO.Delete();
	pyramidEBO.Delete();



	// ============================================================
	// DELETE SHADER
	// ============================================================

	shaderProgram.Delete();



	// ============================================================
	// CLOSE WINDOW
	// ============================================================

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}