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
            std::shared_ptr<Tile> oldTile = GetTileAtPosition(pos);
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

    station->UpdateSpriteOffsets();

    station->effects.push_back(std::make_shared<FireEffect>(Vector2Int(12, 0)));
    station->effects.push_back(std::make_shared<FoamEffect>(Vector2Int(13, 0)));

    station->AddPowerWire(Vector2Int(0, 0));
    station->AddPowerWire(Vector2Int(0, -2));
    station->AddPowerWire(Vector2Int(0, -1));


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
    std::shared_ptr<Tile> tile = GetTileAtPosition(pos, height);

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
    status |= CheckAdjacentTile(tilePos, tileId, Direction::N) ? SpriteCondition::NORTH_SAME : SpriteCondition::NORTH_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::E) ? SpriteCondition::EAST_SAME : SpriteCondition::EAST_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::S) ? SpriteCondition::SOUTH_SAME : SpriteCondition::SOUTH_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::W) ? SpriteCondition::WEST_SAME : SpriteCondition::WEST_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::N | Direction::E) ? SpriteCondition::NORTH_EAST_SAME : SpriteCondition::NORTH_EAST_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::S | Direction::E) ? SpriteCondition::SOUTH_EAST_SAME : SpriteCondition::SOUTH_EAST_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::S | Direction::W) ? SpriteCondition::SOUTH_WEST_SAME : SpriteCondition::SOUTH_WEST_DIFFERENT;
    status |= CheckAdjacentTile(tilePos, tileId, Direction::N | Direction::W) ? SpriteCondition::NORTH_WEST_SAME : SpriteCondition::NORTH_WEST_DIFFERENT;

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

std::shared_ptr<Tile> Station::GetTileAtPosition(const Vector2Int &pos, TileDef::Height height) const
{
    auto posIt = tileMap.find(pos);
    if (posIt != tileMap.end())
    {
        if (height == TileDef::Height::NONE)
        {
            if (posIt->second.empty())
                return nullptr;

            return posIt->second[0];
        }
        else
        {
            for (const std::shared_ptr<Tile> &tile : posIt->second)
            {
                if ((magic_enum::enum_integer(tile->GetHeight()) & magic_enum::enum_integer(height)) == 0)
                    continue;

                return tile;
            }
        }
    }

    return nullptr;
}

const std::vector<std::shared_ptr<Tile>> &Station::GetTilesAtPosition(const Vector2Int &pos) const
{
    static const std::vector<std::shared_ptr<Tile>> empty;
    auto posIt = tileMap.find(pos);
    if (posIt == tileMap.end())
        return empty;

    return posIt->second;
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

bool Station::AddPowerWire(const Vector2Int &pos)
{
    std::shared_ptr<PowerGrid> finalGrid = nullptr;
    std::vector<std::shared_ptr<PowerGrid>> gridsToRemove;

    // Iterate through existing power grids
    for (auto it = powerGrids.begin(); it != powerGrids.end(); ++it)
    {
        auto &currentGrid = *it;
        // Check if can connect to the current grid
        u_int8_t state = currentGrid->GetWireProximityState(pos);

        // Wire already exists in this grid
        if (state == 1)
            return false;

        // Not connectable to this grid
        if (state == 0)
            continue;

        // Current grid is connectable
        if (!finalGrid)
        {
            // This is the first connectable grid found, make it the finalGrid
            finalGrid = currentGrid;
            finalGrid->AddWire(pos);
        }
        else
        {
            // Another connectable grid found, merge it into finalGrid
            if (currentGrid != finalGrid)
            {
                finalGrid->MergeGrid(currentGrid);
                // Mark currentGrid for removal
                gridsToRemove.push_back(currentGrid);
            }
        }
    }

    // Remove all grids that were merged into finalGrid
    if (!gridsToRemove.empty())
    {
        std::erase_if(powerGrids, [&](const std::shared_ptr<PowerGrid> &g)
                      { return Contains(gridsToRemove, g); });
    }

    // If no existing grid was connectable, create a new one
    if (!finalGrid)
    {
        auto newGrid = std::make_shared<PowerGrid>();
        newGrid->AddWire(pos);
        powerGrids.push_back(newGrid);
        finalGrid = newGrid;
    }

    auto powerConsumerTile = GetTileWithComponentAtPosition<PowerConsumerComponent>(pos);
    if (powerConsumerTile)
    {
        auto powerConsumerComponent = powerConsumerTile->GetComponent<PowerConsumerComponent>();
        auto powerConnectorComponent = powerConsumerTile->GetComponent<PowerConnectorComponent>();
        // Add the consumer to the grid
        if (powerConsumerComponent && powerConnectorComponent)
        {
            finalGrid->AddConsumer(pos, powerConsumerComponent);
            powerConnectorComponent->SetPowerGrid(finalGrid);
        }
    }

    auto powerProducerTile = GetTileWithComponentAtPosition<PowerProducerComponent>(pos);
    if (powerProducerTile)
    {
        auto powerProducerComponent = powerProducerTile->GetComponent<PowerProducerComponent>();
        auto powerConnectorComponent = powerProducerTile->GetComponent<PowerConnectorComponent>();
        // Add the producer to the grid
        if (powerProducerComponent && powerConnectorComponent)
        {
            finalGrid->AddProducer(pos, powerProducerComponent);
            powerConnectorComponent->SetPowerGrid(finalGrid);
        }
    }

    auto batteryTile = GetTileWithComponentAtPosition<BatteryComponent>(pos);
    if (batteryTile)
    {
        auto batteryComponent = batteryTile->GetComponent<BatteryComponent>();
        auto powerConnectorComponent = batteryTile->GetComponent<PowerConnectorComponent>();
        // Add the battery to the grid
        if (batteryComponent && powerConnectorComponent)
        {
            finalGrid->AddBattery(pos, batteryComponent);
            powerConnectorComponent->SetPowerGrid(finalGrid);
        }
    }

    return true;
}

bool Station::RemovePowerWire(const Vector2Int &pos)
{
    // Find the grid that owns this wire
    std::shared_ptr<PowerGrid> originalGrid = GetPowerGridAt(pos);

    // Wire not part of any grid
    if (!originalGrid)
        return false;

    // Remove the wire from the grid
    originalGrid->RemoveWire(pos);

    // If the grid is now empty after removing the wire
    if (originalGrid->GetWires().empty())
    {
        // Remove the empty grid
        std::erase(powerGrids, originalGrid);
        return true;
    }

    // Perform flood fill to find all connected components in the potentially fragmented grid
    std::vector<std::unordered_set<Vector2Int>> foundComponents;
    // Tracks wires already assigned to a component
    std::unordered_set<Vector2Int> visitedWires;

    // Iterate over all wires remaining in the original grid to find components
    for (const auto &startWirePos : originalGrid->GetWires())
    {
        // This wire is already part of a found component
        if (visitedWires.contains(startWirePos))
            continue;

        // Start a new BFS for a new component
        std::unordered_set<Vector2Int> currentComponent;
        std::queue<Vector2Int> toVisitQueue;
        toVisitQueue.push(startWirePos);

        while (!toVisitQueue.empty())
        {
            Vector2Int currentWire = toVisitQueue.front();
            toVisitQueue.pop();

            // Already processed
            if (visitedWires.contains(currentWire))
                continue;

            visitedWires.insert(currentWire);
            currentComponent.insert(currentWire);

            for (const auto &dir : CARDINAL_DIRECTIONS)
            {
                Vector2Int neighborPos = currentWire + DirectionToVector2Int(dir);

                // Check if neighbor is a wire in this grid and not yet visited
                if (originalGrid->ContainsWire(neighborPos) && !visitedWires.contains(neighborPos))
                    toVisitQueue.push(neighborPos);
            }
        }

        if (!currentComponent.empty())
            foundComponents.push_back(std::move(currentComponent));
    }

    // If the grid did not split, no need to replace it in powerGrids
    if (foundComponents.size() <= 1)
        return true;

    // Grid split into multiple components, remove the original grid object
    std::erase(powerGrids, originalGrid);

    // Create new grid objects for each identified component
    for (const auto &componentWires : foundComponents)
    {
        auto newGrid = std::make_shared<PowerGrid>();
        for (const Vector2Int &wirePosInComponent : componentWires)
        {
            // Populate the new grid
            newGrid->AddWire(wirePosInComponent);
        }

        // Add the new grid to the station's list
        powerGrids.push_back(newGrid);
    }

    return true;
}
