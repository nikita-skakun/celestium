#include "component.hpp"
#include "station.hpp"
#include "tile.hpp"

void OxygenProducerComponent::ProduceOxygen(float deltaTime) const
{
    auto parent = GetParent();
    if (!parent || !parent->GetStation() || !parent->IsActive())
        return;

    auto oxygenTile = parent->GetStation()->GetTileWithComponentAtPosition(parent->GetPosition(), ComponentType::OXYGEN);
    if (!oxygenTile)
        return;

    auto oxygen = oxygenTile->GetComponent<OxygenComponent>();
    oxygen->SetOxygenLevel(std::min(oxygen->GetOxygenLevel() + oxygenProduction * deltaTime, TILE_OXYGEN_MAX));
}

void DoorComponent::Animate(float deltaTime)
{
    if (forcedOpenTimer > 0.f)
        forcedOpenTimer -= deltaTime;

    auto parent = GetParent();
    if (!parent || !parent->IsActive())
        return;

    float targetProgress = (forcedOpenTimer > 0.f) ? 0.f : 1.f;
    
    // Handle SolidComponent toggle based on progress
    if (progress <= 0.f && targetProgress > 0.f) {
        // Starting to close
    } else if (progress >= 1.f && targetProgress < 1.f) {
        // Starting to open
        parent->RemoveComponent<SolidComponent>();
    }

    if (progress == targetProgress) {
        if (progress >= 1.f && !parent->HasComponent(ComponentType::SOLID))
            parent->AddComponent<SolidComponent>();
        return;
    }

    float direction = (targetProgress > progress) ? 1.f : -1.f;
    float nextProgress = progress + direction * movingSpeed * deltaTime;
    
    if (direction > 0 && nextProgress >= 1.f) {
        nextProgress = 1.f;
        parent->AddComponent<SolidComponent>();
    } else if (direction < 0 && nextProgress <= 0.f) {
        nextProgress = 0.f;
    }

    SetProgress(nextProgress);
}

void DurabilityComponent::SetHitpoints(float newHitpoints)
{
    auto parent = GetParent();
    hitpoints = std::max(newHitpoints, 0.f);
    if (hitpoints > 0.f || !parent)
        return;

    parent->DeleteTile(true);
}

void OxygenComponent::Diffuse(float deltaTime)
{
    auto parent = GetParent();
    if (!parent || !parent->GetStation())
        return;

    auto station = parent->GetStation();

    for (auto direction : CARDINAL_DIRECTIONS)
    {
        Vector2Int neighborPos = parent->GetPosition() + DirectionToVector2Int(direction);

        if (!station->GetTileAtPosition(neighborPos))
        {
            SetOxygenLevel(GetOxygenLevel() - GetOxygenLevel() * OXYGEN_DIFFUSION_RATE * deltaTime);
            continue;
        }

        if (station->GetTileWithComponentAtPosition(neighborPos, ComponentType::SOLID))
            continue;

        std::shared_ptr<Tile> neighborWithOxygen = station->GetTileWithComponentAtPosition(neighborPos, ComponentType::OXYGEN);
        if (!neighborWithOxygen)
            continue;

        auto neighborOxygen = neighborWithOxygen->GetComponent<OxygenComponent>();
        double oxygenDiff = GetOxygenLevel() - neighborOxygen->GetOxygenLevel();

        if (oxygenDiff <= 0)
            continue;

        float oxygenTransfer = std::min(oxygenDiff * OXYGEN_DIFFUSION_RATE * deltaTime, oxygenDiff);
        SetOxygenLevel(GetOxygenLevel() - oxygenTransfer);
        neighborOxygen->SetOxygenLevel(neighborOxygen->GetOxygenLevel() + oxygenTransfer);
    }
}

float SolarPanelComponent::GetPowerProduction() const
{
    if (auto tile = GetParent())
    {
        if (auto station = tile->GetStation())
        {
            if (station->GetTileWithComponentAtPosition(tile->GetPosition(), ComponentType::OXYGEN))
                return 0.f;
        }
    }
    return PowerProducerComponent::GetPowerProduction();
}
