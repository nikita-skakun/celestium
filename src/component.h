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

    struct Hash
    {
        std::size_t operator()(const std::shared_ptr<Component> &component) const
        {
            return std::hash<std::string>()(component->GetName());
        }
    };

    // struct Equal
    // {
    //     bool operator()(const std::shared_ptr<Component> &lhs, const std::shared_ptr<Component> &rhs) const
    //     {
    //         return lhs->GetName() == rhs->GetName();
    //     }
    // };
};

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