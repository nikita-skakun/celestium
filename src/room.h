#pragma once
#include "tile.h"

struct Room
{
    std::vector<std::shared_ptr<Tile>> tiles;
    std::shared_ptr<Station> station;

    Room(const std::shared_ptr<Station> &s)
        : station(s)
    {
    }
};