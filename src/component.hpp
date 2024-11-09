#pragma once
#include "sprite.hpp"
#include <memory>
#include <vector>

struct Tile;

struct Component : public std::enable_shared_from_this<Component>
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
        DURABILITY,
        ROTATABLE,
    };

    std::weak_ptr<Tile> _parent;

    Component(std::shared_ptr<Tile> parent = nullptr) : _parent(parent) {}

    virtual std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const = 0;
    virtual ~Component() = default;

    constexpr virtual Type GetType() const { return Type::NONE; }
    constexpr virtual std::optional<std::string> GetInfo() = 0;
    constexpr std::string GetName() const { return EnumToName<Type>(GetType()); }

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

    constexpr Type GetType() const override { return Type::WALKABLE; }
    constexpr std::optional<std::string> GetInfo() override { return std::nullopt; }
};

struct SolidComponent : Component
{
    SolidComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<SolidComponent>(newParent);
    }

    constexpr Type GetType() const override { return Type::SOLID; }
    constexpr std::optional<std::string> GetInfo() override { return std::nullopt; }
};

struct PowerConnectorComponent : Component
{
    enum class IO : u_int8_t
    {
        INPUT = 1 << 0,
        OUTPUT = 1 << 1,
    };

protected:
    IO io;

public:
    std::vector<std::weak_ptr<PowerConnectorComponent>> _connections;
    PowerConnectorComponent(IO io, std::shared_ptr<Tile> parent = nullptr) : Component(parent), io(io) {}

    constexpr IO GetIo() const
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

    void DisconnectFromAll()
    {
        // Cast into PowerConnectorComponent
        auto self = std::dynamic_pointer_cast<PowerConnectorComponent>(shared_from_this());

        for (int i = _connections.size() - 1; i >= 0; --i)
        {
            if (auto connection = _connections[i].lock())
            {
                DeleteConnection(self, connection);
            }
            else
                _connections.erase(_connections.begin() + i);
        }
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

    constexpr Type GetType() const override { return Type::POWER_CONNECTOR; }
    constexpr std::optional<std::string> GetInfo() override
    {
        return std::format("   + Power Connector: {} ({})", magic_enum::enum_flags_name(GetIo()), GetConnections().size());
    }
};

struct BatteryComponent : Component
{
protected:
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

    constexpr Type GetType() const override { return Type::BATTERY; }
    constexpr std::optional<std::string> GetInfo() override { return std::format("   + Charge Level: {:.0f} / {:.0f}", charge, maxCharge); }
};

struct PowerConsumerComponent : Component
{
protected:
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

    constexpr Type GetType() const override { return Type::POWER_CONSUMER; }
    constexpr std::optional<std::string> GetInfo() override
    {
        return std::format("   + Power State: {}\n   + Power Consumption: {:.0f}", !isPoweredOn ? "OFF" : (isActive ? "POWERED" : "NOT POWERED"), powerConsumption);
    }
};

struct PowerProducerComponent : Component
{
protected:
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

    constexpr Type GetType() const override { return Type::POWER_PRODUCER; }
    constexpr std::optional<std::string> GetInfo() override { return std::format("   + Power Production: {:.0f}", powerProduction); }
};

// TODO: Implement occlusions for solar panels (if indoor)
struct SolarPanelComponent : Component
{
    SolarPanelComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<SolarPanelComponent>(newParent);
    }

    constexpr Type GetType() const override { return Type::SOLAR_PANEL; }
    constexpr std::optional<std::string> GetInfo() override { return std::nullopt; }
};

struct OxygenComponent : Component
{
protected:
    float oxygenLevel;

public:
    OxygenComponent(float startOxygenLevel = TILE_OXYGEN_MAX, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), oxygenLevel(startOxygenLevel) {}

    constexpr void SetOxygenLevel(float oxygen)
    {
        oxygenLevel = oxygen;
    }

    constexpr float &GetOxygenLevel()
    {
        return oxygenLevel;
    }

    void Diffuse(float deltaTime);

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<OxygenComponent>(oxygenLevel, newParent);
    }

    constexpr Type GetType() const override { return Type::OXYGEN; }
    constexpr std::optional<std::string> GetInfo() override { return std::format("   + Oxygen Level: {:.0f}", oxygenLevel); }
};

struct OxygenProducerComponent : Component
{
protected:
    float oxygenProduction;

public:
    OxygenProducerComponent(float oxygenProduction, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), oxygenProduction(oxygenProduction) {}

    void ProduceOxygen(float deltaTime) const;

    constexpr float GetOxygenProduction() const { return oxygenProduction; }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<OxygenProducerComponent>(oxygenProduction, newParent);
    }

    constexpr Type GetType() const override { return Type::OXYGEN_PRODUCER; }
    constexpr std::optional<std::string> GetInfo() override { return std::format("   + Oxygen Production: {:.0f}", oxygenProduction); }
};

struct DecorativeComponent : Component
{
protected:
    std::vector<std::shared_ptr<Sprite>> decorativeTiles;

public:
    DecorativeComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    constexpr void AddDecorativeTile(const std::shared_ptr<Sprite> &sprite)
    {
        decorativeTiles.push_back(sprite);
    }

    constexpr const std::vector<std::shared_ptr<Sprite>> &GetDecorativeTiles() const
    {
        return decorativeTiles;
    }

    constexpr void ClearDecorativeTiles()
    {
        decorativeTiles.clear();
    }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<DecorativeComponent>(newParent);
    }

    constexpr Type GetType() const override { return Type::DECORATIVE; }
    constexpr std::optional<std::string> GetInfo() override { return std::nullopt; }
};

struct DoorComponent : Component
{
    enum class MovingState : u_int8_t
    {
        IDLE,
        OPENING,
        CLOSING,
        FORCED_OPEN,
    };

protected:
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
    void Animate(float deltaTime);

    constexpr std::string GetMovementName() const { return EnumToName<MovingState>(movingState); }

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

    constexpr void PingPong()
    {
        if (movingState != MovingState::IDLE)
            return;

        movingState = isOpen ? MovingState::CLOSING : MovingState::OPENING;
    }

    constexpr void KeepClosed()
    {
        if (movingState != MovingState::IDLE)
            return;

        movingState = MovingState::CLOSING;
    }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<DoorComponent>(movingSpeed, isOpen, newParent);
    }

    constexpr Type GetType() const override { return Type::DOOR; }
    constexpr std::optional<std::string> GetInfo() override
    {
        return std::format("   + State: {}\n   + Action: {} ({:.0f}%)", isOpen ? "Open" : "Closed", GetMovementName(), progress * 100.);
    }
};

struct DurabilityComponent : Component
{
protected:
    float maxHitpoints;
    float hitpoints;

public:
    DurabilityComponent(float maxHitpoints, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), maxHitpoints(maxHitpoints), hitpoints(maxHitpoints) {}

    constexpr float GetMaxHitpoints() const { return maxHitpoints; }
    constexpr float GetHitpoints() const { return hitpoints; }

    void SetHitpoints(float newHitpoints);

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<DurabilityComponent>(maxHitpoints, newParent);
    }

    constexpr Type GetType() const override { return Type::DURABILITY; }
    constexpr std::optional<std::string> GetInfo() override { return std::format("   + HP: {:.1f} / {:.1f}", hitpoints, maxHitpoints); }
};

struct RotatableComponent : Component
{
protected:
    Rotation rotation;

public:
    RotatableComponent(Rotation rotation = Rotation::UP, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), rotation(rotation) {}

    constexpr Rotation GetRotation() const { return rotation; }
    constexpr void SetRotation(Rotation newRotation) { rotation = newRotation; }

    constexpr void RotateClockwise()
    {
        // Shift the bit, if it's at the end, reset it to the beginning
        if (rotation == Rotation::LEFT)
            rotation = Rotation::UP;
        else
            rotation = static_cast<Rotation>(static_cast<int>(rotation) << 1);
    }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<RotatableComponent>(rotation, newParent);
    }

    constexpr Type GetType() const override { return Type::ROTATABLE; }
    constexpr std::optional<std::string> GetInfo() override { return std::nullopt; }
};