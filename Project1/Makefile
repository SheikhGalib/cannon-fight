CXX := g++
CXXFLAGS := -std=c++17 -O2
GLFW_DIR := ../opengl-cpp/deps/glfw
INCLUDES := -ILibraries/include -I. -I$(GLFW_DIR)/include
LIBDIRS := -L$(GLFW_DIR)/lib-mingw-w64
LIBS := -lglfw3 -lopengl32 -lgdi32
LDFLAGS := -static-libgcc -static-libstdc++ -static

# Everything except the file that holds main(). Both the game and the doc-image
# tool link against these, they just each add their own main() on top.
COMMON := VAO.cpp VBO.cpp EBO.cpp shaderClass.cpp glad.c Transform.cpp Mesh.cpp Primitives.cpp Part.cpp Carriage.cpp Wheel.cpp Shaft.cpp Cannon.cpp Projectile.cpp Wall.cpp FortGate.cpp Crenellation.cpp Door.cpp Tower.cpp Castle.cpp CornerTower.cpp CompoundCastle.cpp Water.cpp Bridge.cpp Robot.cpp Archer.cpp Soldier.cpp CampTent.cpp Scenery.cpp GoldCrest.cpp Arrow.cpp Tree.cpp SkyClouds.cpp Birds.cpp SignalTower.cpp ShadowMap.cpp ParticleSystem.cpp
SRCS := Main.cpp $(COMMON)
SHOT_SRCS := tools/RenderDocShots.cpp $(COMMON)

BUILD_DIR := build_mingw
TARGET := $(BUILD_DIR)/app.exe
SHOT_TARGET := $(BUILD_DIR)/docshots.exe
CAPTURE_TARGET := $(BUILD_DIR)/capture.exe

.PHONY: all run clean shots capture video

all: $(TARGET)

$(TARGET): $(SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) $(LIBDIRS) $(LIBS) $(LDFLAGS) -o $(TARGET)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

run: all
	$(TARGET)

# Regenerates every picture used by docs/code-walkthrough.md. Renders them as
# .bmp (trivial to write from C++), then converts them to .png with PowerShell
# and deletes the .bmp files.
$(SHOT_TARGET): $(SHOT_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SHOT_SRCS) $(LIBDIRS) $(LIBS) $(LDFLAGS) -o $(SHOT_TARGET)

shots: $(SHOT_TARGET)
	$(SHOT_TARGET)
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/bmp2png.ps1

# Phase 9: build the capture tool that runs the full scene + battle
# simulation and writes per-frame BMPs into capture/.  Combine the
# frames into a presentation MP4 with `make video`.
CAPTURE_SRCS := tools/CaptureSim.cpp $(COMMON)
$(CAPTURE_TARGET): $(CAPTURE_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CAPTURE_SRCS) $(LIBDIRS) $(LIBS) $(LDFLAGS) -o $(CAPTURE_TARGET)

capture: $(CAPTURE_TARGET)
	$(CAPTURE_TARGET)

video: capture
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/encode_video.ps1

clean:
	del /Q $(BUILD_DIR)\*.exe 2>nul || true
