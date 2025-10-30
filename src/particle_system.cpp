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
                                   { return p.age >= p.lifetime; }),
                    particles.end());
}

void ParticleSystem::Draw() const
{
    BeginBlendMode(blendMode);
    for (const auto &p : particles)
    {
        Color c = p.color;
        c.a = (unsigned char)(p.color.a * (1.0f - p.age / p.lifetime));
        // Convert world to screen space for drawing
        Vector2 screenPos = GameManager::WorldToScreen(p.position);
        float zoom = GameManager::GetCamera().GetZoom();
        Vector2 size = Vector2(p.size * zoom, p.size * zoom);
        DrawRectangleV(screenPos - size * 0.5f, size, c);
    }
    EndBlendMode();
}

void ParticleSystem::Emit(const Particle &proto, int count)
{
    for (int i = 0; i < count; ++i)
    {
        Particle p = proto;
        particles.push_back(p);
    }
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