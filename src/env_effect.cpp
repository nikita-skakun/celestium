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

void FireEffect::Update(const std::shared_ptr<Station> &station, int index)
{
    auto tileWithOxygen = station->GetTileWithComponentAtPosition<OxygenComponent>(GetPosition());
    bool foamPresentOnTile = station->GetEffectOfTypeAtPosition<FoamEffect>(GetPosition()) != nullptr;
    if (!tileWithOxygen || foamPresentOnTile)
    {
        if (index >= 0 && index < station->effects.size())
            station->effects.erase(station->effects.begin() + index);
        return;
    }

    auto tilesWithDurability = station->GetTilesWithComponentAtPosition<DurabilityComponent>(GetPosition());
    if (!tilesWithDurability.empty())
    {
        for (auto &tileWithDurability : tilesWithDurability)
        {
            auto durability = tileWithDurability->GetComponent<DurabilityComponent>();
            durability->SetHitpoints(durability->GetHitpoints() - FireEffect::DAMAGE_PER_SECOND * FIXED_DELTA_TIME);
        }
    }

    auto oxygen = tileWithOxygen->GetComponent<OxygenComponent>();
    if (oxygen->GetOxygenLevel() < GetOxygenConsumption() * FIXED_DELTA_TIME * 2.)
        SetSize(GetSize() / 3. * 2.);

    float oxygenToConsume = GetOxygenConsumption() * FIXED_DELTA_TIME;
    if (oxygen->GetOxygenLevel() < oxygenToConsume)
    {
        oxygen->SetOxygenLevel(0);
        if (index >= 0 && index < station->effects.size())
            station->effects.erase(station->effects.begin() + index);
        return;
    }

    oxygen->SetOxygenLevel(oxygen->GetOxygenLevel() - oxygenToConsume);
    SetSize(GetSize() + FireEffect::GROWTH_IF_FED_PER_SECOND * FIXED_DELTA_TIME);

    bool tileIsSolid = station->GetTileWithComponentAtPosition<SolidComponent>(GetPosition()) != nullptr;
    if (!tileIsSolid && CheckIfEventHappens(SPREAD_CHANCE_PER_SECOND, FIXED_DELTA_TIME))
    {
        std::vector<Vector2Int> possibleOffsets;
        for (const auto &direction : CARDINAL_DIRECTIONS)
        {
            Vector2Int offset = DirectionToVector2Int(direction);
            Vector2Int neighborPos = GetPosition() + offset;

            bool neighborHasOxygen = station->GetTileWithComponentAtPosition<OxygenComponent>(neighborPos) != nullptr;
            bool neighborHasFire = station->GetEffectOfTypeAtPosition<FireEffect>(neighborPos) != nullptr;
            bool neighborHasFoam = station->GetEffectOfTypeAtPosition<FoamEffect>(neighborPos) != nullptr;
            if (neighborHasOxygen && !neighborHasFire && !neighborHasFoam)
                possibleOffsets.push_back(offset);
        }

        if (possibleOffsets.empty())
            return;

        int directionSelected = RandomIntWithRange(0, (int)possibleOffsets.size() - 1);
        Vector2Int newFirePos = GetPosition() + possibleOffsets[directionSelected];
        station->effects.push_back(std::make_shared<FireEffect>(newFirePos));
    }
}
