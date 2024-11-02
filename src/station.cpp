#include "station.hpp"
#include "audio_manager.hpp"
#include "game_state.hpp"

std::shared_ptr<Room> CreateRectRoom(const Vector2Int &pos, const Vector2Int &size, std::shared_ptr<Station> station)
{
    std::shared_ptr<Room> room = Room::CreateEmptyRoom(station);

    for (int y = 0; y < size.y; y++)
    {
        for (int x = 0; x < size.x; x++)
        {
            bool isWall = (x == 0 || y == 0 || x == size.x - 1 || y == size.y - 1);
            Tile::CreateTile(isWall ? "WALL" : "BLUE_FLOOR", pos + Vector2Int(x, y), station, room);
        }
    }

    return room;
}

std::shared_ptr<Room> CreateHorizontalCorridor(const Vector2Int &startPos, int length, int width, std::shared_ptr<Station> station)
{
    if (width < 1 || length == 0)
        return nullptr;

    std::shared_ptr<Room> room = Room::CreateEmptyRoom(station);

    int totalWidth = width + 2;
    int start = -(totalWidth / 2);
    int end = (totalWidth + 1) / 2;

    int direction = (length > 0) ? 1 : -1;
    int absLength = std::abs(length);

    for (int i = 0; i < absLength; i++)
    {
        for (int y = start; y < end; y++)
        {
            bool isEnding = (i == 0 || i == absLength - 1);
            bool isWall = isEnding ? (y != 0) : (y == start || y == end - 1);
            Vector2Int pos = startPos + Vector2Int(i * direction, y);
            std::shared_ptr<Tile> oldTile = station->GetTileAtPosition(pos);
            if (isWall && oldTile)
                continue;

            if (!isWall && oldTile)
                oldTile->DeleteTile();

            Tile::CreateTile(isWall ? "WALL" : "BLUE_FLOOR", pos, station, room);

            // TODO: Add larger sized doors
            if (isEnding && !isWall)
                Tile::CreateTile("DOOR", pos, station, room);
        }
    }

    return room;
}

std::shared_ptr<Station> CreateStation()
{
    std::shared_ptr<Station> station = std::make_shared<Station>();
    std::shared_ptr<Room> room1 = CreateRectRoom(Vector2Int(-4, -4), Vector2Int(9, 9), station);
    std::shared_ptr<Room> room2 = CreateRectRoom(Vector2Int(10, -4), Vector2Int(9, 9), station);
    std::shared_ptr<Room> room3 = CreateHorizontalCorridor(Vector2Int(4, 0), 7, 3, station);

    auto oxygenProducer1 = Tile::CreateTile("OXYGEN_PRODUCER", Vector2Int(0, 0), station, room1);
    auto oxygenProducer2 = Tile::CreateTile("OXYGEN_PRODUCER", Vector2Int(14, 0), station, room2);
    auto battery = Tile::CreateTile("BATTERY", Vector2Int(3, -3), station, room1);
    Tile::CreateTile("FRAME", Vector2Int(0, -5), station);
    Tile::CreateTile("FRAME", Vector2Int(0, -6), station);
    Tile::CreateTile("FRAME", Vector2Int(-1, -6), station);
    Tile::CreateTile("FRAME", Vector2Int(1, -6), station);
    Tile::CreateTile("FRAME", Vector2Int(0, -7), station);
    Tile::CreateTile("FRAME", Vector2Int(-1, -7), station);
    Tile::CreateTile("FRAME", Vector2Int(1, -7), station);
    auto panel1 = Tile::CreateTile("SOLAR_PANEL", Vector2Int(0, -7), station);
    auto panel2 = Tile::CreateTile("SOLAR_PANEL", Vector2Int(-1, -7), station);
    auto panel3 = Tile::CreateTile("SOLAR_PANEL", Vector2Int(1, -7), station);

    PowerConnectorComponent::AddConnection(battery->GetComponent<PowerConnectorComponent>(), panel1->GetComponent<PowerConnectorComponent>());
    PowerConnectorComponent::AddConnection(battery->GetComponent<PowerConnectorComponent>(), panel2->GetComponent<PowerConnectorComponent>());
    PowerConnectorComponent::AddConnection(battery->GetComponent<PowerConnectorComponent>(), panel3->GetComponent<PowerConnectorComponent>());
    PowerConnectorComponent::AddConnection(battery->GetComponent<PowerConnectorComponent>(), oxygenProducer1->GetComponent<PowerConnectorComponent>());
    PowerConnectorComponent::AddConnection(battery->GetComponent<PowerConnectorComponent>(), oxygenProducer2->GetComponent<PowerConnectorComponent>());

    station->UpdateSpriteOffsets();

    station->effects.push_back(std::make_shared<FireEffect>(Vector2Int(12, 0)));
    station->effects.push_back(std::make_shared<FoamEffect>(Vector2Int(13, 0)));

    auto fireAlarm = AudioManager::LoadSoundEffect("../assets/audio/fire_alarm.opus", SoundEffect::Type::EFFECT, false, true, .05);
    std::weak_ptr<SoundEffect> fireAlarmWeak = fireAlarm;
    fireAlarm->onUpdate = [fireAlarmWeak, &station]()
    {
        auto fireAlarm = fireAlarmWeak.lock();
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
                auto decorative = tile->AddComponent<DecorativeComponent>(tile);
                decorative->AddDecorativeTile(Vector2Int(0, -1), Vector2Int(0, 5));
                decorative->AddDecorativeTile(Vector2Int(0, 1), Vector2Int(0, 6));
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
    effects.erase(std::remove_if(effects.begin(), effects.end(),
                                 [effect](const std::shared_ptr<Effect> &other)
                                 { return effect == other; }),
                  effects.end());
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
                    if (pos == tile->GetPosition() + dTiles.offset)
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
    for (const auto &tile : tilesAtPos)
    {
        if (magic_enum::enum_flags_test_any(tile->GetHeight(), height))
            foundTiles.push_back(tile);
    }

    return foundTiles;
}
