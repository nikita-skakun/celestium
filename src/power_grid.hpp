#pragma once
#include "direction.hpp"
#include "component.hpp"
#include <unordered_set>

struct PowerGrid
{
    std::unordered_set<Vector2Int> powerWires;
    std::unordered_map<Vector2Int, std::weak_ptr<PowerConsumerComponent>> consumers;
    std::unordered_map<Vector2Int, std::weak_ptr<PowerProducerComponent>> producers;
    std::unordered_map<Vector2Int, std::weak_ptr<BatteryComponent>> batteries;

    bool dirty = false;

    u_int8_t GetWireProximityState(Vector2Int pos) const
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

    bool ContainsWire(Vector2Int pos) const
    {
        return Contains(powerWires, pos);
    }

    void AddWire(Vector2Int pos)
    {
        powerWires.insert(pos);
        dirty = true;
    }

    void RemoveWire(Vector2Int pos)
    {
        powerWires.erase(pos);
        dirty = true;
    }

    void MergeGrid(std::shared_ptr<PowerGrid> other)
    {
        powerWires.merge(other->powerWires);
        consumers.merge(other->consumers);
        producers.merge(other->producers);
        batteries.merge(other->batteries);

        dirty = true;
    }
};