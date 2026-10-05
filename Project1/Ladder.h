#ifndef LADDER_H
#define LADDER_H

#include <vector>
#include <glm/glm.hpp>
#include "Part.h"
#include "shaderClass.h"

// A wooden medieval ladder leaning against the inner castle wall,
// allowing sentries to climb between the courtyard and the battlements.
class Ladder {
public:
    Ladder(glm::vec3 bottomPos, glm::vec3 topPos, float width = 0.50f, int numRungs = 8);

    void Draw(Shader& shader);
    void Delete();

    glm::vec3 GetBottomPosition() const { return bottom; }
    glm::vec3 GetTopPosition() const { return top; }

private:
    std::vector<Part> parts;
    glm::mat4 transform;
    glm::vec3 bottom;
    glm::vec3 top;
};

#endif
