#pragma once
#include "utils.hpp"

struct Particle
{
    Vector2 position;
    Vector2 velocity;
    Color color;
    float size;
    float lifetime;
    float age;

    Particle()
        : position(), velocity(), color(Color(255, 255, 255, 255)), size(1.f), lifetime(1.f), age(0.f) {}

    Particle(Vector2 pos, Vector2 vel, Color col, float size, float life, float age = 0.f)
        : position(pos), velocity(vel), color(col), size(size), lifetime(life), age(age) {}
};

struct ParticleSystem
{
public:
    ParticleSystem();
    void Update(float dt);
    void Draw() const;
    size_t Emit(const Particle &proto = Particle());
    void SetBlendMode(int mode);
    void Clear();
    bool IsEmpty() const;
    // Safe accessors by index. Returns nullptr if out of range.
    Particle *GetParticlePtr(size_t idx);
    const Particle *GetParticlePtr(size_t idx) const;
    size_t GetParticleCount() const;

private:
    std::vector<Particle> particles;
    int blendMode = BLEND_ALPHA;
};
