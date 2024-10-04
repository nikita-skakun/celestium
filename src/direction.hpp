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