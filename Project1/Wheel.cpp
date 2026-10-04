#include "Wheel.h"
#include "Primitives.h"
#include "Dimensions.h"
#include "Palette.h"
#include <glm/gtc/type_ptr.hpp>

Wheel::Wheel(float radius, float width, int spokeCount, glm::vec3 axlePosition)
    : radius(radius)
{
    const int segments = 32;

    // The wheel's own transform: where it sits, and how far it has rolled.
    // Everything below is positioned relative to THIS, so the whole wheel
    // spins as one the moment Roll() changes the angle.
    transform.position = axlePosition;
    transform.rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f); // roll about the axle
    transform.rotationDegrees = 0.0f;

    // --- iron tire: the dark outer band ----------------------------------
    parts.push_back({ Primitives::CreateTube(Dim::FelloeOuter, radius, width, segments, Palette::DarkIron),
                      Local::AlongZ(glm::vec3(0.0f)) });

    // --- wooden felloe: the rim just inside the tire, slightly narrower ---
    parts.push_back({ Primitives::CreateTube(Dim::FelloeInner, Dim::FelloeOuter, width * 0.86f, segments, Palette::WoodLight),
                      Local::AlongZ(glm::vec3(0.0f)) });

    // --- spokes ----------------------------------------------------------
    // One box per spoke, all identical; only the angle differs. TurnMove
    // rotates first and then steps outward along the already-turned X axis,
    // so "half way between the hub and the rim" is the same number for every
    // spoke no matter which direction it ends up pointing.
    const float spokeLength = Dim::SpokeOuter - Dim::SpokeInner;
    const float spokeMiddle = 0.5f * (Dim::SpokeInner + Dim::SpokeOuter);
    for (int i = 0; i < spokeCount; i++) {
        float angle = 360.0f * float(i) / float(spokeCount);
        parts.push_back({ Primitives::CreateBox(spokeLength, Dim::SpokeThick, Dim::SpokeThick, Palette::WoodLight),
                          Local::TurnMove(angle, glm::vec3(spokeMiddle, 0.0f, 0.0f)) });
    }

    // --- hub: the wooden barrel at the centre, poking out both sides ------
    parts.push_back({ Primitives::CreateCylinder(Dim::HubRadius, Dim::HubLength, 24, Palette::WoodLight),
                      Local::AlongZ(glm::vec3(0.0f)) });

    // --- brass caps on each end of the hub --------------------------------
    // Built with centered=false so each cone grows outward from the hub's end
    // face; AlongZ / AlongNegZ then point that growth at the two opposite sides.
    const float hubEnd = Dim::HubLength / 2.0f;
    parts.push_back({ Primitives::CreateCone(0.09f, 0.05f, 0.07f, 16, Palette::Brass, /*centered=*/false),
                      Local::AlongZ(glm::vec3(0.0f, 0.0f, hubEnd)) });
    parts.push_back({ Primitives::CreateCone(0.09f, 0.05f, 0.07f, 16, Palette::Brass, /*centered=*/false),
                      Local::AlongNegZ(glm::vec3(0.0f, 0.0f, -hubEnd)) });
}

void Wheel::Roll(float distanceMoved) {
    transform.rotationDegrees -= glm::degrees(distanceMoved / radius);
}

void Wheel::Draw(Shader& shader, const glm::mat4& parentMatrix) {
    DrawParts(shader, parentMatrix * transform.GetMatrix(), parts);
}

void Wheel::Delete() {
    DeleteParts(parts);
}
