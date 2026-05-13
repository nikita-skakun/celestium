#pragma once
#include "utils.hpp"
#include <deque>

struct Crew;
struct Tile;
struct PlannedTask;

struct Action
{
    enum class Type : u_int8_t
    {
        NONE,
        MOVE,
        EXTINGUISH,
        REPAIR,
        CONSTRUCTION,
    };

    virtual bool Update(const std::shared_ptr<Crew> &crew) = 0;
    virtual std::string GetActionName() const = 0;
    virtual Type GetType() const = 0;
    virtual ~Action() = default;
};

struct MoveAction : Action
{
    Vector2 targetPosition;
    std::deque<Vector2> path;

    explicit MoveAction(const Vector2 &position) : targetPosition(position) {}

    bool Update(const std::shared_ptr<Crew> &crew) override;

    std::string GetActionName() const override { return "Moving"; }
    Type GetType() const override { return Type::MOVE; }
};

struct ExtinguishAction : Action
{
protected:
    Vector2Int targetPosition;
    float progress;

public:
    explicit ExtinguishAction(const Vector2Int &position) : targetPosition(position), progress(0) {}

    bool Update(const std::shared_ptr<Crew> &crew) override;

    float GetProgress() const { return progress; }
    const Vector2Int &GetTargetPosition() const { return targetPosition; }

    std::string GetActionName() const override { return "Extinguishing"; }
    Type GetType() const override { return Type::EXTINGUISH; }
};

struct RepairAction : Action
{
protected:
    std::weak_ptr<Tile> _targetTile;

public:
    explicit RepairAction(std::shared_ptr<Tile> tile) : _targetTile(tile) {}

    bool Update(const std::shared_ptr<Crew> &crew) override;

    std::shared_ptr<Tile> GetTargetTile() const { return _targetTile.lock(); }

    std::string GetActionName() const override { return "Repairing"; }
    Type GetType() const override { return Type::REPAIR; }
};

struct ConstructionAction : Action
{
protected:
    std::weak_ptr<PlannedTask> _task;

public:
    explicit ConstructionAction(std::shared_ptr<PlannedTask> task) : _task(task) {}

    bool Update(const std::shared_ptr<Crew> &crew) override;

    std::weak_ptr<PlannedTask> GetPlanned() const { return _task; }

    std::string GetActionName() const override { return "Constructing"; }
    Type GetType() const override { return Type::CONSTRUCTION; }
};