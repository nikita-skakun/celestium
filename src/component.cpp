#include "component.hpp"
#include "station.hpp"

template <>
struct magic_enum::customize::enum_range<PowerConnectorComponent::IO>
{
    static constexpr bool is_flags = true;
};

void BatteryComponent::Charge()
{
    if (charge >= BATTERY_CHARGE_MAX)
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
            float transferredCharge = std::clamp(connectedProducer->GetAvailablePower(), 0.f, BATTERY_CHARGE_MAX - charge);
            charge += transferredCharge;
            connectedProducer->UseAvailablePower(transferredCharge);
        }

        if (charge >= BATTERY_CHARGE_MAX)
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
    if (!parent || !parent->station)
        return;

    auto oxygenTile = parent->station->GetTileWithComponentAtPosition<OxygenComponent>(parent->position);
    auto powerConsumer = parent->GetComponent<PowerConsumerComponent>();
    if (!oxygenTile || !powerConsumer || !powerConsumer->IsActive())
        return;

    auto oxygen = oxygenTile->GetComponent<OxygenComponent>();
    oxygen->SetOxygenLevel(std::min(oxygen->GetOxygenLevel() + OXYGEN_PRODUCTION_RATE * deltaTime, TILE_OXYGEN_MAX));
}