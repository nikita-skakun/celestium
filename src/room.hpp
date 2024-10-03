#pragma once
#include "tile.hpp"

struct Room
{
    std::vector<std::weak_ptr<Tile>> tiles;
    std::weak_ptr<Station> station;

private:
    Room(std::shared_ptr<Station> station) : station(station) {}

public:
    static std::shared_ptr<Room> CreateEmptyRoom(std::shared_ptr<Station> station);
};
