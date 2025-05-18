#pragma once
#include "direction.hpp"
#include "tile.hpp"
#include <unordered_set>
#include "logging.hpp"

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

        LogMessage(LogLevel::DEBUG, "Added wire at position: " + ToString(pos));
    }

    void RemoveWire(Vector2Int pos)
    {
        powerWires.erase(pos);
        dirty = true;

        LogMessage(LogLevel::DEBUG, "Removed wire at position: " + ToString(pos));
    }

    void MergeGrid(std::shared_ptr<PowerGrid> other)
    {
        powerWires.merge(other->powerWires);
        consumers.merge(other->consumers);
        producers.merge(other->producers);
        batteries.merge(other->batteries);

        dirty = true;

        LogMessage(LogLevel::DEBUG, "Merged grids");
    }

    void Disconnect(std::shared_ptr<Tile> parentTile)
    {
        if (!parentTile)
            return;

        // TODO: Replace with loop when the map will have a list of connections
        if (auto consumerIt = consumers.find(parentTile->GetPosition()); consumerIt != consumers.end())
        {
            if (auto consumer = consumerIt->second.lock(); consumer->GetParent() == parentTile)
            {
                consumers.erase(consumerIt);
                LogMessage(LogLevel::DEBUG, "Disconnected consumer at position: " + ToString(parentTile->GetPosition()));
            }
        }

        if (auto producerIt = producers.find(parentTile->GetPosition()); producerIt != producers.end())
        {
            if (auto producer = producerIt->second.lock(); producer->GetParent() == parentTile)
            {
                producers.erase(producerIt);
                LogMessage(LogLevel::DEBUG, "Disconnected producer at position: " + ToString(parentTile->GetPosition()));
            }
        }

        if (auto batteryIt = batteries.find(parentTile->GetPosition()); batteryIt != batteries.end())
        {
            if (auto battery = batteryIt->second.lock(); battery->GetParent() == parentTile)
            {
                batteries.erase(batteryIt);
                LogMessage(LogLevel::DEBUG, "Disconnected battery at position: " + ToString(parentTile->GetPosition()));
            }
        }

        if (auto connector = parentTile->GetComponent<PowerConnectorComponent>())
            connector->SetPowerGrid(nullptr);

        dirty = true;
    }

    void Print() const
    {
        LogMessage(LogLevel::DEBUG, "Power Grid:");
        LogMessage(LogLevel::DEBUG, "- Wires: " + std::to_string(powerWires.size()));
        LogMessage(LogLevel::DEBUG, "- Consumers: " + std::to_string(consumers.size()));
        LogMessage(LogLevel::DEBUG, "- Producers: " + std::to_string(producers.size()));
        LogMessage(LogLevel::DEBUG, "- Batteries: " + std::to_string(batteries.size()));
    }
};