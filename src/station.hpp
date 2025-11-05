#pragma once
#include "tile_def.hpp"

struct Effect;
struct PlannedTask;
struct PowerGrid;
struct Tile;

enum class SpriteCondition : uint32_t;
enum class ComponentType : uint8_t;

struct Station : public std::enable_shared_from_this<Station>
{
    std::unordered_map<Vector2Int, std::vector<std::shared_ptr<Tile>>> tileMap;
    std::vector<std::shared_ptr<Effect>> effects;
    std::vector<std::shared_ptr<PowerGrid>> powerGrids;
    std::vector<std::shared_ptr<PlannedTask>> plannedTasks;
    std::unordered_map<std::string, int> resources;

public:
    std::shared_ptr<Tile> GetTileAtPosition(const Vector2Int &pos, TileDef::Height height = TileDef::Height::NONE) const;
    const std::vector<std::shared_ptr<Tile>> &GetTilesAtPosition(const Vector2Int &pos) const;
    std::vector<std::shared_ptr<Tile>> GetDecorativeTilesAtPosition(const Vector2Int &pos) const;
    std::vector<std::shared_ptr<Tile>> GetAllTilesAtPosition(const Vector2Int &pos) const;
    std::shared_ptr<Tile> GetTileWithHeightAtPosition(const Vector2Int &pos, TileDef::Height height) const;
    std::vector<std::shared_ptr<Tile>> GetTilesWithHeightAtPosition(const Vector2Int &pos, TileDef::Height height) const;
    std::string GetTileIdAtPosition(const Vector2Int &pos, TileDef::Height height = TileDef::Height::NONE) const;
    bool CheckAdjacentTile(const Vector2Int &tilePos, const std::string &tileId, Direction direction, TileDef::Height height = TileDef::Height::NONE) const;
    
    SpriteCondition GetSpriteConditionForTile(const std::shared_ptr<Tile> &tile) const;
    void UpdateSpriteOffsets() const;
    
    bool IsPositionPathable(const Vector2Int &pos) const;
    bool IsDoorFullyOpenAtPos(const Vector2Int &pos) const;

    void RemoveEffect(const std::shared_ptr<Effect> &effect);
    
    std::vector<std::shared_ptr<Effect>> GetEffectsAtPosition(const Vector2Int &pos) const;
    std::shared_ptr<Effect> GetEffectOfTypeAtPosition(const Vector2Int &pos, const std::string &id) const;
    bool HasEffectOfType(const std::string &id) const;

    std::shared_ptr<Tile> GetTileWithComponentAtPosition(const Vector2Int &pos, ComponentType type) const;
    std::vector<std::shared_ptr<Tile>> GetTilesWithComponentAtPosition(const Vector2Int &pos, ComponentType type) const;

    void RebuildPowerGridsFromInfrastructure();

    void CreateRectRoom(const Vector2Int &pos, const Vector2Int &size);
    void CreateHorizontalCorridor(const Vector2Int &startPos, int length, int width);

    void AddPlannedTask(const Vector2Int &pos, const std::string &tileId, bool isBuild);
    void CompletePlannedTask(const Vector2Int &pos);
    bool HasPlannedTaskAt(const Vector2Int &pos) const;

    int GetResourceCount(const std::string &resourceId) const;
    void AddResource(const std::string &resourceId, int amount);
    bool HasResources(const std::unordered_map<std::string, int> &requiredResources) const;
    void ConsumeResources(const std::unordered_map<std::string, int> &resourcesToConsume);
};

std::shared_ptr<Station> CreateStation();