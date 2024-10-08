#pragma once
#include "direction.hpp"
#include "room.hpp"
#include <map>
#include <unordered_map>

struct Station
{
    std::vector<std::shared_ptr<Room>> rooms;
    std::unordered_map<Vector2Int, std::vector<std::shared_ptr<Tile>>> tileMap;

public:
    std::shared_ptr<Tile> GetTileAtPosition(const Vector2Int &pos, TileDef::Height height = TileDef::Height::NONE) const;
    const std::vector<std::shared_ptr<Tile>> &GetTilesAtPosition(const Vector2Int &pos) const;
    std::string GetTileIdAtPosition(const Vector2Int &pos, TileDef::Height height = TileDef::Height::NONE) const;
    void UpdateSpriteOffsets() const;
    bool CanPath(const Vector2Int &pos) const;

    constexpr bool CheckAdjacentTile(const Vector2Int &tilePos, const std::string &tileId, Direction direction) const
    {
        return GetTileIdAtPosition(tilePos + DirectionToVector2Int(direction)) == tileId;
    }

    template <typename T>
    std::shared_ptr<Tile> GetTileWithComponentAtPosition(const Vector2Int &pos) const
    {
        auto posIt = tileMap.find(pos);
        if (posIt == tileMap.end())
            return nullptr;

        for (const std::shared_ptr<Tile> &tile : posIt->second)
        {
            if (tile->HasComponent<T>())
                return tile;
        }

        return nullptr;
    }

    template <typename T>
    std::vector<std::shared_ptr<Tile>> GetTilesWithComponentAtPosition(const Vector2Int &pos) const
    {
        std::vector<std::shared_ptr<Tile>> result;

        auto posIt = tileMap.find(pos);
        if (posIt == tileMap.end())
            return result;

        for (const std::shared_ptr<Tile> &tile : posIt->second)
        {
            if (tile->HasComponent<T>())
                result.push_back(tile);
        }

        return result;
    }
};

std::shared_ptr<Room> CreateRectRoom(const Vector2Int &pos, const Vector2Int &size, std::shared_ptr<Station> station);
std::shared_ptr<Room> CreateHorizontalCorridor(const Vector2Int &startPos, int length, int width, std::shared_ptr<Station> station);
std::shared_ptr<Station> CreateStation();