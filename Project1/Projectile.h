#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <glm/glm.hpp>
#include "Mesh.h"
#include "shaderClass.h"

// One cannon ball in flight. A `Projectile` is the smallest object in the
// scene: just a sphere mesh, a world-space position, and a world-space
// velocity. Each frame `Main.cpp` calls Update() to advance the position
// under gravity, then Draw() to render the ball at its current position.
//
// Why the position and velocity are world-space, not local to anything: the
// ball is no longer "part of" the gun once it leaves the muzzle. It does
// its own thing. The cannon's hierarchical transforms are only used at the
// instant of FIRE, to find the spawn position and the spawn velocity - see
// Main.cpp and Shaft::GetMuzzleWorldPosition / GetForwardWorldDirection.
class Projectile {
public:
    // muzzlePos   : world-space position the ball appears at.
    // velocity    : world-space velocity the ball starts with.
    // radius      : ball radius in metres. Drives both the sphere mesh and
    //               the collision test used against the wall.
    Projectile(glm::vec3 muzzlePos, glm::vec3 velocity, float radius);

    // Advance the ball under gravity. Update() does NOT remove the ball -
    // it just sets the `dead` flag when the ball has hit the ground or aged
    // out, so the caller can drop it from its list.
    void Update(float deltaTime, float gravity);

    void Draw(Shader& shader);

    // For Wall::CheckHit. The ball is approximated as an axis-aligned box
    // around its position - the radius pad lets fast-moving balls still
    // register a hit when they would otherwise slip between frames.
    glm::vec3 GetPosition() const { return position; }
    glm::vec3 GetVelocity() const { return velocity; }
    float GetRadius() const { return radius; }
    bool IsDead() const { return dead; }
    // Phase 8: the projectile loop calls this when the ball collides
    // with a non-breakable surface (e.g. a stone curtain wall) so it
    // stops and the caller can drop it from the active list.
    void Kill() { dead = true; }

    void Delete();

    static constexpr float LifetimeSeconds = 6.0f; // safety cap so missed shots don't pile up
    static constexpr float DefaultRadius   = 0.10f;
    // Tuned so that a 12-degree shot from a stationary cannon (muzzle at
    // ~(1.8, 1.5, 0)) lands near the wall (centre at x = 12) at about chest
    // height for the dummy robot (~1.0 m).
    static constexpr float DefaultSpeed    = 14.0f;
    static constexpr float Gravity         = 9.81f;

private:
    Mesh mesh;
    glm::vec3 position;
    glm::vec3 velocity;
    float radius;
    float ageSeconds = 0.0f;
    bool dead = false;
};

#endif