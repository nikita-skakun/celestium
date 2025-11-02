#pragma once
#include "sprite.hpp"

struct Tile;
struct PowerGrid;

struct Component : public std::enable_shared_from_this<Component>
{
protected:
    std::weak_ptr<Tile> _parent;

public:
    enum class Type : uint8_t
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
        STRUCTURE,
    };

    Component(std::shared_ptr<Tile> parent = nullptr) : _parent(parent) {}

    virtual std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const = 0;
    virtual ~Component() = default;

    constexpr virtual Type GetType() const { return Type::NONE; }
    constexpr std::shared_ptr<Tile> GetParent() const { return _parent.lock(); }
    
    virtual std::optional<std::string> GetInfo() const = 0;
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
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct SolidComponent : Component
{
    SolidComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<SolidComponent>(newParent);
    }

    constexpr Type GetType() const override { return Type::SOLID; }
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct PowerConnectorComponent : Component
{
protected:
    std::weak_ptr<PowerGrid> _powerGrid;

public:
    PowerConnectorComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<PowerConnectorComponent>(newParent);
    }

    constexpr void SetPowerGrid(std::shared_ptr<PowerGrid> powerGrid) { _powerGrid = powerGrid; }
    constexpr std::shared_ptr<PowerGrid> GetPowerGrid() const { return _powerGrid.lock(); }

    constexpr Type GetType() const override { return Type::POWER_CONNECTOR; }
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct BatteryComponent : Component
{
protected:
    float charge;
    float maxCharge;

public:
    BatteryComponent(float maxCharge, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), charge(maxCharge), maxCharge(maxCharge) {}

    constexpr float GetMaxChargeLevel() const { return maxCharge; }
    constexpr float GetChargeLevel() const { return charge; }

    constexpr float AddCharge(float amount)
    {
        if (charge >= maxCharge)
            return 0.f;

        float added = std::min(maxCharge - charge, amount);
        charge += added;
        return added;
    }

    constexpr float Drain(float amount)
    {
        if (charge <= 0.f)
            return 0.f;

        float drained = std::min(charge, amount);
        charge -= drained;
        return drained;
    }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<BatteryComponent>(charge, newParent);
    }

    constexpr Type GetType() const override { return Type::BATTERY; }
    std::optional<std::string> GetInfo() const override { return std::format("   + Charge Level: {:.0f} / {:.0f}", charge, maxCharge); }
};

struct PowerConsumerComponent : Component
{
public:
    enum class PowerPriority : uint8_t
    {
        CRITICAL = 0, // Life support, etc.
        HIGH = 1,     // Main systems
        MEDIUM = 2,   // General equipment
        LOW = 3,      // Convenience, decoration
        OFFLINE = 255 // Manually turned off
    };

protected:
    bool isActive;
    float powerConsumption;
    PowerPriority powerPriority;

public:
    PowerConsumerComponent(float powerConsumption, PowerPriority powerPriority, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), isActive(false), powerConsumption(std::max(powerConsumption, 0.f)), powerPriority(powerPriority) {}

    constexpr bool IsActive() const { return isActive; }
    constexpr void SetActive(bool active) { isActive = active; }

    constexpr float GetPowerConsumption() const { return powerConsumption; }

    constexpr PowerPriority GetPowerPriority() const { return powerPriority; }
    constexpr void SetPowerPriority(PowerPriority priority) { powerPriority = priority; }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<PowerConsumerComponent>(powerConsumption, powerPriority, newParent);
    }

    constexpr Type GetType() const override { return Type::POWER_CONSUMER; }
    std::optional<std::string> GetInfo() const override
    {
        return std::format("   + Power Priority: {}\n   + Power Consumption: {:.0f}", magic_enum::enum_name(powerPriority), powerConsumption);
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

    constexpr float GetPowerProduction() const { return powerProduction; }
    constexpr float GetAvailablePower() const { return availablePower; }

    constexpr void UseAvailablePower(float usedPower) { availablePower -= usedPower; }
    constexpr void ProducePower(float deltaTime) { availablePower = GetPowerProduction() * deltaTime; }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<PowerProducerComponent>(powerProduction, newParent);
    }

    constexpr Type GetType() const override { return Type::POWER_PRODUCER; }
    std::optional<std::string> GetInfo() const override { return std::format("   + Power Production: {:.0f}", powerProduction); }
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
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
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
    std::optional<std::string> GetInfo() const override { return std::format("   + Oxygen Level: {:.0f}", oxygenLevel); }
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
    std::optional<std::string> GetInfo() const override { return std::format("   + Oxygen Production: {:.0f}", oxygenProduction); }
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
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct DoorComponent : Component
{
    enum class MovingState : uint8_t
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
    std::optional<std::string> GetInfo() const override
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
    std::optional<std::string> GetInfo() const override { return std::format("   + HP: {:.1f} / {:.1f}", hitpoints, maxHitpoints); }
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
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct StructureComponent : Component
{
    StructureComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<StructureComponent>(newParent);
    }

    constexpr Type GetType() const override { return Type::STRUCTURE; }
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};
