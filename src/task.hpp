#pragma once
#include <raylib.h>
#include <deque>

struct Crew;

struct Task
{
    enum class Type : u_int8_t
    {
        NONE,
        MOVE,
        EXTINGUISH,
    };

    virtual void Update(Crew &crew) = 0;
    virtual constexpr std::string GetActionName() const = 0;
    virtual constexpr Type GetType() const = 0;
    virtual ~Task() = default;
};

struct MoveTask : Task
{
    Vector2Int targetPosition;
    std::deque<Vector2Int> path;

    MoveTask(const Vector2Int &p) : targetPosition(p) {}

    void Update(Crew &crew) override;

    constexpr std::string GetActionName() const override { return "Moving"; }
    constexpr Type GetType() const override { return Type::MOVE; }
};

struct ExtinguishTask : Task
{
    Vector2Int targetPosition;
    float progress;

    ExtinguishTask(const Vector2Int &p) : targetPosition(p), progress(0) {}

    void Update(Crew &crew) override;

    constexpr std::string GetActionName() const override { return "Extinguishing"; }
    constexpr Type GetType() const override { return Type::EXTINGUISH; }
};