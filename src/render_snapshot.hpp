#pragma once
#include "utils.hpp"
#include <unordered_map>

struct Crew;
struct Effect;
struct Station;
struct Tile;

struct RenderSnapshot
{
    std::unordered_map<uint64_t, std::shared_ptr<const Crew>> crewList;
    std::shared_ptr<const Station> station;
    double timeSinceFixedUpdate = 0;

    std::vector<std::shared_ptr<const Crew>> GetCrewAtPosition(const Vector2 &pos) const;
};
