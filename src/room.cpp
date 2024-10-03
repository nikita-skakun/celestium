#include "room.hpp"
#include "station.hpp"

std::shared_ptr<Room> Room::CreateEmptyRoom(std::shared_ptr<Station> station)
{
    if (!station)
        return nullptr;

    auto room = std::make_shared<Room>(Room(station));
    station->rooms.push_back(room);
    return room;
}
