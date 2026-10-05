#ifndef GOLDCREST_H
#define GOLDCREST_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"
#include "shaderClass.h"

// A pile of gold coins + an ornate chest + a tall flag, sitting on a
// stone pedestal in the centre of the castle compound.  Phase 8 marks
// this as the "objective" the attackers are trying to seize: when
// the battle sim ends with the attackers winning, the crest pulses
// gold; when the defenders win, it stays inert.
//
// Crest space: the pedestal sits on the ground at baseWorld.  The
// chest sits on top of the pedestal, the coins spill out, the flag
// rises from the back corner.
class GoldCrest {
public:
    GoldCrest(glm::vec3 baseWorld);

    // Phase 8: pulse the gold (set this true to start the pulsing
    // animation; false to stop).  Drawn at a brighter shade while
    // pulsing.
    void SetVictorious(bool v) { victorious = v; pulseT = 0.0f; }
    void Update(float dt);

    void Draw(Shader& shader);
    void Delete();

private:
    std::vector<Part> parts;
    bool victorious = false;
    float pulseT = 0.0f;            // 0..1 saw-tooth for pulse
};

#endif
