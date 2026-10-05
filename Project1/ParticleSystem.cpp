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
        particleMesh = new Mesh(Primitives::CreateBox(1.0f, 1.0f, 1.0f, glm::vec3(1.0f)));
    }
}

void ParticleSystem::EmitSmoke(glm::vec3 origin, glm::vec3 direction, int count) {
    glm::vec3 fwd = glm::normalize(direction);
    for (int i = 0; i < count; i++) {
        glm::vec3 spread(RandomFloat(-0.4f, 0.4f),
                         RandomFloat(-0.2f, 0.5f),
                         RandomFloat(-0.4f, 0.4f));
        glm::vec3 vel = (fwd + spread) * RandomFloat(2.5f, 6.5f);
        float life = RandomFloat(1.2f, 2.4f);
        float shade = RandomFloat(0.65f, 0.90f);
        particles.push_back({
            origin + spread * 0.3f,
            vel,
            glm::vec3(shade, shade, shade * 1.02f),
            RandomFloat(0.35f, 0.70f),
            life,
            life,
            ParticleType::Smoke
        });
    }
}

void ParticleSystem::EmitSparks(glm::vec3 origin, glm::vec3 direction, int count) {
    glm::vec3 fwd = glm::normalize(direction);
    for (int i = 0; i < count; i++) {
        glm::vec3 spread(RandomFloat(-0.6f, 0.6f),
                         RandomFloat(-0.3f, 0.7f),
                         RandomFloat(-0.6f, 0.6f));
        glm::vec3 vel = (fwd + spread) * RandomFloat(7.0f, 16.0f);
        float life = RandomFloat(0.3f, 0.8f);
        particles.push_back({
            origin,
            vel,
            glm::vec3(1.0f, RandomFloat(0.60f, 0.85f), 0.15f),
            RandomFloat(0.08f, 0.16f),
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
        float life = RandomFloat(1.0f, 2.0f);
        float stoneShade = RandomFloat(0.40f, 0.65f);
        particles.push_back({
            origin,
            vel,
            glm::vec3(stoneShade, stoneShade * 0.96f, stoneShade * 0.90f),
            RandomFloat(0.12f, 0.32f),
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

        if (p.type == ParticleType::Smoke) {
            p.velocity *= std::max(0.0f, 1.0f - 1.2f * dt); // air drag
            p.velocity.y += 1.5f * dt;                       // smoke buoyancy rises
            p.position += p.velocity * dt;
            p.size += 0.75f * dt;                            // smoke expands as it rises
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const Particle& p : particles) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
        model = glm::scale(model, glm::vec3(p.size));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        particleMesh->Draw();
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
