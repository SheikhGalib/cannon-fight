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
    // was killed this call.
    bool CheckHit(glm::vec3 sphereCentre, float sphereRadius);

    int AlivePanelCount() const;
    int TotalPanelCount() const { return (int)panels.size(); }

    void Draw(Shader& shader);
    void Delete();

private:
    struct Panel {
        Mesh mesh;          // the panel itself (a thin box)
        glm::mat4 local;    // world matrix of this panel
        std::vector<Part> planks;   // horizontal plank strips on the face
        bool alive = true;
        float half[3];      // cached half-extents for AABB hit test

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