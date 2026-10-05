#ifndef DOOR_H
#define DOOR_H

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Part.h"
#include "shaderClass.h"

// A pair of wooden fortress doors hung in a doorway. Each panel is a flat
// vertical box; in front of the panel sit a few horizontal "plank" boxes
// (purely visual decoration - they share the panel's alive flag so they
// disappear when the panel breaks).
//
// Both panels are breakable independently: a cannon ball that hits the
// left panel only kills the left door; the right door stays intact until
// hit.
//
// Collision uses the standard sphere-vs-AABB test against each panel's
// own (panelWidth, panelHeight, panelDepth) AABB. The panel is thin
// (panelDepth ~ 0.10 m) so it reads as a real door and the ball cannot
// "skip over" it.
class Door {
public:
    // centreWorld   : world-space centre of the doorway (same as FortGate's).
    // doorHeight    : total height of the doorway (and the door panels).
    // doorWidth     : total width of the doorway (split into two equal panels).
    // panelDepth    : how thick each panel is along Z.
    // plankCount    : how many horizontal plank strips to draw across each
    //                 panel face for that wooden-door look.
    Door(glm::vec3 centreWorld,
         float doorHeight, float doorWidth, float panelDepth, int plankCount);

    // Sphere-vs-AABB against the door panels. Returns true if any panel
    // was killed this call.  "Killed" in Phase 5+ means the panel
    // detaches and flies outward (see Panel below).
    bool CheckHit(glm::vec3 sphereCentre, float sphereRadius);

    // Each frame, advance the break physics for any panel that's been
    // knocked loose.  Called once per frame from Main.
    void Update(float deltaTime);

    int AlivePanelCount() const;
    int TotalPanelCount() const { return (int)panels.size(); }

    void Draw(Shader& shader);
    void Delete();

private:
    struct Panel {
        Mesh mesh;          // the panel itself (a thin box)
        glm::mat4 local;    // world matrix of this panel (pre-hit rest pose)
        std::vector<Part> planks;   // horizontal plank strips on the face
        bool alive = true;
        bool flying = false;        // true once a ball has knocked it loose
        float half[3];      // cached half-extents for AABB hit test

        // Phase 7: each cannon hit removes 25% of the panel's health.
        // At 0, the panel detaches and break-physics takes over. While
        // health > 0, the panel is still attached but visibly tilted
        // (damagePercent * 15 deg) into the doorway so the player can
        // see the cumulative damage.
        float health = 1.0f;

        // --- Break-physics state (Phase 5+) ----------------------------
        // Each panel "hinges" on its inner vertical edge (the edge facing
        // the doorway centre), so it can swing outward instead of
        // pivoting about its own centre.  hingeLocal is that edge in
        // world space at the moment of the hit; once a panel is flying,
        // we integrate angular velocity around that point and let it
        // topple away from the castle.
        glm::vec3 hingeWorld = glm::vec3(0.0f);
        glm::vec3 angularVel = glm::vec3(0.0f);   // rad / sec
        glm::vec3 linearVel  = glm::vec3(0.0f);   // m / sec (mostly +X)
        glm::vec3 euler      = glm::vec3(0.0f);   // accumulated rotation

        // Explicit ctor so we can emplace_back without a default ctor.
        // (Mesh has no default ctor because it owns GPU buffer IDs.)
        Panel(Mesh m, glm::mat4 loc, std::vector<Part> pl, bool a)
            : mesh(std::move(m)), local(loc), planks(std::move(pl)), alive(a) {
            half[0] = half[1] = half[2] = 0.0f;
        }
    };

    std::vector<Panel> panels;
    glm::vec3 panelSize;    // (panelWidth, doorHeight, panelDepth)
};

#endif