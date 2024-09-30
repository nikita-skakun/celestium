#include "component.hpp"
#include "station.hpp"

template <>
struct magic_enum::customize::enum_range<PowerConnectorComponent::IO>
{
    static constexpr bool is_flags = true;
};

void PowerConsumerComponent::ConsumePower(float deltaTime)
{
    std::shared_ptr<Tile> parent = _parent.lock();
    auto powerConnector = parent->GetComponent<PowerConnectorComponent>();

    if (!isPoweredOn || !powerConnector)
    {
        isActive = false;
        return;
    }

    auto connections = powerConnector->GetConnections();
    std::vector<std::shared_ptr<BatteryComponent>> connectedBatteries;

    for (auto &connection : connections)
    {
        auto connectedTile = connection->_parent.lock();
        if (!connectedTile)
            continue;

        if (auto battery = connectedTile->GetComponent<BatteryComponent>())
        {
            connectedBatteries.push_back(battery);
            continue;
        }
    }

    std::sort(connectedBatteries.begin(), connectedBatteries.end(),
              [](const std::shared_ptr<BatteryComponent> &a, const std::shared_ptr<BatteryComponent> &b)
              { return a->GetChargeLevel() > b->GetChargeLevel(); });

    float consumeAccount = GetPowerConsumption() * deltaTime;
    float totalBatteryCharge = std::accumulate(connectedBatteries.begin(), connectedBatteries.end(), 0.0f,
                                               [](float sum, const std::shared_ptr<BatteryComponent> &battery)
                                               { return sum + battery->GetChargeLevel(); });

    if (totalBatteryCharge < consumeAccount)
    {
        isActive = false;
        return;
    }

    float totalConsumed = 0.f;
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
