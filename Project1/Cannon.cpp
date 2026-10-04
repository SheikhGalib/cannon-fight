#include "Cannon.h"
#include "Dimensions.h"
#include <glm/gtc/matrix_transform.hpp>

Cannon::Cannon()
    : Cannon(glm::vec3(0.0f))
{}

Cannon::Cannon(glm::vec3 baseWorld)
    : carriage(baseWorld),
      leftWheel(Dim::WheelRadius, Dim::WheelWidth, Dim::SpokeCount,
                glm::vec3(0.0f, Dim::WheelRadius, -Dim::WheelTrack)),
      rightWheel(Dim::WheelRadius, Dim::WheelWidth, Dim::SpokeCount,
                 glm::vec3(0.0f, Dim::WheelRadius, Dim::WheelTrack)),
      shaft(glm::vec3(Dim::PivotX, Dim::PivotY, Dim::PivotZ))
{
    // Phase 3 starts the barrel tipped up so the pivot is visible.
    shaft.Elevate(12.0f);
}

void Cannon::MoveForward(float distance) {
    carriage.MoveForward(distance);
    leftWheel.Roll(distance);
    rightWheel.Roll(distance);
}

void Cannon::Elevate(float deltaDegrees) {
    shaft.Elevate(deltaDegrees);
}

glm::vec3 Cannon::GetMuzzleWorldPosition() const {
    return shaft.GetMuzzleWorldPosition(carriage.GetMatrix());
}

glm::vec3 Cannon::GetForwardWorldDirection() const {
    return shaft.GetForwardWorldDirection(carriage.GetMatrix());
}

void Cannon::Draw(Shader& shader) {
    glm::mat4 carriageMatrix = carriage.GetMatrix();
    // Carriage is drawn at world-space (parent = identity); its own
    // transform positions it.
    carriage.Draw(shader, glm::mat4(1.0f));
    leftWheel.Draw(shader, carriageMatrix);
    rightWheel.Draw(shader, carriageMatrix);
    shaft.Draw(shader, carriageMatrix);
}

void Cannon::Delete() {
    carriage.Delete();
    leftWheel.Delete();
    rightWheel.Delete();
    shaft.Delete();
}