#include "asset_manager.hpp"
#include "crew.hpp"
#include "env_effect.hpp"
#include "game_state.hpp"
#include "lua_bindings.hpp"
#include "station.hpp"

Effect::Effect(const std::string &defName, const Vector2Int &position, float s)
    : position(position)
{
    effectDef = DefinitionManager::GetEffectDefinition(defName);
    if (!effectDef)
        throw std::runtime_error("Effect definition not found: " + defName);

    if (s == 0)
        s = 1.f / effectDef->GetSizeIncrements();
    size = std::clamp(s, 0.f, 1.f);

    for (const auto &psDef : effectDef->GetParticleSystems())
    {
        ParticleSystemWithLua ps;
        ps.onCreateLua = psDef.onCreateLua;
        ps.onUpdateLua = psDef.onUpdateLua;
        particleSystems.push_back(std::move(ps));
    }
}

void Effect::Render() const
{
    for (const auto &ps : particleSystems)
        ps.system.Draw();
}

std::string Effect::GetInfo() const
{
    std::string effectInfo = " - " + GetName();
    effectInfo += "\n   + Size: " + std::to_string((int)(GetRoundedSize() * effectDef->GetSizeIncrements()));
    return effectInfo;
}

void FireEffect::EffectCrew(const std::shared_ptr<Crew> &crew, float deltaTime) const
{
    crew->SetHealth(crew->GetHealth() - DAMAGE_PER_SECOND * deltaTime);
}

void FireEffect::Update(const std::shared_ptr<Station> &station, size_t index)
{
    sol::state &lua = GameManager::GetLua();
    for (auto &ps : particleSystems)
    {
        // Execute Lua on_create (system) if not yet run
        if (!ps.systemCreated)
        {
            if (!ps.onCreateLua.empty())
            {
                std::string wrapped = "return function(system, effect, station)\n" + ps.onCreateLua + "\nend";
                auto chunk = lua.load(wrapped);
                if (!chunk.valid())
                {
                    sol::error err = chunk;
                    throw std::runtime_error("FireEffect: Failed to load on_create Lua: " + std::string(err.what()));
                }
                else
                {
                    try
                    {
                        sol::protected_function func = chunk();
                        sol::state_view lua_view(lua);
                        sol::table effect_tbl = lua_view.create_table_with(
                            "size", size,
                            "position", lua_view.create_table_with("x", position.x, "y", position.y));
                        func(LuaParticleSystem(&ps.system), effect_tbl, station);
                    }
                    catch (const sol::error &e)
                    {
                        throw std::runtime_error("FireEffect: Error running on_create Lua: " + std::string(e.what()));
                    }
                }
            }
            ps.systemCreated = true;
        }
        if (!ps.onUpdateLua.empty())
        {
            std::string wrapped = "return function(system, effect, station, dt)\n" + ps.onUpdateLua + "\nend";
            auto chunk = lua.load(wrapped);
            if (!chunk.valid())
            {
                sol::error err = chunk;
                throw std::runtime_error("FireEffect: Failed to load on_update Lua: " + std::string(err.what()));
            }
            else
            {
                try
                {
                    sol::protected_function func = chunk();
                    sol::state_view lua_view(lua);
                    sol::table effect_tbl = lua_view.create_table_with(
                        "size", size,
                        "position", lua_view.create_table_with("x", position.x, "y", position.y));
                    func(LuaParticleSystem(&ps.system), effect_tbl, station, FIXED_DELTA_TIME);
                }
                catch (const sol::error &e)
                {
                    throw std::runtime_error("FireEffect: Error running on_update Lua: " + std::string(e.what()));
                }
            }
        }

        // Ensure the particle system advances each fixed update.
        ps.system.Update(FIXED_DELTA_TIME);
    }

    // --- Fire effect logic (tile damage, oxygen, spreading) ---
    auto tileWithOxygen = station->GetTileWithComponentAtPosition<OxygenComponent>(GetPosition());
    if (!tileWithOxygen || station->GetEffectOfTypeAtPosition<FoamEffect>(GetPosition()))
    {
        if (index < station->effects.size())
            station->effects.erase(station->effects.begin() + index);
        return;
    }

    // Damage durability of all tiles at this position
    for (auto &tile : station->GetTilesWithComponentAtPosition<DurabilityComponent>(GetPosition()))
    {
        auto durability = tile->GetComponent<DurabilityComponent>();
        durability->SetHitpoints(durability->GetHitpoints() - DAMAGE_PER_SECOND * FIXED_DELTA_TIME);
    }

    // Oxygen consumption and fire size
    auto oxygen = tileWithOxygen->GetComponent<OxygenComponent>();
    float oxygenToConsume = GetOxygenConsumption() * FIXED_DELTA_TIME;
    if (oxygen->GetOxygenLevel() < oxygenToConsume * 2.f)
        SetSize(GetSize() * (2.f / 3.f));
    if (oxygen->GetOxygenLevel() < oxygenToConsume)
    {
        oxygen->SetOxygenLevel(0.f);
        if (index < station->effects.size())
            station->effects.erase(station->effects.begin() + index);
        return;
    }
    oxygen->SetOxygenLevel(oxygen->GetOxygenLevel() - oxygenToConsume);
    SetSize(GetSize() + (GROWTH_IF_FED_PER_SECOND * FIXED_DELTA_TIME));

    if (!station->GetTileWithComponentAtPosition<SolidComponent>(GetPosition()) && CheckIfEventHappens(SPREAD_CHANCE_PER_SECOND, FIXED_DELTA_TIME))
    {
        std::vector<Vector2Int> possibleOffsets;
        for (const auto &dir : CARDINAL_DIRECTIONS)
        {
            auto neighborPos = GetPosition() + DirectionToVector2Int(dir);
            bool neighborHasOxygen = station->GetTileWithComponentAtPosition<OxygenComponent>(neighborPos) != nullptr;
            bool neighborHasFire = station->GetEffectOfTypeAtPosition<FireEffect>(neighborPos) != nullptr;
            bool neighborHasFoam = station->GetEffectOfTypeAtPosition<FoamEffect>(neighborPos) != nullptr;
            if (neighborHasOxygen && !neighborHasFire && !neighborHasFoam)
                possibleOffsets.push_back(DirectionToVector2Int(dir));
        }
        if (!possibleOffsets.empty())
        {
            int selected = RandomIntWithRange(0, static_cast<int>(possibleOffsets.size()) - 1);
            station->effects.push_back(std::make_shared<FireEffect>(GetPosition() + possibleOffsets[selected]));
        }
    }
}

void FoamEffect::Update(const std::shared_ptr<Station> &station, size_t)
{
    sol::state &lua = GameManager::GetLua();
    for (auto &ps : particleSystems)
    {
        // Execute Lua on_create (system) if not yet run
        if (!ps.systemCreated)
        {
            if (!ps.onCreateLua.empty())
            {
                // Wrap user snippet in a function so it can receive parameters: (system, effect, station)
                std::string wrapped = std::string("return function(system, effect, station)\n") + ps.onCreateLua + "\nend";
                auto chunk = lua.load(wrapped);
                if (!chunk.valid())
                {
                    sol::error err = chunk;
                    throw std::runtime_error(std::string("FoamEffect: Failed to load on_create Lua: ") + std::string(err.what()));
                }
                else
                {
                    try
                    {
                        sol::protected_function func = chunk();
                        // Build a simple Lua table for the effect so scripts can read fields like `size` and `position.x`
                        sol::state_view lua_view(lua);
                        sol::table effect_tbl = lua_view.create_table_with(
                            "size", size,
                            "position", lua_view.create_table_with("x", position.x, "y", position.y));
                        func(LuaParticleSystem(&ps.system), effect_tbl, station);
                    }
                    catch (const sol::error &e)
                    {
                        throw std::runtime_error(std::string("FoamEffect: Error running on_create Lua: ") + std::string(e.what()));
                    }
                }
            }
            ps.systemCreated = true;
        }
        if (!ps.onUpdateLua.empty())
        {
            // Wrap user snippet in a function so it can receive parameters: (system, effect, station, dt)
            std::string wrapped = std::string("return function(system, effect, station, dt)\n") + ps.onUpdateLua + "\nend";
            auto chunk = lua.load(wrapped);
            if (!chunk.valid())
            {
                sol::error err = chunk;
                throw std::runtime_error(std::string("FoamEffect: Failed to load on_update Lua: ") + std::string(err.what()));
            }
            else
            {
                try
                {
                    sol::protected_function func = chunk();
                    sol::state_view lua_view(lua);
                    sol::table effect_tbl = lua_view.create_table_with(
                        "size", size,
                        "position", lua_view.create_table_with("x", position.x, "y", position.y));
                    func(LuaParticleSystem(&ps.system), effect_tbl, station, FIXED_DELTA_TIME);
                }
                catch (const sol::error &e)
                {
                    throw std::runtime_error(std::string("FoamEffect: Error running on_update Lua: ") + std::string(e.what()));
                }
            }
        }

        ps.system.Update(FIXED_DELTA_TIME);
    }
}