#pragma once
#include "tile_enums.hpp"
#include "navigation.hpp"
#include <unordered_set>

struct Effect;
struct PlannedTask;
struct PowerGrid;
struct Tile;

enum class Direction : uint8_t;
enum class SpriteCondition : uint32_t;

struct Station : public std::enable_shared_from_this<Station>
{
    std::unordered_map<Vector2Int, std::vector<std::shared_ptr<Tile>>> tileMap;
    std::vector<std::shared_ptr<Effect>> effects;
    std::vector<std::shared_ptr<PowerGrid>> powerGrids;
    std::vector<std::shared_ptr<PlannedTask>> plannedTasks;
    std::unordered_map<std::string, int> resources;
    
    // Navigation Graph
    std::vector<ConvexPolygon> navPolygons;
    std::vector<std::shared_ptr<Room>> rooms;
    std::unordered_map<Vector2Int, int> tileToPoly;

public:
    template <typename Predicate>
    std::shared_ptr<Tile> FindTile(const Vector2Int &pos, Predicate pred) const
    {
        for (const auto &tile : GetTilesAtPosition(pos))
            if (pred(tile)) return tile;
        return nullptr;
    }

    template <typename Predicate>
    std::vector<std::shared_ptr<Tile>> FindTiles(const Vector2Int &pos, Predicate pred) const
    {
        std::vector<std::shared_ptr<Tile>> found;
        for (const auto &tile : GetTilesAtPosition(pos))
            if (pred(tile)) found.push_back(tile);
        return found;
    }

    std::shared_ptr<Tile> GetTileAtPosition(const Vector2Int &pos, TileHeight height = TileHeight::NONE) const;
    const std::vector<std::shared_ptr<Tile>> &GetTilesAtPosition(const Vector2Int &pos) const;

    SpriteCondition GetSpriteConditionForTile(const std::shared_ptr<Tile> &tile) const;
    void UpdateSpriteOffsets() const;

    bool IsPositionPathable(const Vector2Int &pos) const;
    bool IsDoorFullyOpenAtPos(const Vector2Int &pos) const;
    std::shared_ptr<Room> GetRoomAtPosition(const Vector2Int &pos) const;

    void RemoveEffect(const std::shared_ptr<Effect> &effect);

    std::vector<std::shared_ptr<Effect>> GetEffectsAtPosition(const Vector2Int &pos) const;
    std::shared_ptr<Effect> GetEffectOfTypeAtPosition(const Vector2Int &pos, const std::string &id) const;
    bool HasEffectOfType(const std::string &id) const;

    std::shared_ptr<Tile> GetTileWithComponentAtPosition(const Vector2Int &pos, ComponentType type) const;
    std::vector<std::shared_ptr<Tile>> GetTilesWithComponentAtPosition(const Vector2Int &pos, ComponentType type) const;

    void RebuildPowerGridsFromInfrastructure();
    void RebuildNavigationGraph();


    void CreateRectRoom(const Vector2Int &pos, const Vector2Int &size);
    void CreateHorizontalCorridor(const Vector2Int &startPos, int length, int width);

    void AddPlannedTask(const Vector2Int &pos, const std::string &tileId, bool isBuild);
    void CompletePlannedTask(const Vector2Int &pos);
    void CancelPlannedTask(const Vector2Int &pos);
    bool HasPlannedTaskAt(const Vector2Int &pos) const;

    int GetResourceCount(const std::string &resourceId) const;
    void AddResource(const std::string &resourceId, int amount);
    bool HasResources(const std::unordered_map<std::string, int> &requiredResources) const;
    void ConsumeResources(const std::unordered_map<std::string, int> &resourcesToConsume);
    void ReturnResourcesFromTile(const std::shared_ptr<Tile> &tile);

private:
    void DecomposeRoom(const std::shared_ptr<Room> &room, const std::unordered_set<Vector2Int> &tiles, std::unordered_map<Vector2Int, int> &tileToPoly);
};

std::shared_ptr<Station> CreateStation();