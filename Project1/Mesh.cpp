#include "Mesh.h"

Mesh::Mesh(std::vector<GLfloat> vertices, std::vector<GLuint> indices)
    // VBO/EBO upload their data to the GPU as soon as they're constructed,
    // which happens here in the member-initializer list, before the body runs.
    : vbo(vertices.data(), vertices.size() * sizeof(GLfloat)),
      ebo(indices.data(), indices.size() * sizeof(GLuint)),
      indexCount(static_cast<GLsizei>(indices.size()))
{
    // The VBO/EBO above were bound while THIS mesh's VAO wasn't current yet,
    // so re-bind everything here, with the VAO bound first, so the VAO
    // "remembers" which VBO/EBO/attribute layout belong to it.
    vao.Bind();
    vbo.Bind();
    ebo.Bind();

    // Attribute 0 = position (3 floats), attribute 1 = normal (3 floats),
    // attribute 2 = color (3 floats) - 9 floats per vertex total, matching
    // what lit.vert declares and what Primitives.cpp writes out.
    vao.LinkAttrib(vbo, 0, 3, GL_FLOAT, 9 * sizeof(GLfloat), (void*)0);
    vao.LinkAttrib(vbo, 1, 3, GL_FLOAT, 9 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    vao.LinkAttrib(vbo, 2, 3, GL_FLOAT, 9 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));

    vao.Unbind();
    vbo.Unbind();
    ebo.Unbind();
}

void Mesh::Draw() {
    vao.Bind();
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}

void Mesh::Delete() {
    vao.Delete();
    vbo.Delete();
    ebo.Delete();
}
