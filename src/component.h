#pragma once
#include "utils.h"
#include <memory>
#include <vector>

struct DecorativeTile
{

    Vector2Int offset;
    Vector2Int spriteOffset;

    DecorativeTile(const Vector2Int &o, const Vector2Int &sO) : offset(o), spriteOffset(sO) {}
};

struct Component
{
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
    std::string GetName() const override
    {
        return "Walkable";
    }
};

struct SolidComponent : Component
{
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
    BatteryComponent(float c = BATTERY_CHARGE_MAX) : charge(c) {}

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

struct OxygenComponent : Component
{
private:
    float oxygenLevel;

public:
    OxygenComponent(float o = TILE_OXYGEN_MAX) : oxygenLevel(o) {}

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
    std::shared_ptr<OxygenComponent> output;

    OxygenProducerComponent(std::shared_ptr<OxygenComponent> o) : output(o) {}

    std::string GetName() const override
    {
        return "OxygenProducer";
    }
};

struct DecorativeComponent : Component
{
private:
    std::vector<DecorativeTile> decorativeTiles;

public:
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