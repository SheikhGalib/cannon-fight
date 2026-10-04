// RenderDocShots.cpp
// ==================
//
// A documentation tool, NOT part of the game. It opens a hidden OpenGL window,
// draws each shape / each build-up stage of the cannon on its own, reads the
// pixels back and writes them to ../docs/images/walkthrough/*.bmp, so that
// docs/code-walkthrough.md can show a picture of what every piece of code
// actually produces.
//
// Build and run it with:   mingw32-make shots
// (must be run from inside Project1/, because shaders are loaded by relative path)
//
// It has its own main(), so it is never compiled together with Main.cpp.

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaderClass.h"
#include "Primitives.h"
#include "Dimensions.h"
#include "Palette.h"
#include "Part.h"
#include "Carriage.h"
#include "Wheel.h"
#include "Shaft.h"

using namespace glm;

static const int SHOT_W = 900;
static const int SHOT_H = 620;
static const char* OUT_DIR = "../docs/images/walkthrough";

// ---------------------------------------------------------------------------
// Writing the picture out as a .bmp (the simplest image format there is: a
// small header followed by raw pixels). A PowerShell one-liner converts these
// to .png afterwards - see the `shots` target in the Makefile.
// ---------------------------------------------------------------------------
static void WriteLE32(std::ofstream& f, unsigned int v) { f.put(char(v & 0xFF)); f.put(char((v >> 8) & 0xFF)); f.put(char((v >> 16) & 0xFF)); f.put(char((v >> 24) & 0xFF)); }
static void WriteLE16(std::ofstream& f, unsigned short v) { f.put(char(v & 0xFF)); f.put(char((v >> 8) & 0xFF)); }

static void SaveBMP(const std::string& path, int w, int h, const std::vector<unsigned char>& rgb) {
    const int rowBytes = w * 3;
    const int padding = (4 - (rowBytes % 4)) % 4;  // BMP rows are padded to a multiple of 4 bytes
    const unsigned int pixelBytes = (rowBytes + padding) * h;

    std::ofstream f(path, std::ios::binary);
    f.put('B'); f.put('M');
    WriteLE32(f, 14 + 40 + pixelBytes);  // total file size
    WriteLE32(f, 0);
    WriteLE32(f, 14 + 40);               // where the pixels start
    WriteLE32(f, 40);                    // DIB header size
    WriteLE32(f, (unsigned int)w);
    WriteLE32(f, (unsigned int)h);
    WriteLE16(f, 1);                     // colour planes
    WriteLE16(f, 24);                    // bits per pixel
    WriteLE32(f, 0); WriteLE32(f, pixelBytes);
    WriteLE32(f, 2835); WriteLE32(f, 2835); WriteLE32(f, 0); WriteLE32(f, 0);

    // glReadPixels hands back rows starting from the BOTTOM of the screen, and
    // a BMP also stores rows bottom-first, so the rows go straight through.
    // Only the channel order differs: OpenGL gives R,G,B and BMP wants B,G,R.
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

// ---------------------------------------------------------------------------
// Scene helpers
// ---------------------------------------------------------------------------
struct Ctx {
    Shader* lit;
    Shader* flat;
    Shader* active;
    std::vector<Part> axes;   // the little red/green/blue X-Y-Z marker
    Mesh* ground;
};

static void SetMatrix(Shader& s, const char* name, const mat4& m) {
    glUniformMatrix4fv(glGetUniformLocation(s.ID, name), 1, GL_FALSE, value_ptr(m));
}

static void DrawMesh(Shader& s, Mesh& mesh, const mat4& model) {
    SetMatrix(s, "model", model);
    mesh.Draw();
}

static void BeginShot(Ctx& ctx, vec3 eye, vec3 target, bool unlit = false) {
    ctx.active = unlit ? ctx.flat : ctx.lit;
    ctx.active->Activate();

    glClearColor(0.86f, 0.90f, 0.95f, 1.0f); // pale background, so shapes stand out in the doc
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 view = lookAt(eye, target, vec3(0.0f, 1.0f, 0.0f));
    mat4 proj = perspective(radians(42.0f), float(SHOT_W) / float(SHOT_H), 0.05f, 300.0f);
    SetMatrix(*ctx.active, "view", view);
    SetMatrix(*ctx.active, "proj", proj);
    glUniform3fv(glGetUniformLocation(ctx.active->ID, "lightDir"), 1,
                 value_ptr(normalize(vec3(-0.4f, -1.0f, -0.5f))));
}

static void EndShot(const std::string& name) {
    std::vector<unsigned char> pixels(size_t(SHOT_W) * SHOT_H * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, SHOT_W, SHOT_H, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::string path = std::string(OUT_DIR) + "/" + name + ".bmp";
    SaveBMP(path, SHOT_W, SHOT_H, pixels);
    std::cout << "  " << path << "\n";
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);   // render without showing a window

    GLFWwindow* window = glfwCreateWindow(SHOT_W, SHOT_H, "doc shots", NULL, NULL);
    if (!window) { std::cout << "window failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glViewport(0, 0, SHOT_W, SHOT_H);
    glEnable(GL_DEPTH_TEST);

    Shader lit("lit.vert", "lit.frag");
    Shader flat("lit.vert", "tools/flat.frag");

    Ctx ctx;
    ctx.lit = &lit; ctx.flat = &flat; ctx.active = &lit;

    // The axis marker: three thin bars out of the origin.
    // red = +X (forward), green = +Y (up), blue = +Z (right)
    const vec3 red(0.85f, 0.15f, 0.15f), green(0.15f, 0.70f, 0.20f), blue(0.15f, 0.35f, 0.90f);
    const float axLen = 1.6f, axThick = 0.022f;
    ctx.axes.push_back({ Primitives::CreateBox(axLen, axThick, axThick, red),   Local::Move(vec3(axLen / 2, 0, 0)) });
    ctx.axes.push_back({ Primitives::CreateBox(axThick, axLen, axThick, green), Local::Move(vec3(0, axLen / 2, 0)) });
    ctx.axes.push_back({ Primitives::CreateBox(axThick, axThick, axLen, blue),  Local::Move(vec3(0, 0, axLen / 2)) });

    Mesh ground = Primitives::CreatePlane(120.0f, 120.0f, Palette::Grass);
    ctx.ground = &ground;
    const mat4 groundM = translate(mat4(1.0f), vec3(0.0f, -0.01f, 0.0f));

    const vec3 demoColor(0.55f, 0.45f, 0.75f);  // a neutral purple for the shape demos
    const vec3 shapeEye(2.3f, 1.7f, 2.9f), shapeTgt(0.15f, 0.30f, 0.0f);

    std::cout << "writing doc shots...\n";

    // =====================================================================
    // 1. The raw shapes Primitives.cpp can make
    // =====================================================================
    {
        Mesh box    = Primitives::CreateBox(1.0f, 0.6f, 0.8f, demoColor);
        Mesh cyl    = Primitives::CreateCylinder(0.35f, 1.0f, 24, demoColor);
        Mesh cone   = Primitives::CreateCone(0.45f, 0.18f, 1.0f, 24, demoColor);
        Mesh tube   = Primitives::CreateTube(0.28f, 0.45f, 0.35f, 32, demoColor);
        Mesh sphere = Primitives::CreateSphere(0.5f, 18, 24, demoColor);
        Mesh plane  = Primitives::CreatePlane(2.0f, 2.0f, demoColor);

        struct { const char* name; Mesh* mesh; } shapes[] = {
            { "p1-box", &box }, { "p2-cylinder", &cyl }, { "p3-cone", &cone },
            { "p4-tube", &tube }, { "p5-sphere", &sphere }, { "p6-plane", &plane },
        };
        for (auto& s : shapes) {
            BeginShot(ctx, shapeEye, shapeTgt);
            DrawParts(*ctx.active, mat4(1.0f), ctx.axes);
            DrawMesh(*ctx.active, *s.mesh, mat4(1.0f));
            EndShot(s.name);
        }

        // Lit vs unlit, on the same two shapes.
        for (int pass = 0; pass < 2; pass++) {
            BeginShot(ctx, vec3(2.4f, 1.5f, 2.6f), vec3(0.0f, 0.15f, 0.0f), /*unlit=*/pass == 0);
            DrawMesh(*ctx.active, sphere, translate(mat4(1.0f), vec3(-0.55f, 0.5f, 0.0f)));
            DrawMesh(*ctx.active, box, translate(mat4(1.0f), vec3(0.75f, 0.3f, 0.0f)));
            EndShot(pass == 0 ? "n1-unlit" : "n2-lit");
        }

        // =================================================================
        // 2. Orientation: the SAME cylinder, pointed three different ways
        // =================================================================
        Mesh bar = Primitives::CreateCylinder(0.12f, 1.2f, 20, demoColor, /*centered=*/false);
        struct { const char* name; mat4 m; } orients[] = {
            { "o1-standing", Local::Move(vec3(0.0f))  },  // as built: length along +Y
            { "o2-alongx",   Local::AlongX(vec3(0.0f)) },
            { "o3-alongz",   Local::AlongZ(vec3(0.0f)) },
        };
        for (auto& o : orients) {
            BeginShot(ctx, shapeEye, shapeTgt);
            DrawParts(*ctx.active, mat4(1.0f), ctx.axes);
            DrawMesh(*ctx.active, bar, o.m);
            EndShot(o.name);
        }

        // TurnMove: rotate first, then step out -> a fan of spokes
        {
            BeginShot(ctx, vec3(0.2f, 0.4f, 3.4f), vec3(0.0f, 0.0f, 0.0f));
            DrawParts(*ctx.active, mat4(1.0f), ctx.axes);
            Mesh spoke = Primitives::CreateBox(0.9f, 0.09f, 0.09f, demoColor);
            for (int i = 0; i < 8; i++) {
                DrawMesh(*ctx.active, spoke, Local::TurnMove(360.0f * i / 8.0f, vec3(0.6f, 0.0f, 0.0f)));
            }
            EndShot("o4-turnmove");
            spoke.Delete();
        }
        // MoveTurn: step out first, then rotate in place -> a tilted beam
        {
            BeginShot(ctx, vec3(0.6f, 1.2f, 3.6f), vec3(0.0f, 0.25f, 0.0f));
            DrawParts(*ctx.active, mat4(1.0f), ctx.axes);
            Mesh beam = Primitives::CreateBox(1.6f, 0.16f, 0.2f, demoColor);
            DrawMesh(*ctx.active, beam, Local::Move(vec3(0.0f, 0.9f, 0.0f)));       // untilted, above
            DrawMesh(*ctx.active, beam, Local::MoveTurn(vec3(0.0f, 0.25f, 0.0f), 20.0f)); // tilted
            EndShot("o5-moveturn");
            beam.Delete();
        }

        box.Delete(); cyl.Delete(); cone.Delete(); tube.Delete();
        sphere.Delete(); plane.Delete(); bar.Delete();
    }

    // =====================================================================
    // 3. The wheel, one stage at a time
    // =====================================================================
    {
        Wheel wheel(Dim::WheelRadius, Dim::WheelWidth, Dim::SpokeCount, vec3(0.0f));
        std::vector<Part>& wp = wheel.PartsForDocs();
        const vec3 wheelEye(1.1f, 0.5f, 2.6f), wheelTgt(0.0f, 0.0f, 0.0f);
        // part order: 0 tire | 1 felloe | 2..11 spokes | 12 hub | 13,14 caps
        const size_t stages[] = { 1, 2, 2 + Dim::SpokeCount, wp.size() };
        const char* names[] = { "w1-tire", "w2-felloe", "w3-spokes", "w4-hub" };
        for (int i = 0; i < 4; i++) {
            BeginShot(ctx, wheelEye, wheelTgt);
            DrawPartRange(*ctx.active, mat4(1.0f), wp, 0, stages[i]);
            EndShot(names[i]);
        }
        // the same wheel after rolling, to show the roll rotation
        BeginShot(ctx, wheelEye, wheelTgt);
        wheel.Roll(0.35f);
        wheel.Draw(*ctx.active, mat4(1.0f));
        EndShot("w5-rolled");
        wheel.Delete();
    }

    // =====================================================================
    // 4. The carriage, one stage at a time
    // =====================================================================
    {
        Carriage carriage;
        std::vector<Part>& cp = carriage.PartsForDocs();
        const vec3 carEye(2.9f, 1.9f, 3.5f), carTgt(-0.75f, 0.50f, 0.0f);
        // 0-1 beams | 2-4 transoms | 5 spade | 6-7 cheeks | 8 quoin | 9 axle | 10 bolster
        const size_t stages[] = { 2, 5, 6, 8, 9, cp.size() };
        const char* names[] = { "c1-beams", "c2-transoms", "c3-spade", "c4-cheeks", "c5-quoin", "c6-axle" };
        for (int i = 0; i < 6; i++) {
            BeginShot(ctx, carEye, carTgt);
            DrawMesh(*ctx.active, ground, groundM);
            DrawPartRange(*ctx.active, mat4(1.0f), cp, 0, stages[i]);
            EndShot(names[i]);
        }
        carriage.Delete();
    }

    // =====================================================================
    // 5. The barrel, one stage at a time (drawn at the origin, not at the pivot)
    // =====================================================================
    {
        Shaft shaft(vec3(0.0f));
        std::vector<Part>& bp = shaft.BarrelPartsForDocs();
        std::vector<Part>& tp = shaft.TrunnionPartsForDocs();
        const vec3 barEye(2.2f, 1.5f, 3.3f), barTgt(0.60f, 0.25f, 0.0f);
        // 0 breech | 1-2 cascabel | 3 reinforce | 4 chase | 5-6 swell | 7-8 rings | 9 vent | 10 bore
        const size_t stages[] = { 3, 5, 7, 9, bp.size() };
        const char* names[] = { "b1-breech", "b2-tube", "b3-muzzle", "b4-rings", "b5-bore" };
        for (int i = 0; i < 5; i++) {
            BeginShot(ctx, barEye, barTgt);
            DrawParts(*ctx.active, mat4(1.0f), ctx.axes);
            DrawPartRange(*ctx.active, mat4(1.0f), bp, 0, stages[i]);
            EndShot(names[i]);
        }
        // with the trunnions added
        BeginShot(ctx, barEye, barTgt);
        DrawParts(*ctx.active, mat4(1.0f), ctx.axes);
        DrawParts(*ctx.active, mat4(1.0f), tp);
        DrawParts(*ctx.active, mat4(1.0f), bp);
        EndShot("b6-trunnions");

        // elevation: 0 degrees vs 45 degrees, pivot marked by the axis cross
        for (int i = 0; i < 2; i++) {
            if (i == 1) shaft.Elevate(45.0f);
            BeginShot(ctx, barEye, barTgt);
            DrawParts(*ctx.active, mat4(1.0f), ctx.axes);
            DrawParts(*ctx.active, mat4(1.0f), tp);
            DrawPartRange(*ctx.active, shaft.GetMatrixForDocs(), bp, 0, bp.size());
            EndShot(i == 0 ? "b7-elev0" : "b8-elev45");
        }
        shaft.Delete();
    }

    // =====================================================================
    // 6. Assembling it: carriage -> + wheels -> + barrel
    // =====================================================================
    {
        Carriage carriage;
        Wheel leftWheel(Dim::WheelRadius, Dim::WheelWidth, Dim::SpokeCount, vec3(0.0f, Dim::WheelRadius, -Dim::WheelTrack));
        Wheel rightWheel(Dim::WheelRadius, Dim::WheelWidth, Dim::SpokeCount, vec3(0.0f, Dim::WheelRadius, Dim::WheelTrack));
        Shaft shaft(vec3(Dim::PivotX, Dim::PivotY, Dim::PivotZ));

        const vec3 gunEye(4.8f, 2.7f, 5.5f), gunTgt(0.05f, 0.70f, 0.0f);

        BeginShot(ctx, gunEye, gunTgt);
        DrawMesh(*ctx.active, ground, groundM);
        carriage.Draw(*ctx.active, mat4(1.0f));
        EndShot("a1-carriage");

        BeginShot(ctx, gunEye, gunTgt);
        DrawMesh(*ctx.active, ground, groundM);
        carriage.Draw(*ctx.active, mat4(1.0f));
        leftWheel.Draw(*ctx.active, carriage.GetMatrix());
        rightWheel.Draw(*ctx.active, carriage.GetMatrix());
        EndShot("a2-wheels");

        shaft.Elevate(12.0f);
        BeginShot(ctx, gunEye, gunTgt);
        DrawMesh(*ctx.active, ground, groundM);
        carriage.Draw(*ctx.active, mat4(1.0f));
        leftWheel.Draw(*ctx.active, carriage.GetMatrix());
        rightWheel.Draw(*ctx.active, carriage.GetMatrix());
        shaft.Draw(*ctx.active, carriage.GetMatrix());
        EndShot("a3-full");

        // the gun at maximum elevation
        shaft.Elevate(33.0f);
        BeginShot(ctx, gunEye, gunTgt);
        DrawMesh(*ctx.active, ground, groundM);
        carriage.Draw(*ctx.active, mat4(1.0f));
        leftWheel.Draw(*ctx.active, carriage.GetMatrix());
        rightWheel.Draw(*ctx.active, carriage.GetMatrix());
        shaft.Draw(*ctx.active, carriage.GetMatrix());
        EndShot("a4-elev45");
        shaft.Elevate(-33.0f);

        // driven forward 2.5 m: carriage moved, wheels rolled to match
        carriage.MoveForward(2.5f);
        leftWheel.Roll(2.5f);
        rightWheel.Roll(2.5f);
        BeginShot(ctx, gunEye, gunTgt);
        DrawMesh(*ctx.active, ground, groundM);
        carriage.Draw(*ctx.active, mat4(1.0f));
        leftWheel.Draw(*ctx.active, carriage.GetMatrix());
        rightWheel.Draw(*ctx.active, carriage.GetMatrix());
        shaft.Draw(*ctx.active, carriage.GetMatrix());
        EndShot("a5-driven");

        // THE BUG PICTURE: the same move, but the wheels and barrel are drawn
        // with the world as their parent instead of the carriage - so the body
        // drives away and leaves its own wheels behind.
        BeginShot(ctx, gunEye, gunTgt);
        DrawMesh(*ctx.active, ground, groundM);
        carriage.Draw(*ctx.active, mat4(1.0f));
        leftWheel.Draw(*ctx.active, mat4(1.0f));
        rightWheel.Draw(*ctx.active, mat4(1.0f));
        shaft.Draw(*ctx.active, mat4(1.0f));
        EndShot("a6-noparent");

        carriage.Delete(); leftWheel.Delete(); rightWheel.Delete(); shaft.Delete();
    }

    DeleteParts(ctx.axes);
    ground.Delete();
    lit.Delete();
    flat.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "done.\n";
    return 0;
}
