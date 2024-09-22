#pragma once
#include "room.h"
#include <unordered_map>

struct Station
{
    std::vector<std::shared_ptr<Tile>> tiles;
    std::vector<std::shared_ptr<Component>> components;
    std::vector<std::shared_ptr<Room>> rooms;
    std::unordered_map<Vector2, std::shared_ptr<Tile>, Vector2Hash, Vector2Equal> tileMap;

    std::shared_ptr<Tile> GetTileAtPosition(const Vector2 &pos) const;
    void UpdateSpriteOffsets();
};

std::shared_ptr<Tile> CreateTile(Tile::Type type, const Vector2 &position, Tile::ID id, std::shared_ptr<Station> station, std::shared_ptr<Room> room, std::shared_ptr<Component> component);
std::shared_ptr<Room> CreateEmptyRoom(std::shared_ptr<Station> station);
std::shared_ptr<Room> CreateRectRoom(const Vector2 &pos, const Vector2Int &size, std::shared_ptr<Station> station);
std::shared_ptr<Room> CreateHorizontalCorridor(const Vector2 &startPos, int length, std::shared_ptr<Station> station);
std::shared_ptr<Station> CreateStation();