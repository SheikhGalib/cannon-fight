#include "Tree.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

Tree::Tree(glm::vec3 baseWorld,
           float trunkHeight, float trunkRadius,
           float crownHeight, float crownRadius)
    : trunkMesh(Primitives::CreateCylinder(trunkRadius, trunkHeight, 14, Palette::Bark,
                                            /*centered=*/false, /*yOffset=*/0.0f)),
      crownMesh(Primitives::CreateCone(crownRadius, 0.0f, crownHeight, 18, Palette::Leaf,
                                        /*centered=*/false, /*yOffset=*/trunkHeight)),
      trunkHeight(trunkHeight),
      trunkRadius(trunkRadius)
{
    // Place the tree at the requested spot. Y position is the BASE of the
    // trunk (on the ground), not the centre of the trunk - that's why both
    // pieces are built with centered=false: their local origin is at their
    // bottom, not their middle, so they grow up from the translation point.
    transform = glm::translate(glm::mat4(1.0f), baseWorld);

    // Meshes are built in the member-initializer list - Mesh has no default
    // constructor (it owns GPU buffer IDs, so it must be constructed via
    // Primitives::Create*). That's why both meshes appear up there.
}

void Tree::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform));
    trunkMesh.Draw();
    crownMesh.Draw();
}

void Tree::Delete() {
    trunkMesh.Delete();
    crownMesh.Delete();
}