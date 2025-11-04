#pragma once
#include "utils.hpp"

struct Tile;

struct PowerConnectorComponent;
struct PowerConsumerComponent;
struct PowerProducerComponent;
struct BatteryComponent;

struct PowerGrid : public std::enable_shared_from_this<PowerGrid>
{
protected:
    std::unordered_map<Vector2Int, std::weak_ptr<PowerConsumerComponent>> _consumers;
    std::unordered_map<Vector2Int, std::weak_ptr<PowerProducerComponent>> _producers;
    std::unordered_map<Vector2Int, std::weak_ptr<BatteryComponent>> _batteries;

    std::vector<std::shared_ptr<PowerConsumerComponent>> cachedConsumers;
    std::vector<std::shared_ptr<PowerProducerComponent>> cachedProducers;
    std::vector<std::shared_ptr<BatteryComponent>> cachedBatteries;

    bool dirty = false;
    Color debugColor = WHITE;

public:
    PowerGrid()
    {
        debugColor = RandomColor();
        debugColor.a = 192;
        dirty = false;
    }

    constexpr void SetDebugColor(const Color &c) { debugColor = c; }
    constexpr Color GetDebugColor() const { return debugColor; }

    constexpr void AddConsumer(Vector2Int pos, const std::shared_ptr<PowerConsumerComponent> &consumer)
    {
        _consumers[pos] = consumer;
        dirty = true;
    }

    constexpr void AddProducer(Vector2Int pos, const std::shared_ptr<PowerProducerComponent> &producer)
    {
        _producers[pos] = producer;
        dirty = true;
    }

    constexpr void AddBattery(Vector2Int pos, const std::shared_ptr<BatteryComponent> &battery)
    {
        _batteries[pos] = battery;
        dirty = true;
    }

    void Disconnect(const std::shared_ptr<Tile> &parentTile);

    float GetTotalPowerConsumption() const;
    float GetTotalPowerProduction() const;
    float GetTotalBatteryCharge() const;
    float GetTotalMaxBatteryCharge() const;
    float GetTotalBatteryCapacity() const;

    float DrainBatteriesProportionally(float amount);
    void RebuildCaches();
    void Update(float deltaTime);
};