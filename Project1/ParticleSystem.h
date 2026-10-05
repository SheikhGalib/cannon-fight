#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "shaderClass.h"

enum class ParticleType {
    Smoke,
    RedSmoke,
    Spark,
    Debris
};

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float size;
    float life;
    float maxLife;
    ParticleType type;
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    void Init();
    void EmitSmoke(glm::vec3 origin, glm::vec3 direction, int count = 20);
    void EmitRedSmoke(glm::vec3 origin, int count = 25);
    void EmitSparks(glm::vec3 origin, glm::vec3 direction, int count = 30);
    void EmitDebris(glm::vec3 origin, int count = 35);

    void Update(float dt);
    void Draw(Shader& shader, const glm::mat4& view, const glm::mat4& proj);
    void Delete();

private:
    std::vector<Particle> particles;
    Mesh* particleMesh = nullptr;
};

#endif
