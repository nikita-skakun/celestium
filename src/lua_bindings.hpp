#pragma once
#include <sol/sol.hpp>

void RegisterAllLuaBindings(sol::state &lua);

struct ParticleSystem;

struct LuaParticle
{
    ParticleSystem *system;
    size_t index;
    LuaParticle(ParticleSystem *s = nullptr, size_t i = SIZE_MAX) : system(s), index(i) {}

    float lifetime() const;
    void set_lifetime(float v);

    float size() const;
    void set_size(float v);

    float age() const;
    void set_age(float v);

    sol::table velocity(sol::this_state s);
    void set_velocity(sol::table t);

    sol::table position(sol::this_state s);
    void set_position(sol::table t);

    sol::table color(sol::this_state s);
    void set_color(sol::table t);
};

struct LuaParticleSystem
{
    ParticleSystem *system;
    LuaParticleSystem(ParticleSystem *sys) : system(sys) {}

    void set_blend_mode(const std::string &mode);
    LuaParticle emit();
    sol::table get_particles(sol::this_state s);
};

struct LuaEffect
{
    const struct Effect *effect;
    LuaEffect(const struct Effect *e = nullptr) : effect(e) {}

    float size() const;
    sol::table position(sol::this_state s) const;
};

void register_particle_lua(sol::state &lua);
