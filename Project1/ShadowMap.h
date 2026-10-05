#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// High-quality depth-map shadow mapping with FBO depth attachment.
// Renders the scene from the directional light's perspective into a 2048x2048
// depth texture, used by lit.frag with Percentage-Closer Filtering (PCF).
class ShadowMap {
public:
    ShadowMap();
    ~ShadowMap();

    bool Init(int resolution = 2048);
    void BindForWriting();
    void Unbind(int viewportWidth, int viewportHeight);
    void BindDepthTexture(GLenum textureUnit = GL_TEXTURE0);

    // Computes an orthographic light projection matrix tightly covering
    // the battlefield, cannons, river, and castle.
    glm::mat4 ComputeLightSpaceMatrix(glm::vec3 lightDir, glm::vec3 sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f));

    GLuint GetDepthMap() const { return depthMap; }
    glm::mat4 GetLightSpaceMatrix() const { return lightSpaceMatrix; }
    int GetResolution() const { return resolution; }

    void Delete();

private:
    GLuint fbo = 0;
    GLuint depthMap = 0;
    int resolution = 2048;
    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
};

#endif
