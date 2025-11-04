#pragma once
#include "utils.hpp"
#include "direction.hpp"

struct Tile;
struct PowerGrid;
struct Sprite;

enum class ComponentType : uint8_t
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

struct Component : public std::enable_shared_from_this<Component>
{
protected:
    std::weak_ptr<Tile> _parent;

public:
    Component(std::shared_ptr<Tile> parent = nullptr) : _parent(parent) {}

    virtual std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const = 0;
    virtual ~Component() = default;

    virtual ComponentType GetType() const { return ComponentType::NONE; }
    std::shared_ptr<Tile> GetParent() const { return _parent.lock(); }

    virtual std::optional<std::string> GetInfo() const = 0;
    std::string GetName() const { return EnumToName<ComponentType>(GetType()); }

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
}

struct WalkableComponent : Component
{
    WalkableComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<WalkableComponent>(newParent);
    }

    ComponentType GetType() const override { return ComponentType::WALKABLE; }
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct SolidComponent : Component
{
    SolidComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<SolidComponent>(newParent);
    }

    ComponentType GetType() const override { return ComponentType::SOLID; }
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

    void SetPowerGrid(std::shared_ptr<PowerGrid> powerGrid) { _powerGrid = powerGrid; }
    std::shared_ptr<PowerGrid> GetPowerGrid() const { return _powerGrid.lock(); }

    ComponentType GetType() const override { return ComponentType::POWER_CONNECTOR; }
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

    float GetMaxChargeLevel() const { return maxCharge; }
    float GetChargeLevel() const { return charge; }

    float AddCharge(float amount)
    {
        if (charge >= maxCharge)
            return 0.f;

        float added = std::min(maxCharge - charge, amount);
        charge += added;
        return added;
    }

    float Drain(float amount)
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

    ComponentType GetType() const override { return ComponentType::BATTERY; }
    std::optional<std::string> GetInfo() const override { return "   + Charge Level: " + ToString(charge, 0) + " / " + ToString(maxCharge, 0); }
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

    bool IsActive() const { return isActive; }
    void SetActive(bool active) { isActive = active; }

    float GetPowerConsumption() const { return powerConsumption; }

    PowerPriority GetPowerPriority() const { return powerPriority; }
    void SetPowerPriority(PowerPriority priority) { powerPriority = priority; }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<PowerConsumerComponent>(powerConsumption, powerPriority, newParent);
    }

    ComponentType GetType() const override { return ComponentType::POWER_CONSUMER; }
    std::optional<std::string> GetInfo() const override
    {
        return "   + Power Priority: " + std::string(magic_enum::enum_name(powerPriority)) +
               "\n   + Power Consumption: " + ToString(powerConsumption, 0);
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

    float GetPowerProduction() const { return powerProduction; }
    float GetAvailablePower() const { return availablePower; }

    void UseAvailablePower(float usedPower) { availablePower -= usedPower; }
    void ProducePower(float deltaTime) { availablePower = GetPowerProduction() * deltaTime; }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<PowerProducerComponent>(powerProduction, newParent);
    }

    ComponentType GetType() const override { return ComponentType::POWER_PRODUCER; }
    std::optional<std::string> GetInfo() const override { return "   + Power Production: " + ToString(powerProduction, 0); }
};

// TODO: Implement occlusions for solar panels (if indoor)
struct SolarPanelComponent : Component
{
    SolarPanelComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<SolarPanelComponent>(newParent);
    }

    ComponentType GetType() const override { return ComponentType::SOLAR_PANEL; }
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct OxygenComponent : Component
{
protected:
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

    void Diffuse(float deltaTime);

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<OxygenComponent>(oxygenLevel, newParent);
    }

    ComponentType GetType() const override { return ComponentType::OXYGEN; }
    std::optional<std::string> GetInfo() const override { return "   + Oxygen Level: " + ToString(oxygenLevel, 0); }
};

struct OxygenProducerComponent : Component
{
protected:
    float oxygenProduction;

public:
    OxygenProducerComponent(float oxygenProduction, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), oxygenProduction(oxygenProduction) {}

    void ProduceOxygen(float deltaTime) const;

    float GetOxygenProduction() const { return oxygenProduction; }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<OxygenProducerComponent>(oxygenProduction, newParent);
    }

    ComponentType GetType() const override { return ComponentType::OXYGEN_PRODUCER; }
    std::optional<std::string> GetInfo() const override { return "   + Oxygen Production: " + ToString(oxygenProduction, 0); }
};

struct DecorativeComponent : Component
{
protected:
    std::vector<std::shared_ptr<Sprite>> decorativeTiles;

public:
    DecorativeComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    void AddDecorativeTile(const std::shared_ptr<Sprite> &sprite)
    {
        decorativeTiles.push_back(sprite);
    }

    const std::vector<std::shared_ptr<Sprite>> &GetDecorativeTiles() const
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

    ComponentType GetType() const override { return ComponentType::DECORATIVE; }
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

    bool IsOpen() const { return isOpen; }
    MovingState GetMovingState() const { return movingState; }
    float GetProgress() const { return progress; }
    void SetProgress(float newProgress) { progress = std::clamp(newProgress, 0.f, 1.f); }

    void SetOpenState(bool openState);
    void Animate(float deltaTime);

    std::string GetMovementName() const { return EnumToName<MovingState>(movingState); }

    void SetMovingState(MovingState newMovingState)
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

    void PingPong()
    {
        if (movingState != MovingState::IDLE)
            return;

        movingState = isOpen ? MovingState::CLOSING : MovingState::OPENING;
    }

    void KeepClosed()
    {
        if (movingState != MovingState::IDLE)
            return;

        movingState = MovingState::CLOSING;
    }

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<DoorComponent>(movingSpeed, isOpen, newParent);
    }

    ComponentType GetType() const override { return ComponentType::DOOR; }
    std::optional<std::string> GetInfo() const override
    {
        return std::string("   + State: ") + (isOpen ? "Open" : "Closed") + "\n   + Action: " + GetMovementName() + "(" + ToString(progress * 100.f, 0) + "%)";
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

    float GetMaxHitpoints() const { return maxHitpoints; }
    float GetHitpoints() const { return hitpoints; }

    void SetHitpoints(float newHitpoints);

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<DurabilityComponent>(maxHitpoints, newParent);
    }

    ComponentType GetType() const override { return ComponentType::DURABILITY; }
    std::optional<std::string> GetInfo() const override { return "   + HP: " + ToString(hitpoints, 1) + " / " + ToString(maxHitpoints, 1); }
};

struct RotatableComponent : Component
{
protected:
    Rotation rotation;

public:
    RotatableComponent(Rotation rotation = Rotation::UP, std::shared_ptr<Tile> parent = nullptr)
        : Component(parent), rotation(rotation) {}

    Rotation GetRotation() const { return rotation; }
    void SetRotation(Rotation newRotation) { rotation = newRotation; }

    void RotateClockwise()
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

    ComponentType GetType() const override { return ComponentType::ROTATABLE; }
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct StructureComponent : Component
{
    StructureComponent(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<StructureComponent>(newParent);
    }

    ComponentType GetType() const override { return ComponentType::STRUCTURE; }
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};
