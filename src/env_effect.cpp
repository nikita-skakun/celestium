#include "asset_manager.hpp"
#include "crew.hpp"
#include "env_effect.hpp"
#include "game_state.hpp"
#include "station.hpp"

Effect::Effect(const std::string &defName, const Vector2Int &position, float s)
    : position(position)
{
    effectDef = DefinitionManager::GetEffectDefinition(defName);
    if (s == 0)
        s = 1. / effectDef->GetSizeIncrements();

    size = std::clamp(s, 0.f, 1.f);
}

void Effect::Render() const
{
    Vector2 screenSize = Vector2(1, 1) * TILE_SIZE * GameManager::GetCamera().GetZoom() * GetRoundedSize();
    Vector2 startPos = GameManager::WorldToScreen(GetPosition());
    int frame = GetEvenlySpacedIndex(GetTime() * effectDef->GetAnimationSpeed(), effectDef->GetSpriteCount());
    Rectangle sourceRect = Rectangle(frame, 0, 1, 1) * TILE_SIZE;

    DrawTexturePro(AssetManager::GetTexture(effectDef->GetSpriteSheet()), sourceRect, Vector2ToRect(startPos, screenSize), screenSize / 2., 0, WHITE);
}

std::string Effect::GetInfo() const
{
    std::string effectInfo = " - " + GetName();
    effectInfo += std::format("\n   + Size: {:.0f}", GetRoundedSize() * effectDef->GetSizeIncrements());

    return effectInfo;
}

void FireEffect::EffectCrew(const std::shared_ptr<Crew> &crew, float deltaTime) const
{
    crew->SetHealth(crew->GetHealth() - DAMAGE_PER_SECOND * deltaTime);
}

void FireEffect::Update(const std::shared_ptr<Station> &station, size_t index)
{
    auto tileWithOxygen = station->GetTileWithComponentAtPosition<OxygenComponent>(GetPosition());
    if (!tileWithOxygen || station->GetEffectOfTypeAtPosition<FoamEffect>(GetPosition()))
    {
        if (index < station->effects.size())
            station->effects.erase(std::begin(station->effects) + index);
        return;
    }

    for (auto &tile : station->GetTilesWithComponentAtPosition<DurabilityComponent>(GetPosition()))
    {
        auto durability = tile->GetComponent<DurabilityComponent>();
        durability->SetHitpoints(durability->GetHitpoints() - DAMAGE_PER_SECOND * FIXED_DELTA_TIME);
    }

    auto oxygen = tileWithOxygen->GetComponent<OxygenComponent>();
    if (oxygen->GetOxygenLevel() < GetOxygenConsumption() * FIXED_DELTA_TIME * 2.)
        SetSize(GetSize() * (2.f / 3.f));

    float oxygenToConsume = GetOxygenConsumption() * FIXED_DELTA_TIME;
    if (oxygen->GetOxygenLevel() < oxygenToConsume)
    {
        oxygen->SetOxygenLevel(0.f);
        if (index < station->effects.size())
            station->effects.erase(std::begin(station->effects) + index);
        return;
    }

    oxygen->SetOxygenLevel(oxygen->GetOxygenLevel() - oxygenToConsume);
    SetSize(GetSize() + (GROWTH_IF_FED_PER_SECOND * FIXED_DELTA_TIME));

    if (!station->GetTileWithComponentAtPosition<SolidComponent>(GetPosition()) && CheckIfEventHappens(SPREAD_CHANCE_PER_SECOND, FIXED_DELTA_TIME))
    {
        std::vector<Vector2Int> possibleOffsets;
        for (auto &dir : CARDINAL_DIRECTIONS)
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
