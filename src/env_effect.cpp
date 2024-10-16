#include "asset_manager.hpp"
#include "camera.hpp"
#include "crew.hpp"
#include "env_effect.hpp"

void FireEffect::EffectCrew(Crew &crew, float deltaTime) const
{
    crew.SetHealth(crew.GetHealth() - DAMAGE_PER_SECOND * deltaTime);
}

void FireEffect::Render(const PlayerCam &camera) const
{
    Vector2 fireSize = Vector2(1, 1) * TILE_SIZE * camera.GetZoom() * GetRoundedSize();
    Vector2 startPos = camera.WorldToScreen(ToVector2(GetPosition()) + Vector2(1, 1) * ((1. - GetRoundedSize()) / 2.));
    Rectangle sourceRect = Rectangle(GetEvenlySpacedIndex(GetTime(), 8), 0, 1, 1) * TILE_SIZE;

    DrawTexturePro(AssetManager::GetTexture("FIRE"), sourceRect, Vector2ToRect(startPos, fireSize), Vector2(), 0, WHITE);
}

void FireEffect::Update(const std::shared_ptr<Station> &station, int index)
{
    auto tileWithOxygen = station->GetTileWithComponentAtPosition<OxygenComponent>(GetPosition());
    bool isFoamOnTile = station->GetTypeEffectAtPosition<FoamEffect>(GetPosition()) != nullptr;
    if (!tileWithOxygen || isFoamOnTile)
    {
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
        station->effects.erase(station->effects.begin() + index);
        return;
    }

    oxygen->SetOxygenLevel(oxygen->GetOxygenLevel() - oxygenToConsume);
    SetSize(GetSize() + FireEffect::GROWTH_IF_FED_PER_SECOND * FIXED_DELTA_TIME);

    bool tileIsSolid = station->GetTileWithComponentAtPosition<SolidComponent>(GetPosition()) != nullptr;
    if (!tileIsSolid && CheckIfEventHappens(SPREAD_CHANCE_PER_SECOND, FIXED_DELTA_TIME))
    {
        std::vector<Direction> neighborDirections = {Direction::N, Direction::E, Direction::S, Direction::W};
        std::vector<Vector2Int> possibleOffsets;
        for (const auto &direction : neighborDirections)
        {
            Vector2Int offset = DirectionToVector2Int(direction);
            Vector2Int neighborPos = GetPosition() + offset;

            bool neighborHasOxygen = station->GetTileWithComponentAtPosition<OxygenComponent>(neighborPos) != nullptr;
            bool neighborHasFire = station->GetTypeEffectAtPosition<FireEffect>(neighborPos) != nullptr;
            bool neighborHasFoam = station->GetTypeEffectAtPosition<FoamEffect>(neighborPos) != nullptr;
            if (neighborHasOxygen && !neighborHasFire && !neighborHasFoam)
                possibleOffsets.push_back(offset);
        }

        if (possibleOffsets.empty())
            return;

        int directionSelected = RandomIntWithRange(0, (int)possibleOffsets.size() - 1);
        Vector2Int newFirePos = GetPosition() + possibleOffsets[directionSelected];
        station->effects.push_back(std::make_shared<FireEffect>(newFirePos, FireEffect::SIZE_INCREMENT));
    }
}

void FoamEffect::Render(const PlayerCam &camera) const
{
    Vector2 foamSize = Vector2(1, 1) * TILE_SIZE * camera.GetZoom();
    Vector2 startPos = camera.WorldToScreen(GetPosition());
    Rectangle sourceRect = Rectangle(0, 0, 1, 1) * TILE_SIZE;

    DrawTexturePro(AssetManager::GetTexture("FOAM"), sourceRect, Vector2ToRect(startPos, foamSize), Vector2(), 0, WHITE);
}
