#include "game_state.hpp"
#include "particle_system.hpp"
#include "raylib.h"

ParticleSystem::ParticleSystem() {}

void ParticleSystem::Update(float dt)
{
    for (auto &p : particles)
    {
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.age += dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle &p)
                                   { return (p.lifetime >= 0.0f) && (p.age > p.lifetime); }),
                    particles.end());
}

void ParticleSystem::Draw() const
{
    BeginBlendMode(blendMode);
    for (const auto &p : particles)
    {
        Color c = p.color;
        // Compute life ratio for fading. If lifetime <= 0 we treat it as infinite (no fade).
        float lifeRatio = (p.lifetime > 0.0f) ? (p.age / p.lifetime) : 0.0f;
        lifeRatio = std::clamp(lifeRatio, 0.0f, 1.0f);
        c.a = static_cast<unsigned char>(p.color.a * (1.0f - lifeRatio));
        // Convert world to screen space for drawing
        Vector2 screenPos = GameManager::WorldToScreen(p.position);
        float zoom = GameManager::GetCamera().GetZoom();
        Vector2 size = Vector2(p.size * zoom, p.size * zoom);
        DrawRectangleV(screenPos - size * 0.5f, size, c);
    }
    EndBlendMode();
}

size_t ParticleSystem::Emit(const Particle &proto)
{
    Particle p = proto;
    particles.push_back(p);
    return particles.size() - 1;
}

Particle *ParticleSystem::GetParticlePtr(size_t idx)
{
    if (idx < particles.size())
        return &particles[idx];
    return nullptr;
}

const Particle *ParticleSystem::GetParticlePtr(size_t idx) const
{
    if (idx < particles.size())
        return &particles[idx];
    return nullptr;
}

void ParticleSystem::SetBlendMode(int mode)
{
    blendMode = mode;
}

void ParticleSystem::Clear()
{
    particles.clear();
}

bool ParticleSystem::IsEmpty() const
{
    return particles.empty();
}

size_t ParticleSystem::GetParticleCount() const
{
    return particles.size();
}