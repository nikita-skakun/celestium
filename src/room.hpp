#pragma once
#include "tile.hpp"

struct Room
{
    std::vector<std::weak_ptr<Tile>> tiles;
    std::weak_ptr<Station> station;

    Room(const std::shared_ptr<Station> &s) : station(s) {}
};