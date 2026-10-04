#ifndef MESH_H
#define MESH_H

#include <vector>
#include <glad/glad.h>
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

// Owns one shape's GPU data (a VAO + VBO + EBO triple) and knows how to draw
// itself. Mesh has no idea whether it's a cylinder, a box, or anything else -
// Primitives::CreateCylinder() (see Primitives.h) builds the vertex/index
// data, Mesh just stores it on the GPU and draws it. That separation is what
// lets Wheel and Shaft share this exact same class while looking completely
// different.
class Mesh {
public:
    // vertices are interleaved 9 floats per vertex: position.xyz, normal.xyz, color.rgb
    // (this matches the attribute layout lit.vert expects).
    Mesh(std::vector<GLfloat> vertices, std::vector<GLuint> indices);

    void Draw();
    void Delete();

private:
    VAO vao;
    VBO vbo;
    EBO ebo;
    GLsizei indexCount;
};

#endif
