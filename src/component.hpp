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

    DecorativeTile(const Vector2Int &o, const Vector2Int &sO) : offset(o), spriteOffset(sO) {}
};

struct Component
{
    std::weak_ptr<Tile> parent;

    Component(std::shared_ptr<Tile> p) : parent(p) {}

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
    WalkableComponent(std::shared_ptr<Tile> p) : Component(p) {}

    std::string GetName() const override
    {
        return "Walkable";
    }
};

struct SolidComponent : Component
{
    SolidComponent(std::shared_ptr<Tile> p) : Component(p) {}

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
    BatteryComponent(std::shared_ptr<Tile> p, float c = BATTERY_CHARGE_MAX) : Component(p), charge(c) {}

    void Charge(float amount)
    {
        charge = std::min(charge + amount, BATTERY_CHARGE_MAX);
    }

    bool Drain(float amount)
    {
        if (amount < charge)
            return false;

        charge = charge - amount;
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

    IO io;
    std::vector<std::weak_ptr<PowerConnectorComponent>> connections;
    PowerConnectorComponent(std::shared_ptr<Tile> p, IO io) : Component(p), io(io) {}

    static bool AddConnection(std::shared_ptr<PowerConnectorComponent> a, std::shared_ptr<PowerConnectorComponent> b)
    {
        for (int i = a->connections.size() - 1; i >= 0; --i)
        {
            if (auto sharedConn = a->connections[i].lock())
            {
                if (sharedConn == b)
                    return false;
            }
            else
                a->connections.erase(a->connections.begin() + i);
        }

        if ((a->io | b->io) == (IO::INPUT | IO::OUTPUT))
        {
            a->connections.push_back(b);
            b->connections.push_back(a);
            return true;
        }
        return false;
    }

    static void DeleteConnection(std::shared_ptr<PowerConnectorComponent> a, std::shared_ptr<PowerConnectorComponent> b)
    {
        for (int i = a->connections.size() - 1; i >= 0; --i)
        {
            if (auto sharedConn = a->connections[i].lock())
            {
                if (sharedConn == b)
                {
                    a->connections.erase(a->connections.begin() + i);
                    break;
                }
            }
            else
                a->connections.erase(a->connections.begin() + i);
        }

        for (int i = b->connections.size() - 1; i >= 0; --i)
        {
            if (auto sharedConn = b->connections[i].lock())
            {
                if (sharedConn == a)
                {
                    b->connections.erase(b->connections.begin() + i);
                    break;
                }
            }
            else
                b->connections.erase(b->connections.begin() + i);
        }
    }

    void CleanOldConnections()
    {
        connections.erase(std::remove_if(connections.begin(), connections.end(), [](const std::weak_ptr<PowerConnectorComponent> &wp)
                                         { return wp.expired(); }),
                          connections.end());
    }

    std::string GetName() const override
    {
        return "Power Connector";
    }
};

struct OxygenComponent : Component
{
private:
    float oxygenLevel;

public:
    OxygenComponent(std::shared_ptr<Tile> p, float o = TILE_OXYGEN_MAX) : Component(p), oxygenLevel(o) {}

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
    std::weak_ptr<OxygenComponent> output;

    OxygenProducerComponent(std::shared_ptr<Tile> p, std::shared_ptr<OxygenComponent> o) : Component(p), output(o) {}

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
    DecorativeComponent(std::shared_ptr<Tile> p) : Component(p) {}

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