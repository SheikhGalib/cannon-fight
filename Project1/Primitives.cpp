#include "Primitives.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace Primitives {

    // Appends one vertex (position + normal + the flat color) to a vertex list.
    // Every CreateX() function below shares this so the vertex layout only
    // has to be written down in one place - it must match Mesh.cpp's LinkAttrib calls.
    static void PushVertex(std::vector<GLfloat>& vertices, glm::vec3 pos, glm::vec3 normal, glm::vec3 color) {
        vertices.push_back(pos.x);
        vertices.push_back(pos.y);
        vertices.push_back(pos.z);
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);
        vertices.push_back(normal.z);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
    }

    // Adds a ring of `segments` points at height y, on a circle of the given
    // radius, each carrying the given normal. `normalFn` decides the normal
    // from the point's angle, because a side wall wants an outward-sideways
    // normal while a flat cap wants a straight up/down one.
    //
    // Returns the index of the first vertex of the ring, which the caller
    // needs in order to build triangles out of it.
    static GLuint AddRing(std::vector<GLfloat>& vertices, float radius, float y,
                          int segments, glm::vec3 color, glm::vec3 flatNormal,
                          bool useRadialNormal, float radialNormalY = 0.0f) {
        GLuint first = static_cast<GLuint>(vertices.size() / 9);
        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * glm::pi<float>() * float(i) / float(segments);
            float cx = cosf(angle);
            float cz = sinf(angle);
            glm::vec3 normal = useRadialNormal
                ? glm::normalize(glm::vec3(cx, radialNormalY, cz))
                : flatNormal;
            PushVertex(vertices, glm::vec3(radius * cx, y, radius * cz), normal, color);
        }
        return first;
    }

    // Stitches two rings (each `segments` points, same winding) into a wall of
    // quads: ring A at one height, ring B at another.
    static void StitchRings(std::vector<GLuint>& indices, GLuint ringA, GLuint ringB, int segments) {
        for (int i = 0; i < segments; i++) {
            int next = (i + 1) % segments; // wraps back to 0, closing the circle
            GLuint a0 = ringA + i, a1 = ringA + next;
            GLuint b0 = ringB + i, b1 = ringB + next;
            indices.insert(indices.end(), { a0, a1, b1 });
            indices.insert(indices.end(), { a0, b1, b0 });
        }
    }

    Mesh CreateCone(float radiusBottom, float radiusTop, float height, int segments,
                    glm::vec3 color, bool centered, float yOffset) {
        std::vector<GLfloat> vertices;
        std::vector<GLuint> indices;

        const float yMin = (centered ? -height / 2.0f : 0.0f) + yOffset;
        const float yMax = (centered ?  height / 2.0f : height) + yOffset;

        // The side wall and the two caps meet at the same rings of points in
        // space, but a cap's flat top/bottom needs a straight-up-or-down
        // normal while the side wall needs an outward-pointing-sideways
        // normal at that exact same spot. One vertex can only store one
        // normal, so each surface gets its OWN copy of the ring vertices,
        // each with the normal that's correct for that surface.

        // For a tapered wall the outward normal tilts: if the shape narrows
        // going up, the surface leans inward, so the normal leans outward-up
        // by exactly the slope of that lean.
        const float slopeY = (height > 0.0f) ? (radiusBottom - radiusTop) / height : 0.0f;

        GLuint wallBottom = AddRing(vertices, radiusBottom, yMin, segments, color, glm::vec3(0.0f), true, slopeY);
        GLuint wallTop    = AddRing(vertices, radiusTop,    yMax, segments, color, glm::vec3(0.0f), true, slopeY);
        StitchRings(indices, wallBottom, wallTop, segments);

        // --- bottom cap: its own ring plus a centre point, normal pointing down
        GLuint capBottom = AddRing(vertices, radiusBottom, yMin, segments, color, glm::vec3(0.0f, -1.0f, 0.0f), false);
        GLuint bottomCentre = static_cast<GLuint>(vertices.size() / 9);
        PushVertex(vertices, glm::vec3(0.0f, yMin, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), color);
        for (int i = 0; i < segments; i++) {
            int next = (i + 1) % segments;
            indices.insert(indices.end(), { bottomCentre, capBottom + static_cast<GLuint>(next), capBottom + static_cast<GLuint>(i) });
        }

        // --- top cap: same idea, normal pointing up
        GLuint capTop = AddRing(vertices, radiusTop, yMax, segments, color, glm::vec3(0.0f, 1.0f, 0.0f), false);
        GLuint topCentre = static_cast<GLuint>(vertices.size() / 9);
        PushVertex(vertices, glm::vec3(0.0f, yMax, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), color);
        for (int i = 0; i < segments; i++) {
            int next = (i + 1) % segments;
            indices.insert(indices.end(), { topCentre, capTop + static_cast<GLuint>(i), capTop + static_cast<GLuint>(next) });
        }

        return Mesh(vertices, indices);
    }

    Mesh CreateCylinder(float radius, float height, int segments, glm::vec3 color,
                        bool centered, float yOffset) {
        return CreateCone(radius, radius, height, segments, color, centered, yOffset);
    }

    Mesh CreateTube(float innerRadius, float outerRadius, float height, int segments,
                    glm::vec3 color, bool centered, float yOffset) {
        std::vector<GLfloat> vertices;
        std::vector<GLuint> indices;

        const float yMin = (centered ? -height / 2.0f : 0.0f) + yOffset;
        const float yMax = (centered ?  height / 2.0f : height) + yOffset;

        // A tube is four surfaces: the outer wall, the inner wall (whose
        // normals point INWARD, towards the axis, because that's the side you
        // actually see through the hole), and two flat rings closing the top
        // and bottom. Each gets its own copy of the ring vertices, same
        // reason as in CreateCone.

        GLuint outerBottom = AddRing(vertices, outerRadius, yMin, segments, color, glm::vec3(0.0f), true);
        GLuint outerTop    = AddRing(vertices, outerRadius, yMax, segments, color, glm::vec3(0.0f), true);
        StitchRings(indices, outerBottom, outerTop, segments);

        // Inner wall: AddRing always makes normals point away from the axis, so
        // for the hole we build the two rings and then negate their normals,
        // turning them around to face the axis instead.
        GLuint innerBottom = AddRing(vertices, innerRadius, yMin, segments, color, glm::vec3(0.0f), true);
        GLuint innerTop    = AddRing(vertices, innerRadius, yMax, segments, color, glm::vec3(0.0f), true);
        for (GLuint v = innerBottom; v < innerTop + static_cast<GLuint>(segments); v++) {
            vertices[v * 9 + 3] = -vertices[v * 9 + 3];
            vertices[v * 9 + 4] = -vertices[v * 9 + 4];
            vertices[v * 9 + 5] = -vertices[v * 9 + 5];
        }
        StitchRings(indices, innerTop, innerBottom, segments); // reversed order = faces inward

        // Flat annulus at the top, and another at the bottom.
        GLuint faceTopOuter = AddRing(vertices, outerRadius, yMax, segments, color, glm::vec3(0.0f, 1.0f, 0.0f), false);
        GLuint faceTopInner = AddRing(vertices, innerRadius, yMax, segments, color, glm::vec3(0.0f, 1.0f, 0.0f), false);
        StitchRings(indices, faceTopInner, faceTopOuter, segments);

        GLuint faceBotOuter = AddRing(vertices, outerRadius, yMin, segments, color, glm::vec3(0.0f, -1.0f, 0.0f), false);
        GLuint faceBotInner = AddRing(vertices, innerRadius, yMin, segments, color, glm::vec3(0.0f, -1.0f, 0.0f), false);
        StitchRings(indices, faceBotOuter, faceBotInner, segments);

        return Mesh(vertices, indices);
    }

    Mesh CreateBox(float width, float height, float depth, glm::vec3 color) {
        std::vector<GLfloat> vertices;
        std::vector<GLuint> indices;

        const float hx = width / 2.0f, hy = height / 2.0f, hz = depth / 2.0f;

        // Six flat faces. A box corner is shared by three faces that each need
        // a different normal, so - same rule as everywhere else here - each
        // face gets its own four corner vertices: 6 * 4 = 24 vertices total.
        // Each face is listed as its four corners in counter-clockwise order
        // when viewed from outside.
        struct Face { glm::vec3 normal, a, b, c, d; };
        const Face faces[6] = {
            { { 0, 0,  1}, {-hx,-hy, hz}, { hx,-hy, hz}, { hx, hy, hz}, {-hx, hy, hz} }, // front (+Z)
            { { 0, 0, -1}, { hx,-hy,-hz}, {-hx,-hy,-hz}, {-hx, hy,-hz}, { hx, hy,-hz} }, // back  (-Z)
            { { 1, 0,  0}, { hx,-hy, hz}, { hx,-hy,-hz}, { hx, hy,-hz}, { hx, hy, hz} }, // right (+X)
            { {-1, 0,  0}, {-hx,-hy,-hz}, {-hx,-hy, hz}, {-hx, hy, hz}, {-hx, hy,-hz} }, // left  (-X)
            { { 0, 1,  0}, {-hx, hy, hz}, { hx, hy, hz}, { hx, hy,-hz}, {-hx, hy,-hz} }, // top   (+Y)
            { { 0,-1,  0}, {-hx,-hy,-hz}, { hx,-hy,-hz}, { hx,-hy, hz}, {-hx,-hy, hz} }, // bottom(-Y)
        };

        for (const Face& f : faces) {
            GLuint base = static_cast<GLuint>(vertices.size() / 9);
            PushVertex(vertices, f.a, f.normal, color);
            PushVertex(vertices, f.b, f.normal, color);
            PushVertex(vertices, f.c, f.normal, color);
            PushVertex(vertices, f.d, f.normal, color);
            indices.insert(indices.end(), { base, base + 1, base + 2 });
            indices.insert(indices.end(), { base, base + 2, base + 3 });
        }

        return Mesh(vertices, indices);
    }

    Mesh CreateSphere(float radius, int stacks, int slices, glm::vec3 color) {
        std::vector<GLfloat> vertices;
        std::vector<GLuint> indices;

        // Walk from the north pole (stack 0, where cos(0) = 1 puts us at +radius)
        // down to the south pole (stack `stacks`, where cos(pi) = -1), laying
        // down a ring of `slices + 1` points at each latitude on the way. On a
        // sphere the outward normal at a point is simply that point's own
        // direction from the centre, which is the position divided by radius.
        for (int stack = 0; stack <= stacks; stack++) {
            float phi = glm::pi<float>() * float(stack) / float(stacks); // 0 .. pi
            float y = cosf(phi);
            float ringRadius = sinf(phi);
            for (int slice = 0; slice <= slices; slice++) {
                float theta = 2.0f * glm::pi<float>() * float(slice) / float(slices);
                glm::vec3 dir(ringRadius * cosf(theta), y, ringRadius * sinf(theta));
                PushVertex(vertices, dir * radius, dir, color);
            }
        }

        // Two triangles per quad between neighbouring latitude rings. The rows
        // are (slices + 1) wide, which is why the stride below is slices + 1.
        for (int stack = 0; stack < stacks; stack++) {
            for (int slice = 0; slice < slices; slice++) {
                GLuint row0 = static_cast<GLuint>(stack * (slices + 1) + slice);
                GLuint row1 = static_cast<GLuint>((stack + 1) * (slices + 1) + slice);
                indices.insert(indices.end(), { row0, row1, row1 + 1 });
                indices.insert(indices.end(), { row0, row1 + 1, row0 + 1 });
            }
        }

        return Mesh(vertices, indices);
    }

    Mesh CreatePlane(float width, float depth, glm::vec3 color) {
        std::vector<GLfloat> vertices;
        std::vector<GLuint> indices;

        const float hw = width / 2.0f;
        const float hd = depth / 2.0f;
        const glm::vec3 up(0.0f, 1.0f, 0.0f);

        PushVertex(vertices, glm::vec3(-hw, 0.0f, -hd), up, color);
        PushVertex(vertices, glm::vec3(hw, 0.0f, -hd), up, color);
        PushVertex(vertices, glm::vec3(hw, 0.0f, hd), up, color);
        PushVertex(vertices, glm::vec3(-hw, 0.0f, hd), up, color);

        indices = { 0, 1, 2, 0, 2, 3 };

        return Mesh(vertices, indices);
    }

}
