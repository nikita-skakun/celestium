#pragma once
#include "utils.hpp"
#include "direction.hpp"

struct Tile;
struct PowerGrid;
struct Sprite;

enum class ComponentType : uint8_t
{
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

    virtual ComponentType GetType() const = 0;
    std::shared_ptr<Tile> GetParent() const { return _parent.lock(); }

protected:
    void SetParent(std::shared_ptr<Tile> parent) { _parent = parent; }

public:
    virtual std::optional<std::string> GetInfo() const = 0;
    std::string GetName() const { return EnumToName<ComponentType>(GetType()); }
};

template <typename Derived, ComponentType CType>
struct ComponentBase : Component
{
    using Component::Component;
    ComponentBase(std::shared_ptr<Tile> parent = nullptr) : Component(parent) {}
    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        auto ptr = std::make_shared<Derived>(*static_cast<const Derived *>(this));
        ptr->SetParent(newParent);
        return ptr;
    }
    ComponentType GetType() const override { return CType; }
    static constexpr ComponentType GetStaticType() { return CType; }
};

struct WalkableComponent : ComponentBase<WalkableComponent, ComponentType::WALKABLE>
{
    using ComponentBase::ComponentBase;
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct SolidComponent : ComponentBase<SolidComponent, ComponentType::SOLID>
{
    using ComponentBase::ComponentBase;
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct PowerConnectorComponent : ComponentBase<PowerConnectorComponent, ComponentType::POWER_CONNECTOR>
{
protected:
    std::weak_ptr<PowerGrid> _powerGrid;

public:
    using ComponentBase::ComponentBase;

    void SetPowerGrid(std::shared_ptr<PowerGrid> powerGrid) { _powerGrid = powerGrid; }
    std::shared_ptr<PowerGrid> GetPowerGrid() const { return _powerGrid.lock(); }

    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct BatteryComponent : ComponentBase<BatteryComponent, ComponentType::BATTERY>
{
protected:
    float charge;
    float maxCharge;
    float deltaCharge;

public:
    BatteryComponent(float maxCharge, std::shared_ptr<Tile> parent = nullptr)
        : ComponentBase(parent), charge(maxCharge), maxCharge(maxCharge), deltaCharge(0.f) {}

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

    void ResetDeltaCharge() { deltaCharge = 0.f; }
    void AccumulateDeltaCharge(float amount) { deltaCharge += amount; }

    std::optional<std::string> GetInfo() const override { return "   + Charge Level: " + ToString(charge, 0) + " / " + ToString(maxCharge, 0) + " (" +
                                                                 (deltaCharge > 0 ? "+" : "") + ToString(deltaCharge, 0) + "/s)"; }
};

struct PowerConsumerComponent : ComponentBase<PowerConsumerComponent, ComponentType::POWER_CONSUMER>
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
    using ComponentBase::ComponentBase;

    PowerConsumerComponent(float powerConsumption, PowerPriority powerPriority, std::shared_ptr<Tile> parent = nullptr)
        : ComponentBase(parent), isActive(false), powerConsumption(std::max(powerConsumption, 0.f)), powerPriority(powerPriority) {}

    bool IsActive() const { return isActive; }
    void SetActive(bool active) { isActive = active; }

    float GetPowerConsumption() const { return powerConsumption; }

    PowerPriority GetPowerPriority() const { return powerPriority; }
    void SetPowerPriority(PowerPriority priority) { powerPriority = priority; }

    std::optional<std::string> GetInfo() const override
    {
        return "   + Power Priority: " + std::string(magic_enum::enum_name(powerPriority)) +
               "\n   + Power Consumption: " + ToString(powerConsumption, 0);
    }
};

struct PowerProducerComponent : ComponentBase<PowerProducerComponent, ComponentType::POWER_PRODUCER>
{
protected:
    float powerProduction;

public:
    using ComponentBase::ComponentBase;

    PowerProducerComponent(float powerProduction, std::shared_ptr<Tile> parent = nullptr)
        : ComponentBase(parent), powerProduction(std::max(powerProduction, 0.f)) {}

    virtual float GetPowerProduction() const { return powerProduction; }

    virtual std::optional<std::string> GetInfo() const override { return "   + Power Production: " + ToString(GetPowerProduction(), 0); }
};

struct SolarPanelComponent : public PowerProducerComponent
{
    SolarPanelComponent(float powerProduction, std::shared_ptr<Tile> parent = nullptr) : PowerProducerComponent(powerProduction, parent) {}

    std::shared_ptr<Component> Clone(std::shared_ptr<Tile> newParent) const override
    {
        return std::make_shared<SolarPanelComponent>(powerProduction, newParent);
    }

    ComponentType GetType() const override { return ComponentType::SOLAR_PANEL; }
    std::optional<std::string> GetInfo() const override { return "   + Power Production: " + ToString(GetPowerProduction(), 0); }

    float GetPowerProduction() const override;
};

struct OxygenComponent : ComponentBase<OxygenComponent, ComponentType::OXYGEN>
{
protected:
    float oxygenLevel;

public:
    using ComponentBase::ComponentBase;

    OxygenComponent(float startOxygenLevel = TILE_OXYGEN_MAX, std::shared_ptr<Tile> parent = nullptr)
        : ComponentBase(parent), oxygenLevel(startOxygenLevel) {}

    void SetOxygenLevel(float oxygen)
    {
        oxygenLevel = oxygen;
    }

    float &GetOxygenLevel()
    {
        return oxygenLevel;
    }

    void Diffuse(float deltaTime);

    std::optional<std::string> GetInfo() const override { return "   + Oxygen Level: " + ToString(oxygenLevel, 0); }
};

struct OxygenProducerComponent : ComponentBase<OxygenProducerComponent, ComponentType::OXYGEN_PRODUCER>
{
protected:
    float oxygenProduction;

public:
    using ComponentBase::ComponentBase;

    OxygenProducerComponent(float oxygenProduction, std::shared_ptr<Tile> parent = nullptr)
        : ComponentBase(parent), oxygenProduction(oxygenProduction) {}

    void ProduceOxygen(float deltaTime) const;

    float GetOxygenProduction() const { return oxygenProduction; }

    std::optional<std::string> GetInfo() const override { return "   + Oxygen Production: " + ToString(oxygenProduction, 0); }
};

struct DecorativeComponent : ComponentBase<DecorativeComponent, ComponentType::DECORATIVE>
{
protected:
    std::vector<std::shared_ptr<Sprite>> decorativeTiles;

public:
    using ComponentBase::ComponentBase;

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

    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct DoorComponent : ComponentBase<DoorComponent, ComponentType::DOOR>
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
    using ComponentBase::ComponentBase;

    DoorComponent(float movingSpeed = 1.f, bool openState = false, std::shared_ptr<Tile> parent = nullptr)
        : ComponentBase(parent), movingSpeed(std::max(movingSpeed, 0.f)), isOpen(openState),
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

    std::optional<std::string> GetInfo() const override
    {
        return std::string("   + State: ") + (isOpen ? "Open" : "Closed") + "\n   + Action: " + GetMovementName() + "(" + ToString(progress * 100.f, 0) + "%)";
    }
};

struct DurabilityComponent : ComponentBase<DurabilityComponent, ComponentType::DURABILITY>
{
protected:
    float maxHitpoints;
    float hitpoints;

public:
    using ComponentBase::ComponentBase;

    DurabilityComponent(float maxHitpoints, std::shared_ptr<Tile> parent = nullptr)
        : ComponentBase(parent), maxHitpoints(maxHitpoints), hitpoints(maxHitpoints) {}

    float GetMaxHitpoints() const { return maxHitpoints; }
    float GetHitpoints() const { return hitpoints; }

    void SetHitpoints(float newHitpoints);

    std::optional<std::string> GetInfo() const override { return "   + HP: " + ToString(hitpoints, 1) + " / " + ToString(maxHitpoints, 1); }
};

struct RotatableComponent : ComponentBase<RotatableComponent, ComponentType::ROTATABLE>
{
protected:
    Rotation rotation;

public:
    using ComponentBase::ComponentBase;

    RotatableComponent(Rotation rotation = Rotation::UP, std::shared_ptr<Tile> parent = nullptr)
        : ComponentBase(parent), rotation(rotation) {}

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

    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};

struct StructureComponent : ComponentBase<StructureComponent, ComponentType::STRUCTURE>
{
    using ComponentBase::ComponentBase;
    std::optional<std::string> GetInfo() const override { return std::nullopt; }
};
