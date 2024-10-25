#include "component.hpp"
#include "station.hpp"

template <>
struct magic_enum::customize::enum_range<PowerConnectorComponent::IO>
{
    static constexpr bool is_flags = true;
};

void BatteryComponent::Charge()
{
    if (charge >= maxCharge)
        return;

    std::shared_ptr<Tile> parent = _parent.lock();
    if (!parent)
        return;

    auto powerConnector = parent->GetComponent<PowerConnectorComponent>();
    if (!powerConnector)
        return;

    auto connections = powerConnector->GetConnections();

    for (auto &connection : connections)
    {
        auto connectedTile = connection->_parent.lock();
        if (!connectedTile)
            continue;

        if (auto connectedProducer = connectedTile->GetComponent<PowerProducerComponent>())
        {
            float transferredCharge = std::clamp(connectedProducer->GetAvailablePower(), 0.f, maxCharge - charge);
            charge += transferredCharge;
            connectedProducer->UseAvailablePower(transferredCharge);
        }

        if (charge >= maxCharge)
            return;
    }
}

void PowerConsumerComponent::ConsumePower(float deltaTime)
{
    std::shared_ptr<Tile> parent = _parent.lock();
    if (!isPoweredOn || !parent)
    {
        isActive = false;
        return;
    }

    auto powerConnector = parent->GetComponent<PowerConnectorComponent>();

    if (!powerConnector)
    {
        isActive = false;
        return;
    }

    auto connections = powerConnector->GetConnections();
    std::vector<std::shared_ptr<PowerProducerComponent>> connectedProducers;
    std::vector<std::shared_ptr<BatteryComponent>> connectedBatteries;
    float totalAvailablePower = 0.f;

    for (auto &connection : connections)
    {
        auto connectedTile = connection->_parent.lock();
        if (!connectedTile)
            continue;

        if (auto producer = connectedTile->GetComponent<PowerProducerComponent>())
        {
            connectedProducers.push_back(producer);
            totalAvailablePower += producer->GetAvailablePower();
        }

        if (auto battery = connectedTile->GetComponent<BatteryComponent>())
        {
            connectedBatteries.push_back(battery);
            totalAvailablePower += battery->GetChargeLevel();
        }
    }

    std::sort(connectedBatteries.begin(), connectedBatteries.end(),
              [](const std::shared_ptr<BatteryComponent> &a, const std::shared_ptr<BatteryComponent> &b)
              { return a->GetChargeLevel() > b->GetChargeLevel(); });

    float consumeAccount = GetPowerConsumption() * deltaTime;

    if (totalAvailablePower < consumeAccount)
    {
        isActive = false;
        return;
    }

    float totalConsumed = 0.f;
    for (auto &producers : connectedProducers)
    {
        float consumed = std::min(producers->GetAvailablePower(), consumeAccount);
        producers->UseAvailablePower(consumed);
        totalConsumed += consumed;

        if (totalConsumed >= consumeAccount)
        {
            isActive = true;
            return;
        }
    }

    for (auto &battery : connectedBatteries)
    {
        float consumed = std::min(battery->GetChargeLevel(), consumeAccount);
        battery->Drain(consumed);
        totalConsumed += consumed;

        if (totalConsumed >= consumeAccount)
        {
            isActive = true;
            return;
        }
    }

    isActive = false;
}

void OxygenProducerComponent::ProduceOxygen(float deltaTime) const
{
    auto parent = _parent.lock();
    if (!parent || !parent->GetStation())
        return;

    auto oxygenTile = parent->GetStation()->GetTileWithComponentAtPosition<OxygenComponent>(parent->GetPosition());
    auto powerConsumer = parent->GetComponent<PowerConsumerComponent>();
    if (!oxygenTile || (powerConsumer && !powerConsumer->IsActive()))
        return;

    auto oxygen = oxygenTile->GetComponent<OxygenComponent>();
    oxygen->SetOxygenLevel(std::min(oxygen->GetOxygenLevel() + oxygenProduction * deltaTime, TILE_OXYGEN_MAX));
}

void DoorComponent::SetOpenState(bool openState)
{
    auto parent = _parent.lock();
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

    if (auto parent = _parent.lock())
    {
        auto powerConsumer = parent->GetComponent<PowerConsumerComponent>();
        if (powerConsumer && !powerConsumer->IsActive())
            return;
    }

    float direction = movingState == MovingState::CLOSING ? 1.f : -1.f;

    SetProgress(progress + direction * movingSpeed * deltaTime);
    if (progress >= 1.f)
        SetOpenState(false);
    if (progress <= 0.f)
        SetOpenState(true);
}

void DurabilityComponent::SetHitpoints(float newHitpoints)
{
    hitpoints = std::max(newHitpoints, 0.f);
    if (hitpoints > 0.f || _parent.expired())
        return;

    _parent.lock()->DeleteTile();
}

void OxygenComponent::Diffuse(float deltaTime)
{
    auto parent = _parent.lock();
    if (!parent || !parent->GetStation())
        return;

    auto station = parent->GetStation();

    for (int i = 0; i < CARDINAL_DIRECTIONS.size(); i++)
    {
        Vector2Int neighborPos = parent->GetPosition() + DirectionToVector2Int(CARDINAL_DIRECTIONS.at(i));

        if (!station->GetTileAtPosition(neighborPos))
        {
            SetOxygenLevel(GetOxygenLevel() - GetOxygenLevel() * OXYGEN_DIFFUSION_RATE * deltaTime);
            continue;
        }

        if (station->GetTileWithComponentAtPosition<SolidComponent>(neighborPos))
            continue;

        std::shared_ptr<Tile> neighborWithOxygen = station->GetTileWithComponentAtPosition<OxygenComponent>(neighborPos);
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
