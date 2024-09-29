#pragma once
#include "tile.hpp"

struct Room
{
    std::vector<std::shared_ptr<Tile>> tiles;
    std::shared_ptr<Station> station;

    Room(const std::shared_ptr<Station> &s)
        : station(s)
    {
    }
};