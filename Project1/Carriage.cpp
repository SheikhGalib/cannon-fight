#include "Carriage.h"
#include "Primitives.h"
#include "Dimensions.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

// Where along a beam a point lies: 0 at the front end, 1 at the rear end.
static glm::vec2 PointOnBeam(float t) {
    return glm::vec2(Dim::BeamFrontX + t * (Dim::BeamRearX - Dim::BeamFrontX),
                     Dim::BeamFrontY + t * (Dim::BeamRearY - Dim::BeamFrontY));
}

float Carriage::BeamHeightAt(float x) {
    // How far along the beam x is, as a fraction, then read the height there.
    float t = (x - Dim::BeamFrontX) / (Dim::BeamRearX - Dim::BeamFrontX);
    return Dim::BeamFrontY + t * (Dim::BeamRearY - Dim::BeamFrontY);
}

Carriage::Carriage() : Carriage(glm::vec3(0.0f)) {}

Carriage::Carriage(glm::vec3 initialPosition) {
    transform.position = initialPosition;
    // --- the two trail beams ---------------------------------------------
    // A beam is one long box, so all we need is its length, its tilt, and the
    // midpoint to hang it on. Those come straight out of its two endpoints.
    const float runX = Dim::BeamRearX - Dim::BeamFrontX;   // negative: the rear is behind
    const float runY = Dim::BeamRearY - Dim::BeamFrontY;   // negative: the rear is lower
    const float beamLength = std::sqrt(runX * runX + runY * runY);
    // atan2 of the reversed run, i.e. measured from the rear towards the
    // front, so a positive angle tips the front end upward.
    const float beamAngle = glm::degrees(std::atan2(-runY, -runX));
    const glm::vec2 beamMiddle = PointOnBeam(0.5f);

    for (float z : { -Dim::BeamZ, Dim::BeamZ }) {
        parts.push_back({ Primitives::CreateBox(beamLength, Dim::BeamThick, Dim::BeamWide, Palette::Wood),
                          Local::MoveTurn(glm::vec3(beamMiddle.x, beamMiddle.y, z), beamAngle) });
    }

    // --- transoms: cross-members holding the beams apart -------------------
    // Spaced along the beams at a fraction of their length, so they follow the
    // slope automatically. Their depth spans the full gap between the beams.
    for (float t : { 0.12f, 0.55f, 0.90f }) {
        glm::vec2 at = PointOnBeam(t);
        parts.push_back({ Primitives::CreateBox(0.16f, 0.14f, 2.0f * Dim::BeamZ, Palette::Wood),
                          Local::Move(glm::vec3(at.x, at.y, 0.0f)) });
    }

    // --- trail spade: the iron shoe at the rear end ------------------------
    parts.push_back({ Primitives::CreateBox(0.24f, 0.30f, 2.0f * Dim::BeamZ + 0.16f, Palette::DarkIron),
                      Local::Move(glm::vec3(Dim::BeamRearX - 0.06f, Dim::BeamRearY - 0.02f, 0.0f)) });

    // --- cheeks: the uprights that carry the trunnion pivot ---------------
    // Each stands on the beam below it and reaches just past the pivot height,
    // so the barrel visibly rests IN them rather than above them.
    const float cheekBase = BeamHeightAt(Dim::PivotX);
    const float cheekHeight = Dim::PivotY - cheekBase + 0.18f;
    for (float z : { -Dim::BeamZ, Dim::BeamZ }) {
        parts.push_back({ Primitives::CreateBox(0.40f, cheekHeight, Dim::BeamWide, Palette::Wood),
                          Local::Move(glm::vec3(Dim::PivotX, cheekBase + cheekHeight * 0.5f - 0.02f, z)) });
    }

    // --- quoin block: the step under the breech ---------------------------
    parts.push_back({ Primitives::CreateBox(0.42f, 0.34f, 2.0f * Dim::BeamZ, Palette::Wood),
                      Local::Move(glm::vec3(Dim::PivotX - 0.62f, cheekBase + 0.22f, 0.0f)) });

    // --- axle: one iron rod running right through, at wheel-centre height --
    // It is a little longer than the wheel spacing so its ends poke out
    // through the hubs, the way a real axle does.
    const float axleHalfSpan = Dim::WheelTrack + 0.10f;
    parts.push_back({ Primitives::CreateCylinder(0.065f, 2.0f * axleHalfSpan, 20, Palette::DarkIron),
                      Local::AlongZ(glm::vec3(0.0f, Dim::WheelRadius, 0.0f)) });

    // --- axle bolster: the timber strapping the axle to the beams ---------
    parts.push_back({ Primitives::CreateBox(0.26f, 0.22f, 2.0f * Dim::WheelTrack - 0.10f, Palette::Wood),
                      Local::Move(glm::vec3(0.0f, Dim::WheelRadius + 0.10f, 0.0f)) });
}

void Carriage::MoveForward(float distance) {
    transform.position.x += distance;
}

void Carriage::Draw(Shader& shader, const glm::mat4& parentMatrix) {
    DrawParts(shader, parentMatrix * transform.GetMatrix(), parts);
}

void Carriage::Delete() {
    DeleteParts(parts);
}
