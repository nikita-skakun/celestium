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

void DoorComponent::SetOpenState(bool openState)
{
    auto parent = GetParent();
    if (!parent)
        return;

    if (openState)
        parent->RemoveComponent<SolidComponent>();
    else
        parent->AddComponent<SolidComponent>();

    isOpen = openState;

    if (movingState != MovingState::FORCED_OPEN)
        movingState = MovingState::IDLE;
}

void DoorComponent::Animate(float deltaTime)
{
    if (movingState == MovingState::IDLE)
        return;

    auto parent = GetParent();
    if (!parent->IsActive())
        return;

    float direction = movingState == MovingState::CLOSING ? 1.f : -1.f;

    SetProgress(progress + direction * movingSpeed * deltaTime);
    if (progress >= 1.f)
        SetOpenState(false);
    if (progress <= 0.f)
        SetOpenState(true);
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
            if (auto oxygenTile = station->GetTileWithComponentAtPosition(tile->GetPosition(), ComponentType::OXYGEN))
                return 0.f;
        }
    }
    return PowerProducerComponent::GetPowerProduction();
}
