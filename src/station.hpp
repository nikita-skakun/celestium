#pragma once
#include "room.hpp"
#include <map>
#include <unordered_map>

struct Station
{
    std::vector<std::shared_ptr<Tile>> tiles;
    std::vector<std::shared_ptr<Room>> rooms;
    std::unordered_map<Vector2Int, std::map<TileDef::Height, std::shared_ptr<Tile>>, Vector2Int::Hash> tileMap;

    std::shared_ptr<Tile> GetTileAtPosition(const Vector2Int &pos, TileDef::Height height = TileDef::Height::NONE) const;
    std::vector<std::shared_ptr<Tile>> GetTilesAtPosition(const Vector2Int &pos) const;
    void UpdateSpriteOffsets();

    template <typename T>
    std::shared_ptr<Tile> GetTileWithComponentAtPosition(const Vector2Int &pos) const
    {
        auto posIt = tileMap.find(pos);
        if (posIt == tileMap.end())
            return nullptr;

        for (const auto &pair : posIt->second)
        {
            if (pair.second->HasComponent<T>())
                return pair.second;
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

        for (const auto &pair : posIt->second)
        {
            if (pair.second->HasComponent<T>())
                result.push_back(pair.second);
        }

        return result;
    }
};

std::shared_ptr<Room> CreateRectRoom(const Vector2Int &pos, const Vector2Int &size, std::shared_ptr<Station> station);
std::shared_ptr<Room> CreateHorizontalCorridor(const Vector2Int &startPos, int length, int width, std::shared_ptr<Station> station);
std::shared_ptr<Station> CreateStation();