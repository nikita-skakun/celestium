#include "audio_manager.hpp"
#include "component.hpp"
#include "env_effect.hpp"
#include "game_state.hpp"
#include "planned_task.hpp"
#include "power_grid.hpp"
#include "sprite.hpp"
#include "station.hpp"
#include "tile.hpp"
#include <deque>
#include <queue>
#include <set>
#include <unordered_set>

void Station::CreateRectRoom(const Vector2Int &pos, const Vector2Int &size)
{
    auto self = shared_from_this();
    for (int y = 0; y < size.y; y++)
    {
        for (int x = 0; x < size.x; x++)
        {
            bool isWall = (x == 0 || y == 0 || x == size.x - 1 || y == size.y - 1);
            Tile::CreateTile(isWall ? "WALL" : "BLUE_FLOOR", pos + Vector2Int(x, y), self, true, false);
        }
    }
}

void Station::CreateHorizontalCorridor(const Vector2Int &startPos, int length, int width)
{
    if (width < 1 || length == 0)
        return;

    int totalWidth = width + 2;
    int start = -(totalWidth / 2);
    int end = (totalWidth + 1) / 2;

    int direction = (length > 0) ? 1 : -1;
    int absLength = std::abs(length);

    auto self = shared_from_this();

    for (int i = 0; i < absLength; i++)
    {
        for (int y = start; y < end; y++)
        {
            bool isEnding = (i == 0 || i == absLength - 1);
            bool isWall = isEnding ? (y != 0) : (y == start || y == end - 1);
            Vector2Int pos = startPos + Vector2Int(i * direction, y);
            Tile::CreateTile(isWall ? "WALL" : "BLUE_FLOOR", pos, self, true, false);

            // TODO: Add larger sized doors
            if (isEnding && !isWall)
                Tile::CreateTile("DOOR", pos, self, true, false);
        }
    }
}

std::shared_ptr<Station> CreateStation()
{
    std::shared_ptr<Station> station = std::make_shared<Station>();
    station->CreateRectRoom(Vector2Int(-4, -4), Vector2Int(9, 9));
    station->CreateRectRoom(Vector2Int(4, -4), Vector2Int(9, 9));

    // Delete the tile at the door position to make space for the door
    Tile::CreateTile("BLUE_FLOOR", Vector2Int(4, 0), station, true, false);
    Tile::CreateTile("DOOR", Vector2Int(4, 0), station, true, false);
    Tile::CreateTile("OXYGEN_PRODUCER", Vector2Int(0, 0), station, true, false);
    Tile::CreateTile("BATTERY", Vector2Int(0, -2), station, true, false);
    Tile::CreateTile("FRAME", Vector2Int(0, -5), station, true, false);

    for (int i = -1; i <= 1; i++)
    {
        Tile::CreateTile("FRAME", Vector2Int(i, -6), station, true, false);
        Tile::CreateTile("SOLAR_PANEL", Vector2Int(i, -6), station, true, false);
        Tile::CreateTile("WIRE", Vector2Int(i, -6), station, true, false);
    }

    for (int i = 0; i <= 5; i++)
        Tile::CreateTile("WIRE", Vector2Int(0, -i), station, true, false);

    for (int i = 0; i <= 4; i++)
        Tile::CreateTile("WIRE", Vector2Int(i, 0), station, true, false);

    station->UpdateSpriteOffsets();
    station->RebuildNavigationGraph();

    // Initialize starting resources
    station->AddResource("METAL", 100);
    station->AddResource("ELECTRONICS", 50);

    return station;
}

void Station::UpdateSpriteOffsets() const
{
    for (const auto &tilesAtPos : tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            tile->RemoveComponent<DecorativeComponent>();

            SpriteCondition status = GetSpriteConditionForTile(tile);

            if (auto spriteDef = tile->GetTileDefinition()->GetReferenceSprite())
            {
                if (auto basicSpriteDef = std::dynamic_pointer_cast<BasicSpriteDef>(spriteDef))
                {
                    tile->SetSprite(std::make_shared<BasicSprite>(basicSpriteDef->spriteOffset));
                }
                else if (auto multiSliceSpriteDef = std::dynamic_pointer_cast<MultiSliceSpriteDef>(spriteDef))
                {
                    std::vector<SpriteSlice> slices;
                    for (const auto &sliceCondition : multiSliceSpriteDef->slices)
                    {
                        if ((status & sliceCondition.conditions) == sliceCondition.conditions)
                            slices.push_back(sliceCondition.slice);
                    }

                    tile->SetSprite(std::make_shared<MultiSliceSprite>(slices));
                }
            }

            if (tile->GetId() == "DOOR")
            {
                Rotation rotation = Rotation::UP;
                if (auto rotatable = tile->GetComponent<RotatableComponent>())
                    rotation = rotatable->GetRotation();

                auto decorative = tile->AddComponent<DecorativeComponent>(tile);
                decorative->AddDecorativeTile(std::make_shared<BasicSprite>(Vector2Int(0, 5), OffsetWithRotation(rotation, Vector2Int(0, -1))));
                decorative->AddDecorativeTile(std::make_shared<BasicSprite>(Vector2Int(0, 6), OffsetWithRotation(rotation, Vector2Int(0, 1))));
            }
        }
    }
}

std::string Station::GetTileIdAtPosition(const Vector2Int &pos, TileDef::Height height) const
{
    const auto &tile = GetTileAtPosition(pos, height);
    return tile ? tile->GetId() : "";
}

bool Station::CheckAdjacentTile(const Vector2Int &tilePos, const std::string &tileId, Direction direction, TileDef::Height height) const
{
    return GetTileIdAtPosition(tilePos + DirectionToVector2Int(direction), height) == tileId;
}

SpriteCondition Station::GetSpriteConditionForTile(const std::shared_ptr<Tile> &tile) const
{
    if (!tile)
        return SpriteCondition::NONE;

    const Vector2Int &tilePos = tile->GetPosition();
    const std::string &tileId = tile->GetId();

    SpriteCondition status = SpriteCondition::NONE;
    auto height = tile->GetHeight();
    status |= CheckAdjacentTile(tilePos, tileId, Direction::N, height) ? SpriteCondition::NORTH_SAME : SpriteCondition::NORTH_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::E, height) ? SpriteCondition::EAST_SAME : SpriteCondition::EAST_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::S, height) ? SpriteCondition::SOUTH_SAME : SpriteCondition::SOUTH_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::W, height) ? SpriteCondition::WEST_SAME : SpriteCondition::WEST_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::N | Direction::E, height) ? SpriteCondition::NORTH_EAST_SAME : SpriteCondition::NORTH_EAST_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::S | Direction::E, height) ? SpriteCondition::SOUTH_EAST_SAME : SpriteCondition::SOUTH_EAST_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::S | Direction::W, height) ? SpriteCondition::SOUTH_WEST_SAME : SpriteCondition::SOUTH_WEST_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::N | Direction::W, height) ? SpriteCondition::NORTH_WEST_SAME : SpriteCondition::NORTH_WEST_DIFFERENT;

    return status;
}

bool Station::IsPositionPathable(const Vector2Int &pos) const
{
    if (!GetTileWithComponentAtPosition(pos, ComponentType::WALKABLE))
        return false;

    auto doorTile = GetTileWithComponentAtPosition(pos, ComponentType::DOOR);
    if (GetTileWithComponentAtPosition(pos, ComponentType::SOLID) && !doorTile)
        return false;

    if (doorTile && !doorTile->IsActive())
        return false;

    return true;
}

std::shared_ptr<Room> Station::GetRoomAtPosition(const Vector2Int &pos) const
{
    if (!tileToPoly.contains(pos)) return nullptr;
    int polyIdx = tileToPoly.at(pos);
    const auto& poly = navPolygons[polyIdx];
    if (poly.roomId >= 0 && poly.roomId < (int)rooms.size())
        return rooms[poly.roomId];
    return nullptr;
}

bool Station::IsDoorFullyOpenAtPos(const Vector2Int &pos) const
{
    if (auto doorTile = GetTileWithComponentAtPosition(pos, ComponentType::DOOR))
    {
        auto door = doorTile->GetComponent<DoorComponent>();
        if (!door->IsOpen())
            return false;
    }
    return true;
}

void Station::RemoveEffect(const std::shared_ptr<Effect> &effect)
{
    std::erase_if(effects, [effect](const std::shared_ptr<Effect> &other)
                  { return effect == other; });
}

std::vector<std::shared_ptr<Effect>> Station::GetEffectsAtPosition(const Vector2Int &pos) const
{
    std::vector<std::shared_ptr<Effect>> foundEffects;
    std::ranges::copy_if(effects, std::back_inserter(foundEffects),
                         [&pos](const std::shared_ptr<Effect> &effect)
                         { return effect && effect->GetPosition() == pos; });
    return foundEffects;
}

std::shared_ptr<Effect> Station::GetEffectOfTypeAtPosition(const Vector2Int &pos, const std::string &id) const
{
    if (auto it = std::ranges::find_if(effects, [&pos, &id](const std::shared_ptr<Effect> &effect)
                                       { return effect && effect->GetPosition() == pos && effect->GetId() == id; });
        it != effects.end())
        return *it;
    return nullptr;
}

bool Station::HasEffectOfType(const std::string &id) const
{
    return std::ranges::any_of(effects, [&id](const std::shared_ptr<Effect> &effect)
                               { return effect && effect->GetId() == id; });
}

std::shared_ptr<Tile> Station::GetTileWithComponentAtPosition(const Vector2Int &pos, ComponentType type) const
{
    auto posIt = tileMap.find(pos);
    if (posIt == tileMap.end())
        return nullptr;

    for (const std::shared_ptr<Tile> &tile : posIt->second)
    {
        if (tile->HasComponent(type))
            return tile;
    }

    return nullptr;
}

std::vector<std::shared_ptr<Tile>> Station::GetTilesWithComponentAtPosition(const Vector2Int &pos, ComponentType type) const
{
    std::vector<std::shared_ptr<Tile>> result;

    auto posIt = tileMap.find(pos);
    if (posIt == tileMap.end())
        return result;

    for (const std::shared_ptr<Tile> &tile : posIt->second)
    {
        if (tile->HasComponent(type))
            result.push_back(tile);
    }

    return result;
}

// Rebuild powerGrids from POWER-layer tiles.
void Station::RebuildPowerGridsFromInfrastructure()
{
    // Preserve old wire->grid mapping by reading the PowerConnectorComponent on existing POWER tiles
    std::unordered_map<Vector2Int, std::shared_ptr<PowerGrid>> oldWireToGridMap;
    for (const auto &tilesAtPos : tileMap)
    {
        const Vector2Int &pos = tilesAtPos.first;
        if (auto powerTile = GetTileWithHeightAtPosition(pos, TileDef::Height::POWER))
        {
            if (auto connector = powerTile->GetComponent<PowerConnectorComponent>())
            {
                if (auto g = connector->GetPowerGrid())
                    oldWireToGridMap[pos] = g;
            }
        }
    }

    for (const auto &tilesAtPos : tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            if (auto consumer = tile->GetComponent<PowerConsumerComponent>())
                consumer->SetActive(false);
        }
    }

    // Clear existing grids
    powerGrids.clear();

    // Component info collected during a single-pass flood fill
    struct ComponentInfo
    {
        std::vector<Vector2Int> positions;
        std::unordered_map<std::shared_ptr<PowerGrid>, int> overlapCounts;
        std::vector<std::pair<Vector2Int, std::weak_ptr<PowerProducerComponent>>> producers;
        std::vector<std::pair<Vector2Int, std::weak_ptr<PowerConsumerComponent>>> consumers;
        std::vector<std::pair<Vector2Int, std::weak_ptr<BatteryComponent>>> batteries;
        std::vector<std::weak_ptr<PowerConnectorComponent>> connectors;
    };

    std::unordered_set<Vector2Int> visited;
    std::vector<ComponentInfo> components;

    // Iterate over tileMap once and start flood-fills from any unvisited POWER-layer tile
    for (const auto &tilesAtPos : tileMap)
    {
        const Vector2Int &start = tilesAtPos.first;

        if (visited.contains(start))
            continue;

        if (!GetTileWithHeightAtPosition(start, TileDef::Height::POWER))
            continue; // skip non-power positions

        ComponentInfo comp;
        std::deque<Vector2Int> q;
        q.push_back(start);

        while (!q.empty())
        {
            Vector2Int cur = q.front();
            q.pop_front();
            if (visited.contains(cur))
                continue;
            visited.insert(cur);

            // Only consider positions that actually have a POWER-layer tile
            if (!GetTileWithHeightAtPosition(cur, TileDef::Height::POWER))
                continue;

            comp.positions.push_back(cur);

            // Record overlap with old grid if any
            if (auto itOpt = Find(oldWireToGridMap, cur); itOpt.has_value())
            {
                auto it = *itOpt;
                if (it->second)
                    comp.overlapCounts[it->second]++;
            }

            // Collect producers/consumers/batteries/connectors at this position once
            auto allTilesHere = GetAllTilesAtPosition(cur);
            for (const auto &tile : allTilesHere)
            {
                if (auto prod = tile->GetComponent<PowerProducerComponent>())
                    comp.producers.emplace_back(cur, std::weak_ptr<PowerProducerComponent>(prod));
                if (auto cons = tile->GetComponent<PowerConsumerComponent>())
                    comp.consumers.emplace_back(cur, std::weak_ptr<PowerConsumerComponent>(cons));
                if (auto bat = tile->GetComponent<BatteryComponent>())
                    comp.batteries.emplace_back(cur, std::weak_ptr<BatteryComponent>(bat));
                if (auto connector = tile->GetComponent<PowerConnectorComponent>())
                    comp.connectors.push_back(std::weak_ptr<PowerConnectorComponent>(connector));
            }

            // Push neighboring positions that might have POWER-layer tiles
            for (const auto &dir : CARDINAL_DIRECTIONS)
            {
                Vector2Int nb = cur + DirectionToVector2Int(dir);
                if (visited.contains(nb))
                    continue;
                if (GetTileWithHeightAtPosition(nb, TileDef::Height::POWER))
                    q.push_back(nb);
            }
        }

        if (!comp.positions.empty())
            components.push_back(std::move(comp));
    }

    // For each old grid determine its best-overlapping component (winner)
    std::unordered_map<std::shared_ptr<PowerGrid>, std::pair<size_t, int>> oldGridBest;
    for (size_t i = 0; i < components.size(); ++i)
    {
        for (const auto &pair : components[i].overlapCounts)
        {
            auto it = oldGridBest.find(pair.first);
            if (it == oldGridBest.end() || pair.second > it->second.second)
                oldGridBest[pair.first] = std::make_pair(i, pair.second);
        }
    }

    // Construct new grids and populate them using the gathered per-component info
    for (size_t idx = 0; idx < components.size(); ++idx)
    {
        auto &comp = components[idx];
        auto newGrid = std::make_shared<PowerGrid>();

        // If an old grid selected this component as its best match, inherit its color
        std::shared_ptr<PowerGrid> chosenOldGrid = nullptr;
        for (const auto &bestPair : oldGridBest)
        {
            if (bestPair.second.first == static_cast<size_t>(idx))
            {
                chosenOldGrid = bestPair.first;
                break;
            }
        }

        if (chosenOldGrid)
            newGrid->SetDebugColor(chosenOldGrid->GetDebugColor());

        // Add producers/consumers/batteries
        for (const auto &p : comp.producers)
        {
            if (auto prodShared = p.second.lock())
                newGrid->AddProducer(p.first, prodShared);
        }
        for (const auto &c : comp.consumers)
        {
            if (auto consShared = c.second.lock())
                newGrid->AddConsumer(c.first, consShared);
        }
        for (const auto &b : comp.batteries)
        {
            if (auto batShared = b.second.lock())
                newGrid->AddBattery(b.first, batShared);
        }

        // Point connectors to the new grid
        for (const auto &weakConn : comp.connectors)
        {
            if (auto conn = weakConn.lock())
                conn->SetPowerGrid(newGrid);
        }

        newGrid->RebuildCaches();
        powerGrids.push_back(newGrid);
    }
}

std::shared_ptr<Tile> Station::GetTileAtPosition(const Vector2Int &pos, TileDef::Height height) const
{
    auto tilesAtPosOpt = Find(tileMap, pos);
    if (tilesAtPosOpt.has_value())
    {
        const auto &vec = (*tilesAtPosOpt)->second;
        if (height == TileDef::Height::NONE)
        {
            if (vec.empty())
                return nullptr;

            return vec[0];
        }
        else
        {
            for (const std::shared_ptr<Tile> &tile : vec)
            {
                if (magic_enum::enum_flags_test_any(tile->GetHeight(), height))
                    return tile;
            }
        }
    }

    return nullptr;
}

const std::vector<std::shared_ptr<Tile>> &Station::GetTilesAtPosition(const Vector2Int &pos) const
{
    static const std::vector<std::shared_ptr<Tile>> empty;
    auto tilesAtPosOpt = Find(tileMap, pos);
    if (!tilesAtPosOpt.has_value())
        return empty;

    return (*tilesAtPosOpt)->second;
}

std::vector<std::shared_ptr<Tile>> Station::GetDecorativeTilesAtPosition(const Vector2Int &pos) const
{
    std::vector<std::shared_ptr<Tile>> decorativeTiles;
    for (const auto &tilesAtPos : tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            if (auto decorative = tile->GetComponent<DecorativeComponent>())
            {
                for (const auto &dTiles : decorative->GetDecorativeTiles())
                {
                    if (pos == tile->GetPosition() + dTiles->GetOffsetFromMainTile())
                    {
                        decorativeTiles.push_back(tile);
                        continue;
                    }
                }
            }
        }
    }

    return decorativeTiles;
}

std::vector<std::shared_ptr<Tile>> Station::GetAllTilesAtPosition(const Vector2Int &pos) const
{
    auto tilesAtPos = GetDecorativeTilesAtPosition(pos);
    const auto &tiles = GetTilesAtPosition(pos);
    tilesAtPos.insert(tilesAtPos.begin(), tiles.begin(), tiles.end());

    return tilesAtPos;
}

std::shared_ptr<Tile> Station::GetTileWithHeightAtPosition(const Vector2Int &pos, TileDef::Height height) const
{
    const auto &tilesAtPos = GetTilesAtPosition(pos);
    if (height == TileDef::Height::NONE)
    {
        if (tilesAtPos.empty())
            return nullptr;

        return tilesAtPos[0];
    }
    for (const auto &tile : tilesAtPos)
    {
        if (magic_enum::enum_flags_test_any(tile->GetHeight(), height))
            return tile;
    }

    return nullptr;
}

std::vector<std::shared_ptr<Tile>> Station::GetTilesWithHeightAtPosition(const Vector2Int &pos, TileDef::Height height) const
{
    std::vector<std::shared_ptr<Tile>> foundTiles;

    const auto &tilesAtPos = GetTilesAtPosition(pos);
    if (height == TileDef::Height::NONE)
        return tilesAtPos;

    for (const auto &tile : tilesAtPos)
    {
        if (magic_enum::enum_flags_test_any(tile->GetHeight(), height))
            foundTiles.push_back(tile);
    }

    return foundTiles;
}

void Station::AddPlannedTask(const Vector2Int &pos, const std::string &tileId, bool isBuild)
{
    // Remove any existing plan at this position
    std::erase_if(plannedTasks, [pos](const std::shared_ptr<PlannedTask> &task)
                  { return task->position == pos; });

    plannedTasks.push_back(std::make_shared<PlannedTask>(PlannedTask(pos, tileId, isBuild)));
}

void Station::CompletePlannedTask(const Vector2Int &pos)
{
    auto it = std::find_if(plannedTasks.begin(), plannedTasks.end(), [pos](const std::shared_ptr<PlannedTask> &task)
                           { return task->position == pos; });

    if (it == plannedTasks.end())
        return;
    const auto &task = *it;

    if (task->isBuild)
    {
        if (!Tile::CreateTile(task->tileId, pos, shared_from_this(), true, true))
            return;
    }
    else
    {
        // Remove the specific tile by id
        const auto &tilesHere = GetTilesAtPosition(pos);
        for (const auto &tile : tilesHere)
        {
            if (tile->GetId() == task->tileId)
            {
                tile->DeleteTile(true);
                break;
            }
        }
    }

    plannedTasks.erase(it);
    UpdateSpriteOffsets();
    RebuildNavigationGraph();
}

void Station::CancelPlannedTask(const Vector2Int &pos)
{
    std::erase_if(plannedTasks, [pos](const std::shared_ptr<PlannedTask> &task)
                  { return task->position == pos; });
    UpdateSpriteOffsets();
}

bool Station::HasPlannedTaskAt(const Vector2Int &pos) const
{
    return std::any_of(plannedTasks.begin(), plannedTasks.end(), [pos](const std::shared_ptr<PlannedTask> &task)
                       { return task->position == pos; });
}

int Station::GetResourceCount(const std::string &resourceId) const
{
    auto it = resources.find(resourceId);
    return it != resources.end() ? it->second : 0;
}

void Station::AddResource(const std::string &resourceId, int amount)
{
    resources[resourceId] += amount;
    if (resources[resourceId] <= 0)
        resources.erase(resourceId);
}

bool Station::HasResources(const std::unordered_map<std::string, int> &requiredResources) const
{
    for (const auto &[resourceId, requiredAmount] : requiredResources)
    {
        if (GetResourceCount(resourceId) < requiredAmount)
            return false;
    }
    return true;
}

void Station::ConsumeResources(const std::unordered_map<std::string, int> &resourcesToConsume)
{
    for (const auto &[resourceId, amount] : resourcesToConsume)
        AddResource(resourceId, -amount);
}

void Station::ReturnResourcesFromTile(const std::shared_ptr<Tile> &tile)
{
    if (!tile)
        return;

    try
    {
        const auto &tileDef = tile->GetTileDefinition();
        const auto &requiredResources = tileDef->GetBuildResources();
        for (const auto &p : requiredResources)
        {
            const std::string &resId = p.first;
            int amount = p.second;
            float returnedF = std::ceil(amount * CREW_DECONSTRUCT_EFFICIENCY);
            int returned = static_cast<int>(returnedF);
            if (returned > 0)
                AddResource(resId, returned);
        }
    }
    catch (...)
    {
        throw std::runtime_error("Error returning resources from tile " + (tile ? tile->GetId() : "null"));
    }
}

void Station::RebuildNavigationGraph()
{
    navPolygons.clear();
    rooms.clear();
    tileToPoly.clear();

    std::unordered_set<Vector2Int> globalVisited;
    std::vector<std::pair<std::shared_ptr<Room>, std::unordered_set<Vector2Int>>> roomData;

    // 1. Identify rooms
    for (auto const &[pos, tiles] : tileMap)
    {
        if (globalVisited.contains(pos)) continue;
        if (!IsPositionPathable(pos) || GetTileWithComponentAtPosition(pos, ComponentType::DOOR)) continue;

        auto room = std::make_shared<Room>();
        room->id = (int)rooms.size();
        rooms.push_back(room);

        std::unordered_set<Vector2Int> roomTiles;
        std::queue<Vector2Int> q;
        q.push(pos);
        globalVisited.insert(pos);

        while (!q.empty())
        {
            Vector2Int cur = q.front();
            q.pop();
            roomTiles.insert(cur);

            for (const auto &dir : CARDINAL_DIRECTIONS)
            {
                Vector2Int nb = cur + DirectionToVector2Int(dir);
                if (!globalVisited.contains(nb) && IsPositionPathable(nb) && !GetTileWithComponentAtPosition(nb, ComponentType::DOOR))
                {
                    globalVisited.insert(nb);
                    q.push(nb);
                }
            }
        }
        roomData.push_back({room, std::move(roomTiles)});
    }

    // 2. Decompose rooms into polygons
    for (auto &data : roomData)
    {
        DecomposeRoom(data.first, data.second, tileToPoly);
    }

    // 3. Create door polygons
    for (auto const &[pos, tiles] : tileMap)
    {
        auto doorTile = GetTileWithComponentAtPosition(pos, ComponentType::DOOR);
        if (!doorTile) continue;

        int pIdx = (int)navPolygons.size();
        tileToPoly[pos] = pIdx;

        ConvexPolygon poly;
        poly.roomId = -1;
        float x = (float)pos.x - 0.5f;
        float y = (float)pos.y - 0.5f;

        poly.vertices[0] = {x, y};
        poly.vertices[1] = {x + 1.f, y};
        poly.vertices[2] = {x + 1.f, y + 1.f};
        poly.vertices[3] = {x, y + 1.f};

        navPolygons.push_back(poly);
    }

    // 4. Build adjacency via tile neighbors
    for (int i = 0; i < (int)navPolygons.size(); ++i)
    {
        auto& poly = navPolygons[i];
        poly.links.clear();

        Vector2 p0 = poly.vertices[0];
        Vector2 p2 = poly.vertices[2];
        Vector2Int minTile = {(int)std::floor(p0.x + 0.5f), (int)std::floor(p0.y + 0.5f)};
        Vector2Int maxTile = {(int)std::floor(p2.x - 0.5f), (int)std::floor(p2.y - 0.5f)};

        auto addNeighbor = [&](int edgeIdx, Vector2Int nbTile) {
            if (tileToPoly.contains(nbTile)) {
                int nbPolyIdx = tileToPoly[nbTile];
                if (nbPolyIdx != i) {
                    // Check if we already have a link to this polygon on this edge
                    bool exists = false;
                    for (const auto& link : poly.links) {
                        if (link.targetPolyIdx == nbPolyIdx && link.edgeIdx == edgeIdx) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        auto doorTile = GetTileWithComponentAtPosition(nbTile, ComponentType::DOOR);
                        
                        // Calculate portal segment based on edge index and nbTile
                        Vector2 pA = {(float)nbTile.x - 0.5f, (float)nbTile.y + 0.5f};
                        Vector2 pB = {(float)nbTile.x + 0.5f, (float)nbTile.y + 0.5f};
                        
                        if (edgeIdx == 1) { // East
                            pA = {(float)nbTile.x - 0.5f, (float)nbTile.y - 0.5f};
                            pB = {(float)nbTile.x - 0.5f, (float)nbTile.y + 0.5f};
                        } else if (edgeIdx == 2) { // South
                            pA = {(float)nbTile.x - 0.5f, (float)nbTile.y - 0.5f};
                            pB = {(float)nbTile.x + 0.5f, (float)nbTile.y - 0.5f};
                        } else if (edgeIdx == 3) { // West
                            pA = {(float)nbTile.x + 0.5f, (float)nbTile.y - 0.5f};
                            pB = {(float)nbTile.x + 0.5f, (float)nbTile.y + 0.5f};
                        }

                        poly.links.push_back({nbPolyIdx, edgeIdx, pA, pB, doorTile});
                    } else {
                        // Expand existing portal
                        for (auto& link : poly.links) {
                            if (link.targetPolyIdx == nbPolyIdx && link.edgeIdx == edgeIdx) {
                                if (edgeIdx == 0 || edgeIdx == 2) { // Horizontal edge
                                    link.portalA.x = std::min(link.portalA.x, (float)nbTile.x - 0.5f);
                                    link.portalB.x = std::max(link.portalB.x, (float)nbTile.x + 0.5f);
                                } else { // Vertical edge
                                    link.portalA.y = std::min(link.portalA.y, (float)nbTile.y - 0.5f);
                                    link.portalB.y = std::max(link.portalB.y, (float)nbTile.y + 0.5f);
                                }
                                break;
                            }
                        }
                    }
                }
            }
        };

        // North (Edge 0)
        for (int x = minTile.x; x <= maxTile.x; ++x) addNeighbor(0, {x, minTile.y - 1});
        // East (Edge 1)
        for (int y = minTile.y; y <= maxTile.y; ++y) addNeighbor(1, {maxTile.x + 1, y});
        // South (Edge 2)
        for (int x = minTile.x; x <= maxTile.x; ++x) addNeighbor(2, {x, maxTile.y + 1});
        // West (Edge 3)
        for (int y = minTile.y; y <= maxTile.y; ++y) addNeighbor(3, {minTile.x - 1, y});
    }
}

void Station::DecomposeRoom(const std::shared_ptr<Room> &room, const std::unordered_set<Vector2Int> &tiles, std::unordered_map<Vector2Int, int> &tileToPoly)
{
    auto comp = [](const Vector2Int& a, const Vector2Int& b) {
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    };
    std::set<Vector2Int, decltype(comp)> remaining(tiles.begin(), tiles.end(), comp);

    while (!remaining.empty())
    {
        Vector2Int start = *remaining.begin();

        int width = 1;
        while (remaining.count(start + Vector2Int(width, 0))) width++;

        int height = 1;
        bool canExpandY = true;
        while (canExpandY)
        {
            for (int x = 0; x < width; ++x)
            {
                if (!remaining.count(start + Vector2Int(x, height)))
                {
                    canExpandY = false;
                    break;
                }
            }
            if (canExpandY) height++;
        }

        int polyIdx = (int)navPolygons.size();
        ConvexPolygon poly;
        poly.roomId = room->id;
        float x = (float)start.x - 0.5f;
        float y = (float)start.y - 0.5f;
        float w = (float)width;
        float h = (float)height;

        poly.vertices[0] = {x, y};
        poly.vertices[1] = {x + w, y};
        poly.vertices[2] = {x + w, y + h};
        poly.vertices[3] = {x, y + h};
        
        navPolygons.push_back(poly);
        room->polygonIds.push_back(polyIdx);

        for (int i = 0; i < width; ++i)
            for (int j = 0; j < height; ++j) {
                Vector2Int t = start + Vector2Int(i, j);
                remaining.erase(t);
                tileToPoly[t] = polyIdx;
            }
    }
}
