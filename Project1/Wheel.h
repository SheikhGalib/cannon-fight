#ifndef WHEEL_H
#define WHEEL_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"
#include "Transform.h"
#include "shaderClass.h"

// One spoked cartwheel, built the way a real one is: an iron tire around a
// wooden felloe (rim), ten spokes reaching in to a hub, and a brass cap on
// each end of the hub. Because the rim is a TUBE rather than a solid disc,
// you can see the ground between the spokes - that gap is most of what makes
// a wheel look like a wheel instead of a black doughnut.
//
// Wheel space: the wheel is a flat disc lying in the XY plane with its axle
// running along Z, so rolling is a rotation about Z - the same axis the
// barrel elevates around. Roll() below turns a distance driven into that
// angle; Phase 1 never calls it, but the carriage's driving in Phase 2 will
// (see docs/phase-2-plan.md).
class Wheel {
public:
    // radius       : outer edge of the iron tire.
    // width        : how thick the wheel is along its axle.
    // spokeCount   : number of spokes.
    // axlePosition : where the wheel's centre sits, relative to whatever
    //                parent matrix Draw() is given (today: the carriage).
    Wheel(float radius, float width, int spokeCount, glm::vec3 axlePosition);

    // Rolling without slipping: an arc `radius * angle` of the rim must equal
    // the distance travelled, and the wheel turns clockwise (seen from +Z)
    // when moving forward, hence the minus sign inside.
    void Roll(float distanceMoved);

    void Draw(Shader& shader, const glm::mat4& parentMatrix);
    void Delete();

    float GetRollDegrees() const { return transform.rotationDegrees; }

    // Only used by tools/RenderDocShots.cpp, to draw the wheel one stage at a
    // time for the pictures in docs/code-walkthrough.md. Part order is:
    //   0 tire | 1 felloe | 2..(2+spokes-1) spokes | then hub | cap +Z | cap -Z
    std::vector<Part>& PartsForDocs() { return parts; }

private:
    std::vector<Part> parts;
    Transform transform;   // position on the axle + the rolling rotation
    float radius;
};

#endif
