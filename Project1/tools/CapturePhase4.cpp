// CapturePhase4.cpp
// =================
//
// Verification screenshot for Phase 4 (the Disney-style castle: two flanking
// towers, crenellated curtain walls, breakable wooden doors, drawbridge).
// Same approach as CapturePhase3.cpp - tick the simulation with auto-fired
// shots to break one of the door panels, then render one frame to a hidden
// window and save a .bmp.
//
// Build & run by hand:
//   g++ -std=c++17 -O2 -ILibraries/include -I. -I../opengl-cpp/deps/glfw/include \
//       tools/CapturePhase4.cpp Cannon.cpp Projectile.cpp Wall.cpp FortGate.cpp \
//       Crenellation.cpp Door.cpp Tower.cpp Castle.cpp Tree.cpp Robot.cpp \
//       Carriage.cpp Wheel.cpp Shaft.cpp VAO.cpp VBO.cpp EBO.cpp shaderClass.cpp \
//       glad.c Transform.cpp Mesh.cpp Primitives.cpp Part.cpp \
//       -L../opengl-cpp/deps/glfw/lib-mingw-w64 -lglfw3 -lopengl32 -lgdi32 \
//       -static-libgcc -static-libstdc++ -static \
//       -o build_mingw/capture4.exe
//   ./build_mingw/capture4.exe

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>
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
#include "Crenellation.h"
#include "Door.h"
#include "Tower.h"
#include "Castle.h"
#include "Tree.h"
#include "Robot.h"

using namespace glm;

static const int SHOT_W = 1100;
static const int SHOT_H = 700;

static void WriteLE32(std::ofstream& f, unsigned int v) {
    f.put(char(v & 0xFF)); f.put(char((v >> 8) & 0xFF));
    f.put(char((v >> 16) & 0xFF)); f.put(char((v >> 24) & 0xFF));
}
static void WriteLE16(std::ofstream& f, unsigned short v) {
    f.put(char(v & 0xFF)); f.put(char((v >> 8) & 0xFF));
}

static void SaveBMP(const std::string& path, int w, int h, const std::vector<unsigned char>& rgb) {
    const int rowBytes = w * 3;
    const int padding = (4 - (rowBytes % 4)) % 4;
    const unsigned int pixelBytes = (rowBytes + padding) * h;

    std::ofstream f(path, std::ios::binary);
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
}

int main() {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SHOT_W, SHOT_H, "CapturePhase4", NULL, NULL);
    if (!window) return 2;
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glViewport(0, 0, SHOT_W, SHOT_H);
    glEnable(GL_DEPTH_TEST);

    Shader shaderProgram("lit.vert", "lit.frag");

    Mesh ground = Primitives::CreatePlane(120.0f, 120.0f, Palette::Grass);

    const float cannonSpacing = 2.5f;
    Cannon centreCannon(vec3(0.0f, 0.0f,  0.0f));
    Cannon leftCannon  (vec3(0.0f, 0.0f,  cannonSpacing));
    Cannon rightCannon (vec3(0.0f, 0.0f, -cannonSpacing));

    // Match Main.cpp tuning so the auto-fired balls land on the door.
    centreCannon.Elevate(6.0f);   // 12 -> 18 deg total

    Castle castle(vec3(12.0f, 0.0f, 0.0f),
                  /*gateWidth=*/2.0f,
                  /*wallHeight=*/3.0f,
                  /*towerHeight=*/5.0f,
                  /*curtainLength=*/6.0f);

    std::vector<Tree> trees;
    for (int i = 0; i < 6; i++) {
        float x = 22.0f + float(i) * 3.0f;
        float z = -8.0f + float(i % 2) * 4.0f;
        trees.emplace_back(vec3(x, 0.0f, z), 2.0f, 0.20f, 3.0f, 1.5f);
    }
    Robot robot(vec3(13.5f, 0.0f, 0.0f));

    std::vector<Projectile> projectiles;

    mat4 projMat = perspective(radians(50.0f), float(SHOT_W) / float(SHOT_H), 0.1f, 100.0f);
    mat4 view = lookAt(vec3(-5.0f, 3.0f, 7.0f), vec3(8.0f, 0.7f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

    GLuint viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram.ID, "proj");
    GLuint modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
    GLuint lightDirLoc = glGetUniformLocation(shaderProgram.ID, "lightDir");

    vec3 lightDir = normalize(vec3(-0.4f, -1.0f, -0.5f));

    // Fire three balls (offset slightly left/right/centre) to break at least
    // one door panel and a couple of brick stacks, then advance 2 s.
    float dt = 1.0f / 60.0f;
    for (int frame = 0; frame < 120; frame++) {
        if (frame == 0 || frame == 20 || frame == 40) {
            vec3 muzzle = centreCannon.GetMuzzleWorldPosition();
            vec3 fwd = centreCannon.GetForwardWorldDirection();
            projectiles.emplace_back(muzzle, fwd * Projectile::DefaultSpeed, Projectile::DefaultRadius);
        }
        for (Projectile& b : projectiles) {
            b.Update(dt, Projectile::Gravity);
            castle.CheckHit(b.GetPosition(), b.GetRadius());
        }
        for (auto it = projectiles.begin(); it != projectiles.end();) {
            if (it->IsDead()) it = projectiles.erase(it);
            else ++it;
        }
    }

    std::cout << "After 2s: alive door panels = " << castle.AliveDoorPanelCount()
              << " (out of " << castle.TotalDoorPanelCount() << "), "
              << "alive bricks = " << castle.AliveBrickCount()
              << " (out of " << castle.TotalBrickCount() << "), "
              << "live balls = " << projectiles.size() << std::endl;

    // Render and save.
    glClearColor(0.55f, 0.72f, 0.87f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shaderProgram.Activate();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projMat));
    glUniform3fv(lightDirLoc, 1, value_ptr(lightDir));

    mat4 groundMatrix = translate(mat4(1.0f), vec3(0.0f, -0.01f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(groundMatrix));
    ground.Draw();

    for (Tree& t : trees) t.Draw(shaderProgram);
    castle.Draw(shaderProgram);
    robot.Draw(shaderProgram);
    centreCannon.Draw(shaderProgram);
    leftCannon.Draw(shaderProgram);
    rightCannon.Draw(shaderProgram);
    for (Projectile& b : projectiles) b.Draw(shaderProgram);

    glFinish();
    std::vector<unsigned char> pixels(SHOT_W * SHOT_H * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, SHOT_W, SHOT_H, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glfwSwapBuffers(window);
    SaveBMP("phase4_capture.bmp", SHOT_W, SHOT_H, pixels);
    std::cout << "Saved phase4_capture.bmp" << std::endl;

    ground.Delete();
    centreCannon.Delete();
    leftCannon.Delete();
    rightCannon.Delete();
    castle.Delete();
    for (Tree& t : trees) t.Delete();
    robot.Delete();
    for (Projectile& b : projectiles) b.Delete();
    shaderProgram.Delete();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}