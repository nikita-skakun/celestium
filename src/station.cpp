#include "station.hpp"
#include "audio_manager.hpp"
#include "game_state.hpp"
#include <queue>

void Station::CreateRectRoom(const Vector2Int &pos, const Vector2Int &size)
{
    auto self = shared_from_this();
    for (int y = 0; y < size.y; y++)
    {
        for (int x = 0; x < size.x; x++)
        {
            bool isWall = (x == 0 || y == 0 || x == size.x - 1 || y == size.y - 1);
            Tile::CreateTile(isWall ? "WALL" : "BLUE_FLOOR", pos + Vector2Int(x, y), self);
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
            const auto &oldTile = GetTileAtPosition(pos);
            if (isWall && oldTile)
                continue;

            if (!isWall && oldTile)
                oldTile->DeleteTile();

            Tile::CreateTile(isWall ? "WALL" : "BLUE_FLOOR", pos, self);

            // TODO: Add larger sized doors
            if (isEnding && !isWall)
                Tile::CreateTile("DOOR", pos, self);
        }
    }
}

std::shared_ptr<Station> CreateStation()
{
    std::shared_ptr<Station> station = std::make_shared<Station>();
    station->CreateRectRoom(Vector2Int(-4, -4), Vector2Int(9, 9));
    station->CreateRectRoom(Vector2Int(10, -4), Vector2Int(9, 9));
    station->CreateHorizontalCorridor(Vector2Int(4, 0), 7, 3);

    Tile::CreateTile("OXYGEN_PRODUCER", Vector2Int(0, 0), station);
    Tile::CreateTile("OXYGEN_PRODUCER", Vector2Int(14, 0), station);
    Tile::CreateTile("BATTERY", Vector2Int(0, -2), station);
    Tile::CreateTile("FRAME", Vector2Int(0, -5), station);
    Tile::CreateTile("FRAME", Vector2Int(0, -6), station);
    Tile::CreateTile("FRAME", Vector2Int(-1, -6), station);
    Tile::CreateTile("FRAME", Vector2Int(1, -6), station);
    Tile::CreateTile("FRAME", Vector2Int(0, -7), station);
    Tile::CreateTile("FRAME", Vector2Int(-1, -7), station);
    Tile::CreateTile("FRAME", Vector2Int(1, -7), station);
    Tile::CreateTile("SOLAR_PANEL", Vector2Int(0, -7), station);
    Tile::CreateTile("SOLAR_PANEL", Vector2Int(-1, -7), station);
    Tile::CreateTile("SOLAR_PANEL", Vector2Int(1, -7), station);

    station->effects.push_back(std::make_shared<FireEffect>(Vector2Int(12, 0)));
    station->effects.push_back(std::make_shared<FoamEffect>(Vector2Int(13, 0)));
    station->effects.push_back(std::make_shared<FoamEffect>(Vector2Int(13, 1)));
    station->effects.push_back(std::make_shared<FoamEffect>(Vector2Int(13, 2)));

    // Place initial wire tiles as real tiles on the POWER layer
    Tile::CreateTile("WIRE", Vector2Int(0, 0), station);
    Tile::CreateTile("WIRE", Vector2Int(0, -2), station);
    Tile::CreateTile("WIRE", Vector2Int(0, -1), station);

    station->UpdateSpriteOffsets();

    auto fireAlarm = AudioManager::LoadSoundEffect("../assets/audio/fire_alarm.opus", SoundEffect::Type::EFFECT, false, true, .05);
    std::weak_ptr<SoundEffect> _fireAlarm = fireAlarm;
    fireAlarm->onUpdate = [_fireAlarm, station]()
    {
        auto fireAlarm = _fireAlarm.lock();
        if (!station || !fireAlarm)
            return true;

        if (!station->HasEffectOfType<FireEffect>())
            fireAlarm->Stop();
        else if (GameManager::IsInBuildMode())
            fireAlarm->Pause();
        else
            fireAlarm->Play();

        return false;
    };

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

    if (!tile)
        return "";

    return tile->GetId();
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
    if (!GetTileWithComponentAtPosition<WalkableComponent>(pos))
        return false;

    if (GetTileWithComponentAtPosition<SolidComponent>(pos) && !GetTileWithComponentAtPosition<DoorComponent>(pos))
        return false;

    return true;
}

bool Station::IsDoorFullyOpenAtPos(const Vector2Int &pos) const
{
    if (auto doorTile = GetTileWithComponentAtPosition<DoorComponent>(pos))
    {
        auto door = doorTile->GetComponent<DoorComponent>();
        if (door->GetProgress() > 0.f)
            return false;
    }
    return true;
}

std::vector<std::shared_ptr<Effect>> Station::GetEffectsAtPosition(const Vector2Int &pos) const
{
    std::vector<std::shared_ptr<Effect>> foundEffects;

    for (const auto &effect : effects)
    {
        if (effect->GetPosition() == pos)
            foundEffects.push_back(effect);
    }

    return foundEffects;
}

void Station::RemoveEffect(const std::shared_ptr<Effect> &effect)
{
    std::erase_if(effects, [effect](const std::shared_ptr<Effect> &other)
                  { return effect == other; });
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

    // Clear existing grids
    powerGrids.clear();

    // Collect all wire positions from POWER-layer tiles only
    std::vector<Vector2Int> allWires;
    for (const auto &tilesAtPos : tileMap)
    {
        const Vector2Int &pos = tilesAtPos.first;
        if (GetTileWithHeightAtPosition(pos, TileDef::Height::POWER))
            allWires.push_back(pos);
    }

    // First: discover all connected components of power wires
    std::unordered_set<Vector2Int> visited;
    std::vector<std::vector<Vector2Int>> components;
    for (const auto &start : allWires)
    {
        if (visited.contains(start))
            continue;

        std::vector<Vector2Int> componentVec;
        std::queue<Vector2Int> q;
        q.push(start);

        while (!q.empty())
        {
            Vector2Int cur = q.front();
            q.pop();
            if (visited.contains(cur))
                continue;
            visited.insert(cur);
            componentVec.push_back(cur);

            for (const auto &dir : CARDINAL_DIRECTIONS)
            {
                Vector2Int nb = cur + DirectionToVector2Int(dir);
                if (visited.contains(nb))
                    continue;

                // Neighbor is considered part of the component if it has a POWER-layer tile
                if (GetTileWithHeightAtPosition(nb, TileDef::Height::POWER))
                    q.push(nb);
            }
        }

        if (!componentVec.empty())
            components.push_back(std::move(componentVec));
    }

    // For each component compute overlap counts with old grids and track best
    std::vector<std::unordered_map<std::shared_ptr<PowerGrid>, int>> componentOverlaps(components.size());

    for (size_t i = 0; i < components.size(); ++i)
    {
        for (const auto &p : components[i])
        {
            auto oldWireGridItOpt = Find(oldWireToGridMap, p);
            if (oldWireGridItOpt.has_value())
            {
                auto oldWireGridIt = *oldWireGridItOpt;
                if (oldWireGridIt->second)
                {
                    auto oldGridShared = oldWireGridIt->second;
                    componentOverlaps[i][oldGridShared]++;
                }
            }
        }
    }

    // Determine for each old grid which component it overlaps the most (winner)
    std::unordered_map<std::shared_ptr<PowerGrid>, std::pair<size_t, int>> oldGridBest; // oldGrid -> (bestComponentIndex, count)
    for (size_t i = 0; i < componentOverlaps.size(); ++i)
    {
        for (const auto &pair : componentOverlaps[i])
        {
            auto oldShared = pair.first;
            int count = pair.second;
            auto it = oldGridBest.find(oldShared);
            if (it == oldGridBest.end() || count > it->second.second)
            {
                oldGridBest[oldShared] = std::make_pair(i, count);
            }
        }
    }

    // Now construct new PowerGrid objects for each component. Only inherit
    // color from an old grid if that old grid's best component is this one.
    for (size_t idx = 0; idx < components.size(); ++idx)
    {
        auto &component = components[idx];
        auto newGrid = std::make_shared<PowerGrid>();

        // Find an old grid that chose this component as its best overlap
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
        {
            // Inherit the old color for the winning component only
            newGrid->SetDebugColor(chosenOldGrid->GetDebugColor());
        }

        for (const auto &wirePos : component)
        {
            // Add connectors at this position if any
            auto consumerTile = GetTileWithComponentAtPosition<PowerConsumerComponent>(wirePos);
            if (consumerTile)
            {
                auto consumer = consumerTile->GetComponent<PowerConsumerComponent>();
                if (consumer)
                    newGrid->AddConsumer(wirePos, consumer);
            }

            auto producerTile = GetTileWithComponentAtPosition<PowerProducerComponent>(wirePos);
            if (producerTile)
            {
                auto producer = producerTile->GetComponent<PowerProducerComponent>();
                if (producer)
                    newGrid->AddProducer(wirePos, producer);
            }

            auto batteryTile = GetTileWithComponentAtPosition<BatteryComponent>(wirePos);
            if (batteryTile)
            {
                auto battery = batteryTile->GetComponent<BatteryComponent>();
                if (battery)
                    newGrid->AddBattery(wirePos, battery);
            }

            auto allTilesHere = GetAllTilesAtPosition(wirePos);
            for (const auto &tile : allTilesHere)
            {
                if (auto connector = tile->GetComponent<PowerConnectorComponent>())
                {
                    connector->SetPowerGrid(newGrid);
                }
            }
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
