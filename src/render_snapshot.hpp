#pragma once

#include "env_effect.hpp"
#include "crew.hpp"

struct RenderSnapshot
{
    std::vector<std::shared_ptr<Crew>> crewList;
    std::unordered_map<Vector2Int, std::vector<std::shared_ptr<Tile>>> tileMap;
    std::vector<std::shared_ptr<Effect>> effects;
    std::vector<std::weak_ptr<Crew>> selectedCrewList;
    // Add other renderable game state as needed

    std::vector<std::shared_ptr<Crew>> GetCrewAtPosition(const Vector2 &pos, float radius = CREW_RADIUS / TILE_SIZE) const;
    std::vector<std::shared_ptr<Tile>> GetTilesAtPosition(const Vector2Int &pos) const;
    std::vector<std::shared_ptr<Effect>> GetEffectsAtPosition(const Vector2Int &pos) const;
};
