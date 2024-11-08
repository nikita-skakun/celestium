#pragma once
#include "utils.hpp"

enum class Direction : uint8_t
{
    NONE = 0,
    N = 1 << 0,
    E = 1 << 1,
    S = 1 << 2,
    W = 1 << 3,
};

template <>
struct magic_enum::customize::enum_range<Direction>
{
    static constexpr bool is_flags = true;
};

constexpr Vector2Int DirectionToVector2Int(Direction direction)
{
    Vector2Int v = Vector2Int(0, 0);

    if (magic_enum::enum_flags_test<Direction>(direction, Direction::N))
        v += Vector2Int(0, -1);
    if (magic_enum::enum_flags_test<Direction>(direction, Direction::E))
        v += Vector2Int(1, 0);
    if (magic_enum::enum_flags_test<Direction>(direction, Direction::S))
        v += Vector2Int(0, 1);
    if (magic_enum::enum_flags_test<Direction>(direction, Direction::W))
        v += Vector2Int(-1, 0);

    return v;
}

constexpr std::array<Direction, 4> CARDINAL_DIRECTIONS = {Direction::N, Direction::E, Direction::S, Direction::W};

enum class Rotation : u_int8_t
{
    UP = 1 << 0,    // No rotation
    RIGHT = 1 << 1, // 90 degrees clockwise
    DOWN = 1 << 2,  // 180 degrees
    LEFT = 1 << 3,  // 270 degrees clockwise
};

template <>
struct magic_enum::customize::enum_range<Rotation>
{
    static constexpr bool is_flags = true;
};

constexpr float RotationToAngle(Rotation rotation)
{
    switch (rotation)
    {
    case Rotation::RIGHT:
        return 90.f;
    case Rotation::DOWN:
        return 180.f;
    case Rotation::LEFT:
        return 270.f;
    default:
        return 0.f;
    }
}

constexpr Vector2 OffsetWithRotation(Rotation rotation, const Vector2 &offset)
{
    switch (rotation)
    {
    case Rotation::RIGHT:
        return Vector2(-offset.y, offset.x);
    case Rotation::DOWN:
        return Vector2(-offset.x, -offset.y);
    case Rotation::LEFT:
        return Vector2(offset.y, -offset.x);
    default:
        return offset;
    }
}

constexpr Vector2Int OffsetWithRotation(Rotation rotation, const Vector2Int &offset)
{
    switch (rotation)
    {
    case Rotation::RIGHT:
        return Vector2Int(-offset.y, offset.x);
    case Rotation::DOWN:
        return Vector2Int(-offset.x, -offset.y);
    case Rotation::LEFT:
        return Vector2Int(offset.y, -offset.x);
    default:
        return offset;
    }
}