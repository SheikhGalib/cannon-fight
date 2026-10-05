#ifndef FORTGATE_H
#define FORTGATE_H

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Part.h"
#include "shaderClass.h"

// A short front section of a medieval fortification: a breakable brick wall
// on each side of a doorway opening, with a heavy wooden lintel bar across
// the top of the doorway and two stone pillars anchoring it. The bricks are
// breakable (same sphere-vs-AABB test as Phase 2's `Wall`); the lintel and
// pillars are not.
//
// FortGate space: the gate opening is centred on (centreWorld.x, centreWorld.z).
// Bricks stack up to lintelHeight, with a gap of gateWidth at the centre.
// Above the gap sits the lintel. The two pillars sit at the outer ends.
//
//         +------+      +------+
///        |      |  lintel  |     |
///        | brick brick   brick  brick
///        | brick brick   brick  brick
///        | brick brick   brick  brick
///        | brick brick   brick  brick
///        +------+      +------+   <-- pillars (outer ends)
///        |  pillar  |   |  pillar  |
///        ─────────────────────────  <-- ground
///              ^ gate opening ^
class FortGate {
public:
    // centreWorld : world-space centre of the gate opening.
    // width       : total wall width including pillars.
    // height      : total wall height including lintel.
    // depth       : how thick the wall is (along Z).
    // gateWidth   : width of the doorway opening at the centre.
    // rows        : how many brick rows between the pillars.
    FortGate(glm::vec3 centreWorld,
             float width, float height, float depth,
             float gateWidth, int rows,
             bool alongZ = true);

    // Sphere-vs-AABB against the breakable bricks. Same idea as Wall.
    bool CheckHit(glm::vec3 sphereCentre, float sphereRadius);

    int AliveBrickCount() const;
    int TotalBrickCount() const { return (int)bricks.size(); }

    void Draw(Shader& shader);
    void Delete();

private:
    struct Brick {
        Mesh mesh;
        glm::mat4 local;
        bool alive;

        // Phase 7: each cannon hit strips kDamagePerHit of health.
        // While health > 0 the brick darkens (lerp from stone to
        // stoneDark); at 0 it disappears.
        float health = 1.0f;
    };

    std::vector<Brick> bricks;
    std::vector<Part>  staticParts;  // lintel + 2 pillars - never break
    glm::vec3 brickSize;
};

#endif