#include "Shaft.h"
#include "Primitives.h"
#include "Dimensions.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Shaft::Shaft(glm::vec3 pivotPosition) {
    const int segments = 28;

    transform.position = pivotPosition;
    transform.rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f); // elevation tips about Z
    transform.rotationDegrees = 0.0f;

    // All X values below are measured in barrel space, i.e. from the pivot:
    // negative is behind the pivot (the breech end), positive is out towards
    // the muzzle. Every piece is built with centered=false so it grows forward
    // from the X given, which lets the stack be read top to bottom as a chain:
    // each piece starts where the previous one ended.
    const float breech = Dim::BarrelBreechX;

    // --- rounded breech and the cascabel knob behind it --------------------
    barrelParts.push_back({ Primitives::CreateSphere(0.205f, 20, segments, Palette::Iron),
                            Local::Move(glm::vec3(breech, 0.0f, 0.0f)) });
    barrelParts.push_back({ Primitives::CreateSphere(0.085f, 14, 16, Palette::Iron),
                            Local::Move(glm::vec3(breech - 0.16f, 0.0f, 0.0f)) });
    barrelParts.push_back({ Primitives::CreateCylinder(0.045f, 0.14f, 14, Palette::Iron, /*centered=*/false),
                            Local::AlongX(glm::vec3(breech - 0.18f, 0.0f, 0.0f)) });

    // --- the tube itself: four cones, thick at the back, tapering forward ---
    // first reinforce: the heavy section right behind the trunnions
    barrelParts.push_back({ Primitives::CreateCone(0.200f, 0.185f, 0.55f, segments, Palette::Iron, false),
                            Local::AlongX(glm::vec3(breech, 0.0f, 0.0f)) });
    // chase: the long slim run out towards the mouth
    barrelParts.push_back({ Primitives::CreateCone(0.175f, 0.125f, 1.42f, segments, Palette::Iron, false),
                            Local::AlongX(glm::vec3(breech + 0.55f, 0.0f, 0.0f)) });
    // muzzle swell: the tube flares back out into a lip at the very end
    barrelParts.push_back({ Primitives::CreateCone(0.125f, 0.165f, 0.11f, segments, Palette::Iron, false),
                            Local::AlongX(glm::vec3(breech + 1.97f, 0.0f, 0.0f)) });
    barrelParts.push_back({ Primitives::CreateCone(0.165f, 0.150f, 0.07f, segments, Palette::Iron, false),
                            Local::AlongX(glm::vec3(breech + 2.08f, 0.0f, 0.0f)) });

    // --- brass astragal rings banding the tube -----------------------------
    // Tubes rather than solid discs, so they sit AROUND the barrel as raised
    // bands; their inner radius is tucked just inside the tube at that point.
    for (auto ring : { std::pair<float, float>{ 0.10f, 0.190f },
                       std::pair<float, float>{ 0.72f, 0.155f } }) {
        float x = ring.first, outer = ring.second;
        barrelParts.push_back({ Primitives::CreateTube(outer - 0.03f, outer + 0.022f, 0.05f, segments, Palette::Brass),
                                Local::AlongX(glm::vec3(x, 0.0f, 0.0f)) });
    }

    // --- vent / touch hole on top of the breech ----------------------------
    // The only piece that stays upright, so it needs no Along* at all.
    barrelParts.push_back({ Primitives::CreateCone(0.035f, 0.028f, 0.07f, 12, Palette::DarkIron, false),
                            Local::Move(glm::vec3(breech + 0.15f, 0.16f, 0.0f)) });

    // --- the bore: a near-black tube sunk into the muzzle face -------------
    // Without this the muzzle is a flat metal disc and the gun looks solid.
    barrelParts.push_back({ Primitives::CreateCylinder(Dim::BoreRadius, 0.30f, 20, Palette::Bore, false),
                            Local::AlongX(glm::vec3(Dim::MuzzleX - 0.30f, 0.0f, 0.0f)) });

    // --- trunnions: the two stubs the barrel hangs on ----------------------
    // Centred on the pivot itself, sticking out sideways into the cheeks.
    trunnionParts.push_back({ Primitives::CreateCone(0.09f, 0.085f, 0.24f, 20, Palette::Brass, false),
                              Local::AlongZ(glm::vec3(0.0f, 0.0f, 0.16f)) });
    trunnionParts.push_back({ Primitives::CreateCone(0.09f, 0.085f, 0.24f, 20, Palette::Brass, false),
                              Local::AlongNegZ(glm::vec3(0.0f, 0.0f, -0.16f)) });
}

void Shaft::Elevate(float deltaDegrees) {
    elevationDegrees = std::clamp(elevationDegrees + deltaDegrees, MinElevationDeg, MaxElevationDeg);
    transform.rotationDegrees = elevationDegrees;
}

void Shaft::Draw(Shader& shader, const glm::mat4& parentMatrix) {
    // The trunnions hang off the pivot POSITION only - no elevation rotation -
    // because they are the hinge, not the thing that swings on it.
    glm::mat4 mount = parentMatrix * glm::translate(glm::mat4(1.0f), transform.position);
    DrawParts(shader, mount, trunnionParts);

    // The tube gets the full transform: pivot position AND elevation.
    DrawParts(shader, parentMatrix * transform.GetMatrix(), barrelParts);
}

void Shaft::Delete() {
    DeleteParts(barrelParts);
    DeleteParts(trunnionParts);
}
