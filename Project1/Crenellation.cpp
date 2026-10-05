#include "Crenellation.h"
#include "Primitives.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Crenellation {

std::vector<Part> AlongX(float baseX, float baseY, float baseZ,
                         float lengthX,
                         float merlonW, float merlonH, float merlonD,
                         float gap, glm::vec3 color) {
    std::vector<Part> out;
    const float pitch = merlonW + gap;          // one merlon + one gap
    const int count = (int)std::floor(lengthX / pitch);
    if (count <= 0) return out;

    for (int i = 0; i < count; i++) {
        // The first merlon's centre sits at baseX + merlonW/2 (so its left
        // edge is at baseX), the second at baseX + pitch + merlonW/2, ...
        const float cx = baseX + merlonW * 0.5f + float(i) * pitch;
        const float cy = baseY + merlonH * 0.5f;
        const float cz = baseZ + merlonD * 0.5f;
        out.push_back({
            Primitives::CreateBox(merlonW, merlonH, merlonD, color),
            glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, cz))
        });
    }
    return out;
}

std::vector<Part> AlongZ(float baseX, float baseY, float baseZ,
                         float lengthZ,
                         float merlonW, float merlonH, float merlonD,
                         float gap, glm::vec3 color) {
    std::vector<Part> out;
    const float pitch = merlonW + gap;
    const int count = (int)std::floor(lengthZ / pitch);
    if (count <= 0) return out;

    for (int i = 0; i < count; i++) {
        // Along Z, "merlonW" now runs along Z and "merlonD" runs along X.
        // Caller supplies the merlon in the obvious way: merlonW is the
        // "thickness" perpendicular to the row, merlonD is the visible width.
        // We treat the user input as: merlonW = depth of each merlon (along
        // its own axis), merlonD = width perpendicular to the row.
        const float cz = baseZ + merlonW * 0.5f + float(i) * pitch;
        const float cy = baseY + merlonH * 0.5f;
        const float cx = baseX + merlonD * 0.5f;
        out.push_back({
            Primitives::CreateBox(merlonD, merlonH, merlonW, color),
            glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, cz))
        });
    }
    return out;
}

} // namespace Crenellation
