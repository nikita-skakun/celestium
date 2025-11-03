#pragma once
#include "crew.hpp"
#include "tile.hpp"
#include "env_effect.hpp"

struct RenderSnapshot
{
    std::vector<std::shared_ptr<Crew>> crewList;
    std::unordered_map<Vector2Int, std::vector<std::shared_ptr<Tile>>> tileMap;
    std::vector<std::shared_ptr<Effect>> effects;
    std::vector<std::weak_ptr<Crew>> selectedCrewList;
    // Add other renderable game state as needed

    // Get all crew at a world position (within radius)
    std::vector<std::shared_ptr<Crew>> GetCrewAtPosition(const Vector2 &pos, float radius = CREW_RADIUS / TILE_SIZE) const
    {
        std::vector<std::shared_ptr<Crew>> result;
        float r2 = radius * radius;
        for (const auto &crew : crewList)
        {
            if (crew && Vector2DistanceSq(pos, crew->GetPosition()) <= r2)
                result.push_back(crew);
        }
        return result;
    }

    // Get all tiles at a tile position
    std::vector<std::shared_ptr<Tile>> GetTilesAtPosition(const Vector2Int &pos) const
    {
        auto it = tileMap.find(pos);
        if (it == tileMap.end())
            return {};
        return it->second;
    }

    // Get all effects at a tile position
    std::vector<std::shared_ptr<Effect>> GetEffectsAtPosition(const Vector2Int &pos) const
    {
        std::vector<std::shared_ptr<Effect>> result;
        for (const auto &effect : effects)
        {
            if (effect && effect->GetPosition() == pos)
                result.push_back(effect);
        }
        return result;
    }
};
