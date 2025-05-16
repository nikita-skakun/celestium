#pragma once
#include "env_effect.hpp"
#include "power_grid.hpp"
#include "tile.hpp"
#include <unordered_map>

struct Station : public std::enable_shared_from_this<Station>
{
    std::unordered_map<Vector2Int, std::vector<std::shared_ptr<Tile>>> tileMap;
    std::vector<std::shared_ptr<Effect>> effects;
    std::vector<std::shared_ptr<PowerGrid>> powerGrids;

public:
    std::shared_ptr<Tile> GetTileAtPosition(const Vector2Int &pos, TileDef::Height height = TileDef::Height::NONE) const;
    const std::vector<std::shared_ptr<Tile>> &GetTilesAtPosition(const Vector2Int &pos) const;
    std::vector<std::shared_ptr<Tile>> GetDecorativeTilesAtPosition(const Vector2Int &pos) const;
    std::vector<std::shared_ptr<Tile>> GetAllTilesAtPosition(const Vector2Int &pos) const;
    std::shared_ptr<Tile> GetTileWithHeightAtPosition(const Vector2Int &pos, TileDef::Height height) const;
    std::vector<std::shared_ptr<Tile>> GetTilesWithHeightAtPosition(const Vector2Int &pos, TileDef::Height height) const;
    std::string GetTileIdAtPosition(const Vector2Int &pos, TileDef::Height height = TileDef::Height::NONE) const;
    SpriteCondition GetSpriteConditionForTile(const std::shared_ptr<Tile> &tile) const;
    void UpdateSpriteOffsets() const;
    bool IsPositionPathable(const Vector2Int &pos) const;
    bool IsDoorFullyOpenAtPos(const Vector2Int &pos) const;

    constexpr bool CheckAdjacentTile(const Vector2Int &tilePos, const std::string &tileId, Direction direction) const
    {
        return GetTileIdAtPosition(tilePos + DirectionToVector2Int(direction)) == tileId;
    }

    std::vector<std::shared_ptr<Effect>> GetEffectsAtPosition(const Vector2Int &pos) const;
    void RemoveEffect(const std::shared_ptr<Effect> &effect);

    template <typename T>
    std::shared_ptr<Effect> GetEffectOfTypeAtPosition(const Vector2Int &pos) const
    {
        auto effectsAtPos = GetEffectsAtPosition(pos);
        for (const auto &effect : effectsAtPos)
        {
            if (std::dynamic_pointer_cast<T>(effect))
                return effect;
        }
        return nullptr;
    }

    template <typename T>
    std::vector<std::shared_ptr<Effect>> GetEffectsOfTypeAtPosition(const Vector2Int &pos) const
    {
        auto effectsAtPos = GetEffectsAtPosition(pos);
        std::vector<std::shared_ptr<Effect>> foundEffects;
        for (const auto &effect : effectsAtPos)
        {
            if (std::dynamic_pointer_cast<T>(effect))
                foundEffects.push_back(effect);
        }
        return foundEffects;
    }

    template <typename T>
    bool HasEffectOfType() const
    {
        for (const auto &effect : effects)
        {
            if (std::dynamic_pointer_cast<T>(effect))
                return true;
        }
        return false;
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

    bool AddPowerWire(const Vector2Int &pos);
    // bool RemovePowerWire(const Vector2Int &pos);

    void CreateRectRoom(const Vector2Int &pos, const Vector2Int &size);
    void CreateHorizontalCorridor(const Vector2Int &startPos, int length, int width);
};

std::shared_ptr<Station> CreateStation();