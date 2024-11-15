#pragma once
#include "utils.hpp"
#include <deque>

struct Crew;
struct Tile;

struct Action
{
    enum class Type : u_int8_t
    {
        NONE,
        MOVE,
        EXTINGUISH,
        REPAIR,
    };

    virtual void Update(const std::shared_ptr<Crew> &crew) = 0;
    virtual constexpr std::string GetActionName() const = 0;
    virtual constexpr Type GetType() const = 0;
    virtual ~Action() = default;
};

struct MoveAction : Action
{
    Vector2Int targetPosition;
    std::deque<Vector2Int> path;

    MoveAction(const Vector2Int &position) : targetPosition(position) {}

    void Update(const std::shared_ptr<Crew> &crew) override;

    constexpr std::string GetActionName() const override { return "Moving"; }
    constexpr Type GetType() const override { return Type::MOVE; }
};

struct ExtinguishAction : Action
{
protected:
    Vector2Int targetPosition;
    float progress;

public:
    ExtinguishAction(const Vector2Int &position) : targetPosition(position), progress(0) {}

    void Update(const std::shared_ptr<Crew> &crew) override;

    constexpr float GetProgress() const { return progress; }
    constexpr const Vector2Int &GetTargetPosition() const { return targetPosition; }

    constexpr std::string GetActionName() const override { return "Extinguishing"; }
    constexpr Type GetType() const override { return Type::EXTINGUISH; }
};

struct RepairAction : Action
{
protected:
    std::weak_ptr<Tile> _targetTile;

public:
    RepairAction(std::shared_ptr<Tile> tile) : _targetTile(tile) {}

    void Update(const std::shared_ptr<Crew> &crew) override;

    std::shared_ptr<Tile> GetTargetTile() const { return _targetTile.lock(); }

    constexpr std::string GetActionName() const override { return "Repairing"; }
    constexpr Type GetType() const override { return Type::REPAIR; }
};