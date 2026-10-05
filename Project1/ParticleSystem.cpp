#include "ParticleSystem.h"
#include "Primitives.h"
#include "Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <algorithm>

static float RandomFloat(float minVal, float maxVal) {
    return minVal + (float(rand()) / float(RAND_MAX)) * (maxVal - minVal);
}

ParticleSystem::ParticleSystem() {}

ParticleSystem::~ParticleSystem() {
    Delete();
}

void ParticleSystem::Init() {
    if (!particleMesh) {
        // Round sphere mesh for realistic volumetric smoke puffs
        particleMesh = new Mesh(Primitives::CreateSphere(0.22f, 10, 10, glm::vec3(1.0f)));
    }
}

void ParticleSystem::EmitSmoke(glm::vec3 origin, glm::vec3 direction, int count) {
    glm::vec3 fwd = glm::length(direction) > 0.001f ? glm::normalize(direction) : glm::vec3(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < count; i++) {
        glm::vec3 spread(RandomFloat(-0.35f, 0.35f),
                         RandomFloat(-0.15f, 0.40f),
                         RandomFloat(-0.35f, 0.35f));
        glm::vec3 vel = (fwd + spread) * RandomFloat(2.0f, 5.0f);
        float life = RandomFloat(1.0f, 1.8f);
        float shade = RandomFloat(0.68f, 0.88f);
        particles.push_back({
            origin + spread * 0.25f,
            vel,
            glm::vec3(shade, shade, shade * 1.02f),
            RandomFloat(0.20f, 0.38f),
            life,
            life,
            ParticleType::Smoke
        });
    }
}

void ParticleSystem::EmitRedSmoke(glm::vec3 origin, int count) {
    for (int i = 0; i < count; i++) {
        glm::vec3 spread(RandomFloat(-0.35f, 0.35f),
                         RandomFloat(0.4f, 1.2f),
                         RandomFloat(-0.35f, 0.35f));
        glm::vec3 vel = spread * RandomFloat(3.0f, 6.0f);
        float life = RandomFloat(1.8f, 3.2f);
        float r = RandomFloat(0.85f, 1.0f);
        float g = RandomFloat(0.08f, 0.18f);
        float b = RandomFloat(0.06f, 0.15f);
        particles.push_back({
            origin + glm::vec3(RandomFloat(-0.2f, 0.2f), 0.0f, RandomFloat(-0.2f, 0.2f)),
            vel,
            glm::vec3(r, g, b),
            RandomFloat(0.25f, 0.45f),
            life,
            life,
            ParticleType::RedSmoke
        });
    }
}

void ParticleSystem::EmitSparks(glm::vec3 origin, glm::vec3 direction, int count) {
    glm::vec3 fwd = glm::length(direction) > 0.001f ? glm::normalize(direction) : glm::vec3(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < count; i++) {
        glm::vec3 spread(RandomFloat(-0.6f, 0.6f),
                         RandomFloat(-0.3f, 0.7f),
                         RandomFloat(-0.6f, 0.6f));
        glm::vec3 vel = (fwd + spread) * RandomFloat(6.0f, 14.0f);
        float life = RandomFloat(0.25f, 0.65f);
        particles.push_back({
            origin,
            vel,
            glm::vec3(1.0f, RandomFloat(0.60f, 0.85f), 0.15f),
            RandomFloat(0.06f, 0.12f),
            life,
            life,
            ParticleType::Spark
        });
    }
}

void ParticleSystem::EmitDebris(glm::vec3 origin, int count) {
    for (int i = 0; i < count; i++) {
        glm::vec3 vel(RandomFloat(-4.5f, 4.5f),
                      RandomFloat(2.0f, 7.5f),
                      RandomFloat(-4.5f, 4.5f));
        float life = RandomFloat(0.8f, 1.8f);
        float stoneShade = RandomFloat(0.40f, 0.65f);
        particles.push_back({
            origin,
            vel,
            glm::vec3(stoneShade, stoneShade * 0.96f, stoneShade * 0.90f),
            RandomFloat(0.10f, 0.22f),
            life,
            life,
            ParticleType::Debris
        });
    }
}

void ParticleSystem::Update(float dt) {
    for (size_t i = 0; i < particles.size();) {
        Particle& p = particles[i];
        p.life -= dt;
        if (p.life <= 0.0f) {
            particles[i] = particles.back();
            particles.pop_back();
            continue;
        }

        if (p.type == ParticleType::Smoke || p.type == ParticleType::RedSmoke) {
            p.velocity *= std::max(0.0f, 1.0f - 1.2f * dt); // air drag
            p.velocity.y += (p.type == ParticleType::RedSmoke ? 2.2f : 1.4f) * dt; // smoke buoyancy rises
            p.position += p.velocity * dt;
            p.size += 0.28f * dt;                            // gentle expansion
        } else if (p.type == ParticleType::Spark) {
            p.velocity.y -= 9.81f * dt;                      // gravity
            p.position += p.velocity * dt;
            if (p.position.y < 0.05f) {
                p.position.y = 0.05f;
                p.velocity.y = -p.velocity.y * 0.4f;         // bounce
                p.velocity.x *= 0.6f;
                p.velocity.z *= 0.6f;
            }
        } else if (p.type == ParticleType::Debris) {
            p.velocity.y -= 14.0f * dt;                      // heavy gravity
            p.position += p.velocity * dt;
            if (p.position.y < 0.08f) {
                p.position.y = 0.08f;
                p.velocity.y = -p.velocity.y * 0.3f;         // ground bounce
                p.velocity.x *= 0.5f;
                p.velocity.z *= 0.5f;
            }
        }
        ++i;
    }
}

void ParticleSystem::Draw(Shader& shader, const glm::mat4& /*view*/, const glm::mat4& /*proj*/) {
    if (particles.empty() || !particleMesh) return;

    glUseProgram(shader.ID);
    GLuint modelLoc = glGetUniformLocation(shader.ID, "model");
    GLint colorOverrideLoc = glGetUniformLocation(shader.ID, "colorOverride");
    GLint useColorOverrideLoc = glGetUniformLocation(shader.ID, "useColorOverride");

    if (useColorOverrideLoc >= 0) {
        glUniform1i(useColorOverrideLoc, 1);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const Particle& p : particles) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
        model = glm::scale(model, glm::vec3(p.size));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        if (colorOverrideLoc >= 0) {
            glUniform3fv(colorOverrideLoc, 1, glm::value_ptr(p.color));
        }
        particleMesh->Draw();
    }

    if (useColorOverrideLoc >= 0) {
        glUniform1i(useColorOverrideLoc, 0);
    }

    glDisable(GL_BLEND);
}

void ParticleSystem::Delete() {
    if (particleMesh) {
        particleMesh->Delete();
        delete particleMesh;
        particleMesh = nullptr;
    }
}
