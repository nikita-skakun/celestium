#include "component.hpp"
#include "crew.hpp"
#include "env_effect.hpp"
#include "lua_bindings.hpp"
#include "station.hpp"
#include "tile.hpp"

// static instance id counter
std::atomic<uint64_t> Effect::nextInstanceId{1};

Effect::Effect(const std::string &defName, const Vector2Int &position, float s)
    : position(position)
{
    effectDef = DefinitionManager::GetEffectDefinition(defName);
    if (!effectDef)
        throw std::runtime_error("Effect definition not found: " + defName);

    if (s == 0)
        s = 1.f / effectDef->GetSizeIncrements();
    size = std::clamp(s, 0.f, 1.f);

    instanceId = nextInstanceId.fetch_add(1);
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
    // --- Fire effect logic (tile damage, oxygen, spreading) ---
    auto tileWithOxygen = station->GetTileWithComponentAtPosition(GetPosition(), ComponentType::OXYGEN);
    if (!tileWithOxygen || station->GetEffectOfTypeAtPosition(GetPosition(), "FOAM"))
    {
        if (index < station->effects.size())
            station->effects.erase(station->effects.begin() + index);
        return;
    }

    // Damage durability of all tiles at this position
    for (auto &tile : station->GetTilesWithComponentAtPosition(GetPosition(), ComponentType::DURABILITY))
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

    if (!station->GetTileWithComponentAtPosition(GetPosition(), ComponentType::SOLID) && CheckIfEventHappens(SPREAD_CHANCE_PER_SECOND, FIXED_DELTA_TIME))
    {
        std::vector<Vector2Int> possibleOffsets;
        for (const auto &dir : CARDINAL_DIRECTIONS)
        {
            auto neighborPos = GetPosition() + DirectionToVector2Int(dir);
            bool neighborHasOxygen = station->GetTileWithComponentAtPosition(neighborPos, ComponentType::OXYGEN) != nullptr;
            bool neighborHasFire = station->GetEffectOfTypeAtPosition(neighborPos, "FIRE") != nullptr;
            bool neighborHasFoam = station->GetEffectOfTypeAtPosition(neighborPos, "FOAM") != nullptr;
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
    // If there is no floor component, remove the foam effect
    auto walkableTile = station->GetTileWithComponentAtPosition(GetPosition(), ComponentType::WALKABLE);
    if (!walkableTile)
    {
        auto &effects = station->effects;
        effects.erase(std::remove_if(effects.begin(), effects.end(), [&](const std::shared_ptr<Effect> &effect)
                                     { return effect.get() == this; }),
                      effects.end());
    }
}
