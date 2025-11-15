#pragma once
#include "utils.hpp"

struct Crew;
struct Tile;
struct Station;
struct Effect;

struct RenderSnapshot
{
    std::vector<std::shared_ptr<Crew>> crewList;
    std::shared_ptr<Station> station;
    std::vector<std::weak_ptr<Crew>> selectedCrewList;

    std::vector<std::shared_ptr<Crew>> GetCrewAtPosition(const Vector2 &pos) const;
};
