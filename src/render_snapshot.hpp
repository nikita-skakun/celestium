#pragma once
#include "utils.hpp"

struct Crew;
struct Effect;
struct Station;
struct Tile;

struct RenderSnapshot
{
    std::vector<std::shared_ptr<Crew>> crewList;
    std::shared_ptr<Station> station;
    std::vector<std::weak_ptr<Crew>> selectedCrewList;

    std::vector<std::shared_ptr<Crew>> GetCrewAtPosition(const Vector2 &pos) const;
};
