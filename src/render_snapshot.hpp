#pragma once
#include "utils.hpp"
#include <unordered_map>

struct Pawn;
struct Effect;
struct Station;
struct Tile;

struct RenderSnapshot
{
    std::unordered_map<uint64_t, std::shared_ptr<const Pawn>> pawnList;
    std::shared_ptr<const Station> station;
    double timeSinceFixedUpdate = 0;

    std::vector<std::shared_ptr<const Pawn>> GetPawnAtPosition(const Vector2 &pos) const;
};
