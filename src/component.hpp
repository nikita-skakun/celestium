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
    enum class Type : u_int8_t
    {
        NONE,
        WALKABLE,
        SOLID,
        POWER_CONNECTOR,
        BATTERY,
        POWER_CONSUMER,
        POWER_PRODUCER,
        SOLAR_PANEL,
        OXYGEN,
        OXYGEN_PRODUCER,
        DECORATIVE,
        DOOR,
    };

    std::weak_ptr<Tile> _parent;

    Component(std::shared_ptr<Tile> parent = nullptr) : _parent(parent) {}

    virtual std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const = 0;
    virtual ~Component() = default;

    constexpr virtual Type GetType() const
    {
        return Type::NONE;
    }

    constexpr std::string GetName() const
    {
        std::string name = std::string(magic_enum::enum_name<Type>(GetType()));
        std::replace(name.begin(), name.end(), '_', ' ');
        return StringToTitleCase(name);
    }

    bool operator==(const Component &other) const
    {
        return GetName() == other.GetName();
    }
};

namespace std
{
    template <>
    struct hash<Component>
    {
        std::size_t operator()(const Component &component) const
        {
            return std::hash<std::string>()(component.GetName());
        }
    };

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
    WalkableComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<WalkableComponent>(newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::WALKABLE;
    }
};

struct SolidComponent : Component
{
    SolidComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<SolidComponent>(newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::SOLID;
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
    PowerConnectorComponent(IO io, std::shared_ptr<Tile> parent = nullptr) : Component(parent), io(io) {}

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

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<PowerConnectorComponent>(io, newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::POWER_CONNECTOR;
    }
};

struct BatteryComponent : Component
{
private:
    float charge;
    float maxCharge;

public:
    BatteryComponent(float maxCharge, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), charge(maxCharge), maxCharge(maxCharge) {}

    constexpr bool Drain(float amount)
    {
        if (charge < amount)
            return false;

        charge -= amount;
        return true;
    }

    constexpr float GetMaxChargeLevel() const
    {
        return maxCharge;
    }

    constexpr float GetChargeLevel() const
    {
        return charge;
    }

    void Charge();

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<BatteryComponent>(charge, newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::BATTERY;
    }
};

struct PowerConsumerComponent : Component
{
private:
    bool isPoweredOn;
    bool isActive;
    float powerConsumption;

public:
    PowerConsumerComponent(float powerConsumption, std::shared_ptr<Tile> parent = nullptr)
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

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<PowerConsumerComponent>(powerConsumption, newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::POWER_CONSUMER;
    }
};

struct PowerProducerComponent : Component
{
private:
    float powerProduction;
    float availablePower;

public:
    PowerProducerComponent(float powerProduction, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), powerProduction(std::max(powerProduction, 0.f)), availablePower(0.f) {}

    constexpr float GetPowerProduction() const
    {
        return powerProduction;
    }

    constexpr float GetAvailablePower() const
    {
        return availablePower;
    }

    constexpr void UseAvailablePower(float usedPower)
    {
        availablePower -= usedPower;
    }

    constexpr void ProducePower(float deltaTime)
    {
        availablePower = GetPowerProduction() * deltaTime;
    }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<PowerProducerComponent>(powerProduction, newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::POWER_PRODUCER;
    }
};

struct SolarPanelComponent : Component
{
    SolarPanelComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<SolarPanelComponent>(newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::SOLAR_PANEL;
    }
};

struct OxygenComponent : Component
{
private:
    float oxygenLevel;

public:
    OxygenComponent(float startOxygenLevel = TILE_OXYGEN_MAX, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), oxygenLevel(startOxygenLevel) {}

    void SetOxygenLevel(float oxygen)
    {
        oxygenLevel = oxygen;
    }

    float &GetOxygenLevel()
    {
        return oxygenLevel;
    }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<OxygenComponent>(oxygenLevel, newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::OXYGEN;
    }
};

struct OxygenProducerComponent : Component
{
private:
    float oxygenProduction;

public:
    OxygenProducerComponent(float oxygenProduction, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), oxygenProduction(oxygenProduction) {}

    void ProduceOxygen(float deltaTime) const;

    constexpr float GetOxygenProduction() const
    {
        return oxygenProduction;
    }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<OxygenProducerComponent>(oxygenProduction, newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::OXYGEN_PRODUCER;
    }
};

struct DecorativeComponent : Component
{
private:
    std::vector<DecorativeTile> decorativeTiles;

public:
    DecorativeComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

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

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<DecorativeComponent>(newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::DECORATIVE;
    }
};

struct DoorComponent : Component
{
    enum class MovingState : u_int8_t
    {
        IDLE,
        OPENING,
        CLOSING,
    };

private:
    float movingSpeed;
    bool isOpen;
    MovingState movingState;
    float progress;

public:
    DoorComponent(float movingSpeed = 1.f, bool openState = false, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), movingSpeed(std::max(movingSpeed, 0.f)), isOpen(openState),
          movingState(MovingState::IDLE), progress(openState ? 0.f : 1.f) {}

    constexpr bool IsOpen() const { return isOpen; }
    constexpr MovingState GetMovingState() const { return movingState; }
    constexpr float GetProgress() const { return progress; }
    constexpr void SetProgress(float newProgress) { progress = std::clamp(newProgress, 0.f, 1.f); }

    void SetOpenState(bool openState);

    constexpr std::string GetMovementName() const
    {
        std::string name = std::string(magic_enum::enum_name<MovingState>(movingState));
        std::replace(name.begin(), name.end(), '_', ' ');
        return StringToTitleCase(name);
    }

    constexpr void SetMovingState(MovingState newMovingState)
    {
        if (newMovingState == movingState)
            return;

        if ((progress >= 1.f && newMovingState == MovingState::CLOSING) ||
            (progress <= 0.f && newMovingState == MovingState::OPENING))
        {
            movingState = MovingState::IDLE;
            return;
        }

        movingState = newMovingState;
    }

    constexpr void Animate(float deltaTime)
    {
        if (movingState == MovingState::IDLE)
            return;

        float direction = movingState == MovingState::CLOSING ? 1.f : -1.f;

        SetProgress(progress + direction * movingSpeed * deltaTime);
        if ((isOpen && progress >= 1.f) || (!isOpen && progress <= 0.f))
            SetOpenState(!isOpen);
    }

    constexpr void PingPong()
    {
        if (movingState != MovingState::IDLE)
            return;

        movingState = isOpen ? MovingState::CLOSING : MovingState::OPENING;
    }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<DoorComponent>(movingSpeed, isOpen, newParent);
    }

    constexpr Type GetType() const override
    {
        return Type::DOOR;
    }
};