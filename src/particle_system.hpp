#pragma once
#include "utils.hpp"

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float size;
    float lifetime;
    float age = 0.0f;
};

class ParticleSystem {
public:
    ParticleSystem();
    void Update(float dt);
    void Draw() const;
    void Emit(const Particle& proto, int count);
    void SetBlendMode(int mode); // e.g. BLEND_ALPHA, BLEND_ADDITIVE
    void Clear();
private:
    std::vector<Particle> particles;
    int blendMode = BLEND_ALPHA;
};
