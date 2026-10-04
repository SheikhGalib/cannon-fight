#ifndef WALL_H
#define WALL_H

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Part.h"
#include "shaderClass.h"

// A simple breakable wall built from a small grid of identical bricks. Each
// brick is one axis-aligned `Mesh` (a box) plus an `alive` flag. When a
// cannon ball passes within `radius` of a brick's centre, that brick is
// flagged dead; it disappears from the wall by simply being skipped on the
// next Draw().
//
// Wall space: the wall sits at a fixed position in world space (passed to
// the constructor). Internally the bricks are stacked on the ground, row
// by row, extending in +X (the direction the cannon points).
class Wall {
public:
    // centreWorld : world-space centre of the wall (X/Z - the wall lies flat
    //                on the ground at this X/Z; bricks grow upward from y=0).
    // rows, cols  : how many bricks vertically and along X.
    // brickSize   : (width along X, height along Y, depth along Z).
    Wall(glm::vec3 centreWorld, int rows, int cols, glm::vec3 brickSize);

    // Sphere-vs-AABB: marks any brick whose box intersects the projectile's
    // bounding sphere as destroyed. Returns true if at least one brick was
    // destroyed this call.
    bool CheckHit(glm::vec3 sphereCentre, float sphereRadius);

    // Number of bricks still standing. Useful for the small HUD on screen.
    int AliveBrickCount() const;

    void Draw(Shader& shader);
    void Delete();

private:
    struct Brick {
        Mesh mesh;
        glm::mat4 local;
        bool alive;
    };

    std::vector<Brick> bricks;
    glm::vec3 brickSize;
};

#endif