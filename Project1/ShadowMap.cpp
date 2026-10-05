#include "ShadowMap.h"
#include <iostream>

ShadowMap::ShadowMap() {}

ShadowMap::~ShadowMap() {
    Delete();
}

bool ShadowMap::Init(int res) {
    resolution = res;

    glGenFramebuffers(1, &fbo);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ShadowMap FBO incomplete! Status: " << status << std::endl;
        return false;
    }
    return true;
}

void ShadowMap::BindForWriting() {
    glViewport(0, 0, resolution, resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    // Enable polygon offset or slope-scaled bias to help prevent acne
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
}

void ShadowMap::Unbind(int viewportWidth, int viewportHeight) {
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportWidth, viewportHeight);
}

void ShadowMap::BindDepthTexture(GLenum textureUnit) {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, depthMap);
}

glm::mat4 ShadowMap::ComputeLightSpaceMatrix(glm::vec3 lightDir, glm::vec3 sceneCenter) {
    // Orthographic frustum covering cannons (x = -25), moat (x = -4), and castle (x = 0..24, z = -18..+18)
    float orthoHalfWidth  = 38.0f;
    float orthoHalfHeight = 35.0f;
    float nearPlane       = 1.0f;
    float farPlane        = 180.0f;

    glm::mat4 lightProjection = glm::ortho(-orthoHalfWidth, orthoHalfWidth,
                                           -orthoHalfHeight, orthoHalfHeight,
                                           nearPlane, farPlane);

    glm::vec3 lightNormalized = glm::normalize(-lightDir);
    glm::vec3 lightPos = sceneCenter + lightNormalized * 75.0f;

    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));

    lightSpaceMatrix = lightProjection * lightView;
    return lightSpaceMatrix;
}

void ShadowMap::Delete() {
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (depthMap) {
        glDeleteTextures(1, &depthMap);
        depthMap = 0;
    }
}
