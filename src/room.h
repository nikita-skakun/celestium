#pragma once
#include "component.h"

struct Room
{
    std::vector<std::shared_ptr<Tile>> tiles;
    std::vector<std::shared_ptr<Component>> components;
    std::shared_ptr<Station> station;

    Room(const std::shared_ptr<Station> &s)
        : station(s)
    {
    }
};