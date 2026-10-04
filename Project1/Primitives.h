#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <glm/glm.hpp>
#include "Mesh.h"

// Reusable geometry generators. Wheel, Carriage and Shaft don't hand-write a
// single vertex between them - they just call these with different numbers.
//
// SHARED CONVENTION: everything below is built standing UP, i.e. its length
// runs along +Y and it is centred on X/Z. Nothing here takes a "which
// direction does it point" argument; pointing a shape sideways is the job of
// Local::AlongX / Local::AlongZ in Part.h. Keeping orientation out of the
// generators is what keeps them this short.
//
// Every vertex carries a position, an outward NORMAL (so lit.frag can shade
// it) and a flat color.
namespace Primitives {

    // A cone/frustum: a circular tube whose radius changes from bottom to top.
    // This is the workhorse - a plain cylinder is just the case where both
    // radii are equal, and the barrel's taper, muzzle swell and hub caps are
    // the cases where they differ.
    //   segments : how many flat sides the circle is built from (more = rounder).
    //   centered : true  -> spans Y = -height/2 .. +height/2
    //              false -> spans Y = 0 .. +height, so the shape can be rotated
    //                       about the end sitting at the local origin.
    //   yOffset  : shifts the finished shape along local Y, for stacking pieces
    //              at a chosen point along a longer assembly without giving
    //              each one its own matrix.
    Mesh CreateCone(float radiusBottom, float radiusTop, float height, int segments,
                    glm::vec3 color, bool centered = true, float yOffset = 0.0f);

    // A cone with both radii equal - the common case, named for readability.
    Mesh CreateCylinder(float radius, float height, int segments, glm::vec3 color,
                        bool centered = true, float yOffset = 0.0f);

    // A ring/washer: a cylinder with a cylindrical hole punched through it.
    // Used for the wheel's iron tire and wooden felloe (so you can see daylight
    // between the spokes, which is what makes a wheel read as a wheel) and for
    // the brass bands around the barrel.
    Mesh CreateTube(float innerRadius, float outerRadius, float height, int segments,
                    glm::vec3 color, bool centered = true, float yOffset = 0.0f);

    // An axis-aligned box centred on the local origin.
    //   width  spans X, height spans Y, depth spans Z.
    // Used for every wooden member of the carriage and for the wheel spokes.
    Mesh CreateBox(float width, float height, float depth, glm::vec3 color);

    // A UV sphere centred on the local origin - the barrel's rounded breech
    // and the cascabel knob behind it.
    //   stacks : horizontal rings from pole to pole.  slices : segments around.
    Mesh CreateSphere(float radius, int stacks, int slices, glm::vec3 color);

    // A flat horizontal quad in the XZ plane, centred on the local origin,
    // facing up (+Y) - the ground.
    Mesh CreatePlane(float width, float depth, glm::vec3 color);

}

#endif
