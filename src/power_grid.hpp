#pragma once
#include "tile.hpp"
#include <unordered_set>

struct PowerGrid
{
protected:
    std::unordered_set<Vector2Int> powerWires;
    std::unordered_map<Vector2Int, std::weak_ptr<PowerConsumerComponent>> _consumers;
    std::unordered_map<Vector2Int, std::weak_ptr<PowerProducerComponent>> _producers;
    std::unordered_map<Vector2Int, std::weak_ptr<BatteryComponent>> _batteries;

    std::vector<std::shared_ptr<PowerConsumerComponent>> cachedConsumers;
    std::vector<std::shared_ptr<PowerProducerComponent>> cachedProducers;
    std::vector<std::shared_ptr<BatteryComponent>> cachedBatteries;

    bool dirty = false;

public:
    constexpr u_int8_t GetWireProximityState(Vector2Int pos) const
    {
        if (Contains(powerWires, pos))
            return 1; // Wire at the given position

        for (const auto &dir : CARDINAL_DIRECTIONS)
        {
            Vector2Int adjacentPos = pos + DirectionToVector2Int(dir);
            if (Contains(powerWires, adjacentPos))
                return 2; // Wire adjacent to the given position
        }

        return 0; // No wire at or adjacent to the given position
    }

    constexpr bool ContainsWire(Vector2Int pos) const { return Contains(powerWires, pos); }
    constexpr const std::unordered_set<Vector2Int> &GetWires() const { return powerWires; }

    constexpr void AddWire(Vector2Int pos)
    {
        powerWires.insert(pos);
        dirty = true;
    }

    constexpr void RemoveWire(Vector2Int pos)
    {
        powerWires.erase(pos);
        dirty = true;
    }

    constexpr void AddConsumer(Vector2Int pos, std::shared_ptr<PowerConsumerComponent> consumer)
    {
        _consumers[pos] = consumer;
        dirty = true;
    }

    constexpr void AddProducer(Vector2Int pos, std::shared_ptr<PowerProducerComponent> producer)
    {
        _producers[pos] = producer;
        dirty = true;
    }

    constexpr void AddBattery(Vector2Int pos, std::shared_ptr<BatteryComponent> battery)
    {
        _batteries[pos] = battery;
        dirty = true;
    }

    constexpr void MergeGrid(std::shared_ptr<PowerGrid> other)
    {
        powerWires.merge(other->powerWires);
        _consumers.merge(other->_consumers);
        _producers.merge(other->_producers);
        _batteries.merge(other->_batteries);

        // TODO: Update the power connector components of the tiles in this grid

        dirty = true;
    }

    constexpr void Disconnect(std::shared_ptr<Tile> parentTile)
    {
        if (!parentTile)
            return;

        // TODO: Replace with loop when the map will have a list of connections
        if (auto consumerIt = _consumers.find(parentTile->GetPosition()); consumerIt != _consumers.end())
        {
            auto consumer = consumerIt->second.lock();
            if (consumer && consumer->GetParent() == parentTile)
            {
                _consumers.erase(consumerIt);
            }
        }

        if (auto producerIt = _producers.find(parentTile->GetPosition()); producerIt != _producers.end())
        {
            auto producer = producerIt->second.lock();
            if (producer && producer->GetParent() == parentTile)
            {
                _producers.erase(producerIt);
            }
        }

        if (auto batteryIt = _batteries.find(parentTile->GetPosition()); batteryIt != _batteries.end())
        {
            auto battery = batteryIt->second.lock();
            if (battery && battery->GetParent() == parentTile)
            {
                _batteries.erase(batteryIt);
            }
        }

        if (auto connector = parentTile->GetComponent<PowerConnectorComponent>())
            connector->SetPowerGrid(nullptr);

        dirty = true;
    }

    constexpr float GetTotalPowerConsumption() const
    {
        float totalPower = 0.f;
        for (const auto &consumer : cachedConsumers)
        {
            totalPower += consumer->GetPowerConsumption();
        }
        return totalPower;
    }

    constexpr float GetTotalPowerProduction() const
    {
        float totalPower = 0.f;
        for (const auto &producer : cachedProducers)
        {
            totalPower += producer->GetPowerProduction();
        }
        return totalPower;
    }

    constexpr float GetTotalBatteryCharge() const
    {
        float totalPower = 0.f;
        for (const auto &battery : cachedBatteries)
        {
            totalPower += battery->GetChargeLevel();
        }
        return totalPower;
    }

    constexpr float GetTotalMaxBatteryCharge() const
    {
        float totalPower = 0.f;
        for (const auto &battery : cachedBatteries)
        {
            totalPower += battery->GetMaxChargeLevel();
        }
        return totalPower;
    }

    constexpr float GetTotalBatteryCapacity() const
    {
        float totalPower = 0.f;
        for (const auto &battery : cachedBatteries)
        {
            totalPower += battery->GetMaxChargeLevel() - battery->GetChargeLevel();
        }
        return totalPower;
    }

    float DrainBatteriesProportionally(float amount)
    {
        if (amount <= 0.f || cachedBatteries.empty())
            return 0.f;

        float totalCharge = GetTotalBatteryCharge();
        if (totalCharge <= 0.f)
            return 0.f;

        float remaining = amount;

        for (auto &battery : cachedBatteries)
        {
            float charge = battery->GetChargeLevel();
            if (charge <= 0.f)
                continue;

            float shareRatio = charge / totalCharge;
            float share = remaining * shareRatio;

            float drained = battery->Drain(share);
            remaining -= drained;

            if (remaining <= 0.f)
                break;
        }

        return amount - remaining;
    }

    void RebuildCaches()
    {
        // Update the cached consumers
        cachedConsumers.clear();
        cachedConsumers.reserve(_consumers.size());
        for (auto it = _consumers.begin(); it != _consumers.end();)
        {
            if (auto consumer = it->second.lock())
            {
                if (consumer->GetPowerPriority() != PowerConsumerComponent::PowerPriority::OFFLINE)
                {
                    cachedConsumers.push_back(consumer);
                }
                ++it;
            }
            else
            {
                it = _consumers.erase(it);
            }
        }

        std::ranges::sort(cachedConsumers,
                          [](const auto &a, const auto &b)
                          {
                              return (a->GetPowerPriority() != b->GetPowerPriority()) ? (a->GetPowerPriority() < b->GetPowerPriority()) : (a->GetPowerConsumption() > b->GetPowerConsumption());
                          });

        // Update the cached producers
        cachedProducers.clear();
        cachedProducers.reserve(_producers.size());
        for (auto it = _producers.begin(); it != _producers.end();)
        {
            if (auto producer = it->second.lock())
            {
                cachedProducers.push_back(producer);
                ++it;
            }
            else
            {
                it = _producers.erase(it);
            }
        }

        // Update the cached batteries
        cachedBatteries.clear();
        cachedBatteries.reserve(_batteries.size());
        for (auto it = _batteries.begin(); it != _batteries.end();)
        {
            if (auto battery = it->second.lock())
            {
                cachedBatteries.push_back(battery);
                ++it;
            }
            else
            {
                it = _batteries.erase(it);
            }
        }

        dirty = false;
    }

    void Update(float deltaTime)
    {
        if (dirty)
            RebuildCaches();

        float totalDemand = GetTotalPowerConsumption();
        float availablePower = GetTotalPowerProduction();
        float totalBatteryCharge = GetTotalBatteryCharge();

        float deficit = (totalDemand - availablePower) * deltaTime;

        if (deficit <= 0.f)
        {
            // ✅ Enough production to power everything
            for (const auto &consumer : cachedConsumers)
            {
                consumer->SetActive(true);
            }

            // ✅ Charge batteries with surplus
            float surplus = -deficit;

            // Compute total remaining capacity
            float totalCapacity = GetTotalBatteryCapacity();

            if (totalCapacity <= 0.f || surplus <= 0.f)
                return; // Nothing to charge or nothing to charge with

            // Charge batteries with priority to the emptier ones
            for (const auto &battery : cachedBatteries)
            {
                float remaining = battery->GetMaxChargeLevel() - battery->GetChargeLevel();
                if (remaining <= 0.f)
                    continue;

                float shareRatio = remaining / totalCapacity;
                float share = surplus * shareRatio;

                float added = battery->AddCharge(share);
                surplus -= added;

                if (surplus <= 0.f)
                    break;
            }
        }
        else if (totalBatteryCharge >= deficit)
        {
            // ✅ Not enough production, but batteries can cover it

            // Power all consumers
            for (const auto &consumer : cachedConsumers)
            {
                consumer->SetActive(true);
            }

            // Drain batteries evenly
            DrainBatteriesProportionally(deficit);
        }
        else
        {
            // ❌ Not enough even with batteries: allocate power by priority

            float usablePower = availablePower + totalBatteryCharge;

            for (const auto &consumer : cachedConsumers)
            {
                float demand = consumer->GetPowerConsumption();
                if (usablePower >= demand)
                {
                    consumer->SetActive(true);
                    usablePower -= demand;
                }
                else
                {
                    consumer->SetActive(false);
                }
            }

            // Drain batteries proportionally if any battery power was used
            float toDrain = std::max(0.f, (availablePower + totalBatteryCharge - usablePower - availablePower)) * deltaTime;
            DrainBatteriesProportionally(toDrain);
        }
    }
};