#include "env_effect.hpp"
#include "lua_bindings.hpp"
#include "particle_system.hpp"

float LuaParticle::lifetime() const
{
    if (!system || index == SIZE_MAX)
        return 0.0f;
    auto p = system->GetParticlePtr(index);
    return p ? p->lifetime : 0.0f;
}

void LuaParticle::set_lifetime(float v)
{
    if (!system || index == SIZE_MAX)
        return;
    if (auto p = system->GetParticlePtr(index))
        p->lifetime = v;
}

float LuaParticle::size() const
{
    if (!system || index == SIZE_MAX)
        return 0.0f;
    auto p = system->GetParticlePtr(index);
    return p ? p->size : 0.0f;
}

void LuaParticle::set_size(float v)
{
    if (!system || index == SIZE_MAX)
        return;
    if (auto p = system->GetParticlePtr(index))
        p->size = v;
}

float LuaParticle::age() const
{
    if (!system || index == SIZE_MAX)
        return 0.0f;
    auto p = system->GetParticlePtr(index);
    return p ? p->age : 0.0f;
}

void LuaParticle::set_age(float v)
{
    if (!system || index == SIZE_MAX)
        return;
    if (auto p = system->GetParticlePtr(index))
        p->age = v;
}

sol::table LuaParticle::velocity(sol::this_state s)
{
    sol::state_view lua(s);
    sol::table t = lua.create_table();
    if (system && index != SIZE_MAX)
    {
        if (auto particle = system->GetParticlePtr(index))
        {
            t["x"] = particle->velocity.x;
            t["y"] = particle->velocity.y;
        }
        else
        {
            t["x"] = 0.0f;
            t["y"] = 0.0f;
        }
    }
    else
    {
        t["x"] = 0.0f;
        t["y"] = 0.0f;
    }
    return t;
}

void LuaParticle::set_velocity(sol::table t)
{
    if (!system || index == SIZE_MAX)
        return;
    if (auto particle = system->GetParticlePtr(index))
    {
        sol::optional<float> ox = t["x"];
        sol::optional<float> oy = t["y"];
        if (ox)
            particle->velocity.x = *ox;
        if (oy)
            particle->velocity.y = *oy;
    }
}

sol::table LuaParticle::position(sol::this_state s)
{
    sol::state_view lua(s);
    sol::table t = lua.create_table();
    if (system && index != SIZE_MAX)
    {
        if (auto particle = system->GetParticlePtr(index))
        {
            t["x"] = particle->position.x;
            t["y"] = particle->position.y;
        }
        else
        {
            t["x"] = 0.0f;
            t["y"] = 0.0f;
        }
    }
    else
    {
        t["x"] = 0.0f;
        t["y"] = 0.0f;
    }
    return t;
}

void LuaParticle::set_position(sol::table t)
{
    if (!system || index == SIZE_MAX)
        return;
    if (auto particle = system->GetParticlePtr(index))
    {
        sol::optional<float> ox = t["x"];
        sol::optional<float> oy = t["y"];
        if (ox)
            particle->position.x = *ox;
        if (oy)
            particle->position.y = *oy;
    }
}

sol::table LuaParticle::color(sol::this_state s)
{
    sol::state_view lua(s);
    sol::table t = lua.create_table();
    if (system && index != SIZE_MAX)
    {
        if (auto particle = system->GetParticlePtr(index))
        {
            t["r"] = particle->color.r;
            t["g"] = particle->color.g;
            t["b"] = particle->color.b;
            t["a"] = particle->color.a;
        }
        else
        {
            t["r"] = 255;
            t["g"] = 255;
            t["b"] = 255;
            t["a"] = 255;
        }
    }
    else
    {
        t["r"] = 255;
        t["g"] = 255;
        t["b"] = 255;
        t["a"] = 255;
    }
    return t;
}

void LuaParticle::set_color(sol::table t)
{
    if (!system || index == SIZE_MAX)
        return;
    if (auto particle = system->GetParticlePtr(index))
    {
        sol::optional<int> or_ = t["r"];
        sol::optional<int> og = t["g"];
        sol::optional<int> ob = t["b"];
        sol::optional<int> oa = t["a"];
        if (or_)
            particle->color.r = static_cast<unsigned char>(std::clamp(*or_, 0, 255));
        if (og)
            particle->color.g = static_cast<unsigned char>(std::clamp(*og, 0, 255));
        if (ob)
            particle->color.b = static_cast<unsigned char>(std::clamp(*ob, 0, 255));
        if (oa)
            particle->color.a = static_cast<unsigned char>(std::clamp(*oa, 0, 255));
    }
}

void LuaParticleSystem::set_blend_mode(const std::string &mode)
{
    if (system)
        system->SetBlendMode(magic_enum::enum_cast<BlendMode>(mode, magic_enum::case_insensitive).value_or(BLEND_ALPHA));
}

LuaParticle LuaParticleSystem::emit()
{
    if (!system)
        return LuaParticle(nullptr);
    size_t idx = system->Emit();
    if (idx == SIZE_MAX)
        return LuaParticle(nullptr);
    return LuaParticle(system, idx);
}

sol::table LuaParticleSystem::get_particles(sol::this_state s)
{
    sol::state_view lua(s);
    sol::table t = lua.create_table();
    if (!system)
        return t;

    size_t count = system->GetParticleCount();
    for (size_t i = 0; i < count; ++i)
    {
        t[i + 1] = LuaParticle(system, i);
    }

    return t;
}

void register_particle_lua(sol::state &lua)
{
    lua.new_usertype<LuaParticle>("particle",
                                  sol::constructors<LuaParticle(ParticleSystem *, size_t)>(),
                                  "lifetime", sol::property(&LuaParticle::lifetime, &LuaParticle::set_lifetime),
                                  "size", sol::property(&LuaParticle::size, &LuaParticle::set_size),
                                  "age", sol::property(&LuaParticle::age, &LuaParticle::set_age),
                                  "position", sol::property(&LuaParticle::position, &LuaParticle::set_position),
                                  "velocity", sol::property(&LuaParticle::velocity, &LuaParticle::set_velocity),
                                  "color", sol::property(&LuaParticle::color, &LuaParticle::set_color));

    lua.new_usertype<LuaParticleSystem>("system",
                                        sol::constructors<LuaParticleSystem(ParticleSystem *)>(),
                                        "set_blend_mode", &LuaParticleSystem::set_blend_mode,
                                        "emit", &LuaParticleSystem::emit,
                                        "get_particles", &LuaParticleSystem::get_particles);

    lua.new_usertype<LuaEffect>("effect",
                                "size", sol::property(&LuaEffect::size),
                                "position", sol::property(&LuaEffect::position));
}

float LuaEffect::size() const
{
    if (!effect)
        return 0.0f;
    return effect->GetSize();
}

sol::table LuaEffect::position(sol::this_state s) const
{
    sol::state_view lua(s);
    sol::table t = lua.create_table();
    if (effect)
    {
        t["x"] = effect->GetPosition().x;
        t["y"] = effect->GetPosition().y;
    }
    else
    {
        t["x"] = 0.0f;
        t["y"] = 0.0f;
    }
    return t;
}

void RegisterAllLuaBindings(sol::state &lua)
{
    register_particle_lua(lua);
    lua["math"]["clamp"] = [](double x, double a, double b) -> double
    {
        if (a > b)
            std::swap(a, b);
        return std::clamp(x, a, b);
    };
}
