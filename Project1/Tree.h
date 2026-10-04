#ifndef TREE_H
#define TREE_H

#include <glm/glm.hpp>
#include "Mesh.h"
#include "shaderClass.h"

// A very basic tree: a brown cylindrical trunk with a green conical "leaf"
// on top, the way a kid draws a tree. Phase 3 places a row of these behind
// the fort so the cannon has something leafy to look at.
//
// Tree space: the trunk sits on the ground at the tree's origin (y goes
// from 0 to trunkHeight), and the leaf cone sits on top of the trunk
// (its base at y = trunkHeight, growing up to y = trunkHeight + crownHeight).
// So the whole tree can be drawn with one translation matrix.
class Tree {
public:
    // baseWorld : where the trunk meets the ground, in world space.
    // trunkHeight, trunkRadius : the cylinder's size.
    // crownHeight, crownRadius : the cone's size (the "leaves").
    Tree(glm::vec3 baseWorld,
         float trunkHeight, float trunkRadius,
         float crownHeight, float crownRadius);

    void Draw(Shader& shader);
    void Delete();

    float GetRadius() const { return trunkRadius; }

private:
    glm::mat4 transform;  // base position
    Mesh trunkMesh;
    Mesh crownMesh;
    float trunkHeight;
    float trunkRadius;
};

#endif