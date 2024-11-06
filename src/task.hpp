#pragma once
#include <raylib.h>
#include <deque>

struct Crew;
struct Tile;

struct Task
{
    enum class Type : u_int8_t
    {
        NONE,
        MOVE,
        EXTINGUISH,
        REPAIR,
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

    MoveTask(const Vector2Int &position) : targetPosition(position) {}

    void Update(Crew &crew) override;

    constexpr std::string GetActionName() const override { return "Moving"; }
    constexpr Type GetType() const override { return Type::MOVE; }
};

struct ExtinguishTask : Task
{
protected:
    Vector2Int targetPosition;
    float progress;

public:
    ExtinguishTask(const Vector2Int &position) : targetPosition(position), progress(0) {}

    void Update(Crew &crew) override;

    constexpr float GetProgress() const { return progress; }
    constexpr const Vector2Int &GetTargetPosition() const { return targetPosition; }

    constexpr std::string GetActionName() const override { return "Extinguishing"; }
    constexpr Type GetType() const override { return Type::EXTINGUISH; }
};

struct RepairTask : Task
{
protected:
    std::weak_ptr<Tile> _targetTile;

public:
    RepairTask(std::shared_ptr<Tile> tile) : _targetTile(tile) {}

    void Update(Crew &crew) override;

    constexpr std::shared_ptr<Tile> GetTargetTile() const { return _targetTile.lock(); }

    constexpr std::string GetActionName() const override { return "Repairing"; }
    constexpr Type GetType() const override { return Type::REPAIR; }
};