#ifndef CARRIAGE_H
#define CARRIAGE_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"
#include "Transform.h"
#include "shaderClass.h"

// The wooden gun carriage: the body that ties the two wheels together and
// holds the barrel up. Without it the wheels and barrel just float in the air
// as three unconnected objects, which is exactly the problem this class fixes.
//
// It is made of:
//   * two long trail beams, tilted so their rear ends rest on the ground -
//     this is the wedge-shaped silhouette a field cannon is recognised by;
//   * three transoms (cross-members) bolted between the beams;
//   * an iron trail spade at the very back, which is what digs into the earth
//     and stops the gun sliding when it fires;
//   * two cheeks, the uprights standing over the axle that carry the barrel's
//     trunnion pivot;
//   * a quoin block behind them, the wedge the breech rests on;
//   * the iron axle the wheels turn on, and the wooden bolster strapping that
//     axle to the beams.
//
// The carriage is also the ROOT of the cannon's scene graph. Its matrix is the
// parent of both wheels and of the barrel, so driving the carriage forward
// carries the whole gun with it - see GetMatrix() and docs/phase-2-plan.md.
class Carriage {
public:
    Carriage();
    Carriage(glm::vec3 initialPosition);

    // Moves the whole gun along +X (forward) / -X (back).
    void MoveForward(float distance);

    // Sets the world-space position directly.
    void SetPosition(glm::vec3 pos) { transform.position = pos; }
    glm::vec3 GetPosition() const { return transform.position; }

    // Phase 7: arbitrary horizontal (X/Z) translation. Used by Cannon
    // to drive along the cannon's yawed forward direction rather than
    // strict world +X.
    void MoveRaw(glm::vec3 delta);

    // Phase 5: firing recoil. `Fire()` instantly kicks the carriage back
    // by `kRecoilKick` metres (along -X, the opposite of the firing
    // direction).  Each subsequent `Update(dt)` eases it back toward 0
    // over `kRecoilReturn` seconds using a smooth decay so it feels like
    // the gun rolling back into battery rather than snapping in place.
    void Fire();
    void Update(float deltaTime);

    // The matrix that everything attached to the carriage - both wheels and
    // the barrel - should be drawn relative to.  This INCLUDES the
    // transient recoil offset, so the barrel + wheels visibly slide when
    // the gun fires.
    glm::mat4 GetMatrix() const;

    // Height of the top edge of a trail beam directly above/below x, in
    // carriage space. The cheeks and the quoin block use this so they always
    // sit ON the sloping beams instead of hovering over or sinking into them.
    static float BeamHeightAt(float x);

    void Draw(Shader& shader, const glm::mat4& parentMatrix);

    // Phase 7: like Draw() but does NOT multiply by the carriage's own
    // transform.GetMatrix().  Used by Cannon to draw the carriage with
    // an externally-composed world matrix (e.g. one that has the yaw
    // rotation baked in around the cannon's centre).
    void DrawAt(Shader& shader, const glm::mat4& worldMatrix);

    void Delete();

    // Only used by tools/RenderDocShots.cpp, to draw the carriage one stage at
    // a time for docs/code-walkthrough.md. Part order is:
    //   0-1 beams | 2-4 transoms | 5 spade | 6-7 cheeks | 8 quoin | 9 axle | 10 bolster
    std::vector<Part>& PartsForDocs() { return parts; }

private:
    std::vector<Part> parts;
    Transform transform;
    float recoilOffset = 0.0f;   // current additional -X offset from firing recoil
};

#endif
