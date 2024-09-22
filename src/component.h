#pragma once
#include "tile.h"

struct Component
{
    std::vector<std::shared_ptr<Tile>> tiles;
    std::shared_ptr<Room> room;
    std::shared_ptr<Station> station;

    bool isInternal;
};