#include "Cannon.h"
#include "Dimensions.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <algorithm>

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
    // Phase 7: drive along the cannon's current forward direction.
    // Carriage's existing MoveForward only steps X, so we route the
    // horizontal X+Z delta through Carriage::MoveRaw (added in
    // Carriage.h) instead.
    glm::vec3 fwd(std::cos(glm::radians(yawDeg)), 0.0f, std::sin(glm::radians(yawDeg)));
    carriage.MoveRaw(fwd * distance);
    leftWheel.Roll(distance);
    rightWheel.Roll(distance);
}

void Cannon::Elevate(float deltaDegrees) {
    shaft.Elevate(deltaDegrees);
}

void Cannon::Yaw(float deltaDeg) {
    yawDeg = std::clamp(yawDeg + deltaDeg, -MaxYawDeg, MaxYawDeg);
}

void Cannon::Fire() {
    carriage.Fire();
    flashIntensity = 1.0f;   // bright, decays in Update()
}

void Cannon::Update(float deltaTime) {
    carriage.Update(deltaTime);
    if (flashIntensity > 0.0f) {
        flashIntensity -= deltaTime / kFlashDuration;
        if (flashIntensity < 0.0f) flashIntensity = 0.0f;
    }
}

glm::vec3 Cannon::GetCentreWorldPosition() const {
    return glm::vec3(carriage.GetMatrix()[3]);
}

glm::mat4 Cannon::YawWrappedMatrix() const {
    // Wrap the carriage's matrix in a Y-rotation about the cannon's
    // current centre: T(centre) * R(yaw) * T(-centre) * carriageMatrix.
    // Used as the parent for the wheels + shaft (they're relative to
    // the cannon centre) AND as the muzzle / forward queries.  See the
    // long comment in Draw() for why the carriage itself uses a
    // different parent.
    glm::vec3 centre = GetCentreWorldPosition();
    glm::mat4 Tp   = glm::translate(glm::mat4(1.0f),  centre);
    glm::mat4 Tn   = glm::translate(glm::mat4(1.0f), -centre);
    glm::mat4 Ryaw = glm::rotate(glm::mat4(1.0f),
                                  glm::radians(yawDeg),
                                  glm::vec3(0.0f, 1.0f, 0.0f));
    return Tp * Ryaw * Tn * carriage.GetMatrix();
}

glm::vec3 Cannon::GetMuzzleWorldPosition() const {
    return shaft.GetMuzzleWorldPosition(YawWrappedMatrix());
}

glm::vec3 Cannon::GetForwardWorldDirection() const {
    return shaft.GetForwardWorldDirection(YawWrappedMatrix());
}

void Cannon::Draw(Shader& shader) {
    // The yaw-wrapped matrix is the world transform for everything
    // attached to the cannon: it rotates the whole assembly about the
    // cannon's current centre and then applies the carriage's own
    // position + transform.  The wheels, shaft, and carriage parts
    // all live relative to the carriage origin, so they share this
    // single parent matrix.
    glm::mat4 worldParent = YawWrappedMatrix();

    // Carriage::Draw would multiply by transform.GetMatrix() AGAIN,
    // so we use DrawAt to draw each part directly under worldParent.
    carriage.DrawAt(shader, worldParent);
    leftWheel.Draw(shader, worldParent);
    rightWheel.Draw(shader, worldParent);
    shaft.Draw(shader, worldParent);

    // Phase 7: muzzle flash. A bright sphere sitting just in front of
    // the muzzle, scaled by (1 + 1.5 * intensity) so it grows on fire
    // then shrinks to nothing as intensity decays.
    if (flashIntensity > 0.0f) {
        glm::vec3 muzzle = GetMuzzleWorldPosition();
        static Mesh flashMesh = Primitives::CreateSphere(0.30f, 12, 12, Palette::Flash);
        float scale = 1.0f + 1.5f * flashIntensity;
        glm::mat4 m = glm::translate(glm::mat4(1.0f), muzzle)
                    * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        flashMesh.Draw();
    }
}

void Cannon::Delete() {
    carriage.Delete();
    leftWheel.Delete();
    rightWheel.Delete();
    shaft.Delete();
}