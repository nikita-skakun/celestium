#pragma once
#include <sol/sol.hpp>

void RegisterAllLuaBindings(sol::state &lua);

struct ParticleSystem;
struct Effect;

struct LuaParticle
{
    std::shared_ptr<ParticleSystem> particleSystem;
    size_t index;
    LuaParticle(std::shared_ptr<ParticleSystem> s = nullptr, size_t i = SIZE_MAX) : particleSystem(s), index(i) {}

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
    std::shared_ptr<ParticleSystem> particleSystem;
    LuaParticleSystem(std::shared_ptr<ParticleSystem> sys) : particleSystem(sys) {}

    void set_blend_mode(const std::string &mode);
    LuaParticle emit();
    sol::table get_particles(sol::this_state s);
};

struct LuaEffect
{
    std::weak_ptr<const Effect> effectRef;
    LuaEffect() = default;
    LuaEffect(const std::shared_ptr<Effect> &e) : effectRef(e) {}

    float size() const;
    sol::table position(sol::this_state s) const;
};

void register_particle_lua(sol::state &lua);
