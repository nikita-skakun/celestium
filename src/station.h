#pragma once
#include "logging.h"
#include "room.h"
#include <map>
#include <unordered_map>

struct Station
{
    std::vector<std::shared_ptr<Tile>> tiles;
    std::vector<std::shared_ptr<Room>> rooms;
    std::unordered_map<Vector2Int, std::map<Tile::Height, std::shared_ptr<Tile>>, Vector2Int::Hash> tileMap;

    std::shared_ptr<Tile> GetTileAtPosition(const Vector2Int &pos, Tile::Height height = Tile::Height::NONE) const;
    std::vector<std::shared_ptr<Tile>> GetTilesAtPosition(const Vector2Int &pos) const;
    void UpdateSpriteOffsets();
};

std::shared_ptr<Tile> CreateTile(Tile::ID id, const Vector2Int &position, std::shared_ptr<Station> station, std::shared_ptr<Room> room);
std::shared_ptr<Room> CreateEmptyRoom(std::shared_ptr<Station> station);
std::shared_ptr<Room> CreateRectRoom(const Vector2Int &pos, const Vector2Int &size, std::shared_ptr<Station> station);
std::shared_ptr<Room> CreateHorizontalCorridor(const Vector2Int &startPos, int length, std::shared_ptr<Station> station);
std::shared_ptr<Station> CreateStation();