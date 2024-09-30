#pragma once
#include "utils.hpp"
#include "logging.hpp"
#include <memory>
#include <vector>

struct Tile;

struct DecorativeTile
{

    Vector2Int offset;
    Vector2Int spriteOffset;

    DecorativeTile(const Vector2Int &offset, const Vector2Int &spriteOffset) : offset(offset), spriteOffset(spriteOffset) {}
};

struct Component
{
    std::weak_ptr<Tile> _parent;

    Component(std::shared_ptr<Tile> parent) : _parent(parent) {}

    virtual std::string GetName() const = 0;
    virtual ~Component() = default;

    bool operator==(const Component &other) const
    {
        return GetName() == other.GetName();
    }
};

namespace std
{
    template <>
    struct hash<std::shared_ptr<Component>>
    {
        std::size_t operator()(const std::shared_ptr<Component> &component) const
        {
            return std::hash<std::string>()(component->GetName());
        }
    };
}

struct WalkableComponent : Component
{
    WalkableComponent(std::shared_ptr<Tile> parent) : Component(parent) {}

    std::string GetName() const override
    {
        return "Walkable";
    }
};

struct SolidComponent : Component
{
    SolidComponent(std::shared_ptr<Tile> parent) : Component(parent) {}

    std::string GetName() const override
    {
        return "Solid";
    }
};

struct BatteryComponent : Component
{
private:
    float charge;

public:
    BatteryComponent(std::shared_ptr<Tile> parent, float startCharge = BATTERY_CHARGE_MAX)
        : Component(parent), charge(startCharge) {}

    void Charge(float amount)
    {
        charge = std::min(charge + amount, BATTERY_CHARGE_MAX);
    }

    bool Drain(float amount)
    {
        if (charge < amount)
            return false;

        charge -= amount;
        return true;
    }

    float GetChargeLevel() const
    {
        return charge;
    }

    std::string GetName() const override
    {
        return "Battery";
    }
};

struct PowerConnectorComponent : Component
{
    enum class IO : u_int8_t
    {
        INPUT = 1 << 0,
        OUTPUT = 1 << 1,
    };

private:
    IO io;

public:
    std::vector<std::weak_ptr<PowerConnectorComponent>> _connections;
    PowerConnectorComponent(std::shared_ptr<Tile> parent, IO io) : Component(parent), io(io) {}

    constexpr IO GetIO() const
    {
        return io;
    }

    std::vector<std::shared_ptr<PowerConnectorComponent>> GetConnections()
    {
        std::vector<std::shared_ptr<PowerConnectorComponent>> connections;

        for (int i = _connections.size() - 1; i >= 0; --i)
        {
            if (auto connection = _connections[i].lock())
            {
                connections.push_back(connection);
            }
            else
                _connections.erase(_connections.begin() + i);
        }

        return connections;
    }

    static bool AddConnection(std::shared_ptr<PowerConnectorComponent> a, std::shared_ptr<PowerConnectorComponent> b)
    {
        for (int i = a->_connections.size() - 1; i >= 0; --i)
        {
            if (auto connection = a->_connections[i].lock())
            {
                if (connection == b)
                    return false;
            }
            else
                a->_connections.erase(a->_connections.begin() + i);
        }

        if ((a->io | b->io) == (IO::INPUT | IO::OUTPUT))
        {
            a->_connections.push_back(b);
            b->_connections.push_back(a);
            return true;
        }
        return false;
    }

    static void DeleteConnection(std::shared_ptr<PowerConnectorComponent> a, std::shared_ptr<PowerConnectorComponent> b)
    {
        for (int i = a->_connections.size() - 1; i >= 0; --i)
        {
            if (auto connection = a->_connections[i].lock())
            {
                if (connection == b)
                {
                    a->_connections.erase(a->_connections.begin() + i);
                    break;
                }
            }
            else
                a->_connections.erase(a->_connections.begin() + i);
        }

        for (int i = b->_connections.size() - 1; i >= 0; --i)
        {
            if (auto connection = b->_connections[i].lock())
            {
                if (connection == a)
                {
                    b->_connections.erase(b->_connections.begin() + i);
                    break;
                }
            }
            else
                b->_connections.erase(b->_connections.begin() + i);
        }
    }

    std::string GetName() const override
    {
        return "Power Connector";
    }
};

struct PowerConsumerComponent : Component
{
private:
    bool isPoweredOn;
    bool isActive;
    float powerConsumption;

public:
    PowerConsumerComponent(std::shared_ptr<Tile> parent, float powerConsumption)
        : Component(parent), isPoweredOn(true), isActive(false), powerConsumption(std::max(powerConsumption, 0.f)) {}

    constexpr bool IsPoweredOn() const
    {
        return isPoweredOn;
    }

    constexpr void SetPoweredState(bool state)
    {
        isPoweredOn = state;
    }

    constexpr bool IsActive() const
    {
        return isActive;
    }

    constexpr float GetPowerConsumption() const
    {
        return powerConsumption;
    }

    void ConsumePower(float deltaTime);

    std::string GetName() const override
    {
        return "Power Consumer";
    }
};

struct OxygenComponent : Component
{
private:
    float oxygenLevel;

public:
    OxygenComponent(std::shared_ptr<Tile> parent, float startOxygenLevel = TILE_OXYGEN_MAX)
        : Component(parent), oxygenLevel(startOxygenLevel) {}

    void SetOxygenLevel(float oxygen)
    {
        oxygenLevel = oxygen;
    }

    float &GetOxygenLevel()
    {
        return oxygenLevel;
    }

    std::string GetName() const override
    {
        return "Oxygen";
    }
};

struct OxygenProducerComponent : Component
{
    static constexpr float POWER_CONSUMPTION = 2.f;

    OxygenProducerComponent(std::shared_ptr<Tile> parent) : Component(parent) {}

    void ProduceOxygen(float deltaTime) const;

    std::string GetName() const override
    {
        return "Oxygen Producer";
    }
};

struct DecorativeComponent : Component
{
private:
    std::vector<DecorativeTile> decorativeTiles;

public:
    DecorativeComponent(std::shared_ptr<Tile> parent) : Component(parent) {}

    void AddDecorativeTile(const Vector2Int &offset, const Vector2Int &spriteOffset)
    {
        decorativeTiles.emplace_back(offset, spriteOffset);
    }

    const std::vector<DecorativeTile> &GetDecorativeTiles() const
    {
        return decorativeTiles;
    }

    void ClearDecorativeTiles()
    {
        decorativeTiles.clear();
    }

    std::string GetName() const override
    {
        return "Decorative";
    }
};