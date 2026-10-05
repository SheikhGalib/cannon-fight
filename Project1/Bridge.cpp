#include "Bridge.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <cmath>

Bridge::Bridge(glm::vec3 centre, float lengthX, float widthZ)
    : centre(centre), lengthX(lengthX), widthZ(widthZ)
{
    // Hinge edge: the inner (castle-side) edge of the bridge, at the
    // ground. Rotation about this axis swings the deck up vertically.
    hinge = glm::vec3(centre.x + lengthX * 0.5f, centre.y, centre.z);

    // ---- The bridge deck: a thin slab, slightly above the water -----
    const float deckH = 0.15f;
    parts.push_back({
        Primitives::CreateBox(lengthX, deckH, widthZ, Palette::Bridge),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(centre.x, deckH * 0.5f, centre.z)),
        glm::translate(glm::mat4(1.0f),
            glm::vec3(centre.x, deckH * 0.5f, centre.z))
    });

    // ---- Two low rails along the long sides ----------------------
    const float railH = 0.50f;
    const float railT = 0.10f;
    const float halfW = widthZ * 0.5f;
    const float railOffset = halfW + railT * 0.5f;

    for (float zSide : { -railOffset, +railOffset }) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f),
            glm::vec3(centre.x, railH * 0.5f, centre.z + zSide));
        parts.push_back({
            Primitives::CreateBox(lengthX, railH, railT, Palette::Bridge),
            m, m
        });
    }

    // ---- Four short posts at each end of each rail --------------
    const float postW = 0.12f;
    const float halfL = lengthX * 0.5f;
    const float postH = railH + 0.10f;
    for (float xEnd : { -halfL, +halfL }) {
        for (float zSide : { -railOffset, +railOffset }) {
            glm::mat4 m = glm::translate(glm::mat4(1.0f),
                glm::vec3(centre.x + xEnd, postH * 0.5f, centre.z + zSide));
            parts.push_back({
                Primitives::CreateBox(postW, postH, postW, Palette::Bridge),
                m, m
            });
        }
    }

    // ---- Phase 6: chains -------------------------------------------
    // Build one chain per rail. Each chain has kChainLinks torus-tube
    // links; the actual positions are recomputed every frame in Update()
    // so the chain shortens as the bridge rises. Each link shares the
    // same DarkIron torus-tube mesh but is rotated ±90° around Z
    // alternately so they look like a real linked chain.
    for (float zSide : { -railOffset, +railOffset }) {
        Chain ch;
        // Inner radius 0.03, outer 0.07, height 0.18 (along the chain's
        // own Y axis).  8 segments is plenty at this small scale.
        Mesh linkMesh = Primitives::CreateTube(0.03f, 0.07f, 0.18f, 8,
                                                Palette::DarkIron,
                                                /*centered=*/true,
                                                /*yOffset=*/0.0f);
        for (int i = 0; i < kChainLinks; i++) {
            // Each link is rotated 90° around Z relative to the previous
            // one so the rings alternate orientation.
            glm::mat4 restM = glm::mat4(1.0f);
            float rollZ = (i % 2 == 0) ? 0.0f : 90.0f;
            restM = glm::rotate(restM, glm::radians(rollZ),
                                glm::vec3(0.0f, 0.0f, 1.0f));
            ch.links.push_back({ linkMesh, restM, restM });
        }
        chains.push_back(std::move(ch));
    }
}

void Bridge::SetRaised(bool raised) {
    targetAngleDeg = raised ? 90.0f : 0.0f;
}

void Bridge::Update(float deltaTime) {
    // Lerp currentAngleDeg towards targetAngleDeg.
    float diff = targetAngleDeg - currentAngleDeg;
    if (std::fabs(diff) > 0.01f) {
        float step = kAngularRateDegPerSec * deltaTime;
        if (std::fabs(diff) <= step) {
            currentAngleDeg = targetAngleDeg;
        } else {
            currentAngleDeg += (diff > 0 ? step : -step);
        }
    }

    // Re-derive each part's local matrix about the hinge.
    // The hinge is at hinge (world), and we want to rotate about the
    // Z axis (so the bridge swings up vertically along its length).
    glm::mat4 R = glm::rotate(glm::mat4(1.0f),
                              glm::radians(-currentAngleDeg),
                              glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 TnegH = glm::translate(glm::mat4(1.0f), -hinge);
    glm::mat4 TposH = glm::translate(glm::mat4(1.0f),  hinge);
    for (AnimatedPart& p : parts) {
        p.local = TposH * R * TnegH * p.restLocal;
    }

    // Re-derive chain link positions.
    // The chain hangs from (hinge.x, chainAnchorY, hinge.z ± railOffset)
    // down to a point on the inner end of the rail. The rail's inner
    // end, in world space, is at (hinge.x, hinge.y + railH/2*cosθ +
    // lengthX/2*(1 - cosθ) * - ?, hinge.z ± railOffset).
    // Simpler: the rail's INNER end stays at the hinge (the hinge is
    // exactly at the rail's inner-edge bottom corner), and the rail
    // extends OUTWARD and UPWARD as the bridge rises.
    //
    // The bridge deck centre starts at centre (when angle=0). The deck
    // extends from x = centre.x - lengthX/2 (outer edge) to x = centre.x
    // + lengthX/2 (hinge). When the bridge rotates 90°, the outer edge
    // swings UP and ends up directly above the hinge at y = lengthX.
    //
    // For the chain to attach to a sensible point on the bridge, we
    // pick a point that's at the inner end of the rail — at the hinge
    // (x = hinge.x, y = 0.05 above ground).  The chain length is the
    // distance from that anchor down to the gatehouse top:
    //
    //   when bridge is flat:  chain stretches straight down from
    //   (hinge.x, chainAnchorY, hinge.z ± rOff) to (hinge.x, 0.05, ...).
    //
    // The chain doesn't actually shorten as the bridge raises (since
    // the inner end of the rail stays at the hinge), but its middle
    // bows outward as the rail rotates. We model the chain as a
    // straight line for simplicity — it'll look a bit stretched when
    // the bridge is up, but visually it reads as "a chain hanging
    // from the tower down to the bridge end".

    const float halfW  = widthZ * 0.5f;
    const float railT  = 0.10f;
    const float railOffset = halfW + railT * 0.5f;

    for (size_t cIdx = 0; cIdx < chains.size(); cIdx++) {
        Chain& ch = chains[cIdx];
        float zSide = (cIdx == 0) ? -railOffset : +railOffset;
        glm::vec3 topWorld    = glm::vec3(hinge.x, chainAnchorY, hinge.z + zSide);
        glm::vec3 bottomWorld = glm::vec3(hinge.x, 0.10f,         hinge.z + zSide);
        for (int i = 0; i < kChainLinks; i++) {
            float t = (kChainLinks == 1) ? 0.0f : float(i) / float(kChainLinks - 1);
            glm::vec3 linkCentre = topWorld * (1.0f - t) + bottomWorld * t;
            // The link's restLocal just has the alternating Z-roll; we
            // overlay a translation to put it at linkCentre.
            glm::mat4 T = glm::translate(glm::mat4(1.0f), linkCentre);
            ch.links[i].local = T * ch.links[i].restLocal;
        }
    }
}

void Bridge::Draw(Shader& shader) {
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    for (AnimatedPart& p : parts) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(p.local));
        p.mesh.Draw();
    }
    if (chainsVisible) {
        for (Chain& ch : chains) {
            for (AnimatedPart& link : ch.links) {
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(link.local));
                link.mesh.Draw();
            }
        }
    }
}

void Bridge::Delete() {
    for (AnimatedPart& p : parts) p.mesh.Delete();
    for (Chain& ch : chains) {
        for (AnimatedPart& link : ch.links) link.mesh.Delete();
    }
}