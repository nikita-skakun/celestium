#pragma once
#include <raylib.h>
#include <queue>

struct Task
{
    enum class Type : u_int8_t
    {
        NONE,
        MOVE
    };

    virtual Type GetType() const = 0;
    virtual ~Task() = default;
};

struct MoveTask : Task
{
    Vector2Int targetPosition;
    std::queue<Vector2Int> path;

    MoveTask(const Vector2Int& p) : targetPosition(p) {}

    Type GetType() const override
    {
        return Type::MOVE;
    }
};