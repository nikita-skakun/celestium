#include "station.hpp"

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
    int start = -(int)floor(totalWidth / 2.f);
    int end = (int)ceil(totalWidth / 2.f);

    int direction = (length > 0) ? 1 : -1;
    int absLength = std::abs(length);

    for (int i = 0; i < absLength; i++)
    {
        for (int y = start; y < end; y++)
        {
            bool isWall = (y == start || y == end - 1);
            Vector2Int pos = startPos + Vector2Int(i * direction, y);
            std::shared_ptr<Tile> oldTile = station->GetTileAtPosition(pos);
            if (isWall && oldTile)
                continue;

            if (!isWall && oldTile)
                oldTile->DeleteTile();

            Tile::CreateTile(isWall ? "WALL" : "BLUE_FLOOR", pos, station, room);

            if (i == 0 || i == absLength - 1)
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
    std::shared_ptr<Room> room3 = CreateHorizontalCorridor(Vector2Int(4, 0), 7, 1, station);

    auto oxygenProducer1 = Tile::CreateTile("OXYGEN_PRODUCER", Vector2Int(0, 0), station, room1);
    auto oxygenProducer2 = Tile::CreateTile("OXYGEN_PRODUCER", Vector2Int(14, 0), station, room2);
    auto battery = Tile::CreateTile("BATTERY", Vector2Int(3, -3), station, room1);
    Tile::CreateTile("FRAME", Vector2Int(0, -6), station);
    Tile::CreateTile("FRAME", Vector2Int(0, -7), station);
    Tile::CreateTile("FRAME", Vector2Int(-1, -7), station);
    Tile::CreateTile("FRAME", Vector2Int(1, -7), station);
    Tile::CreateTile("FRAME", Vector2Int(0, -8), station);
    Tile::CreateTile("FRAME", Vector2Int(-1, -8), station);
    Tile::CreateTile("FRAME", Vector2Int(1, -8), station);
    auto panel1 = Tile::CreateTile("SOLAR_PANEL", Vector2Int(0, -8), station);
    auto panel2 = Tile::CreateTile("SOLAR_PANEL", Vector2Int(-1, -8), station);
    auto panel3 = Tile::CreateTile("SOLAR_PANEL", Vector2Int(1, -8), station);

    PowerConnectorComponent::AddConnection(battery->GetComponent<PowerConnectorComponent>(), panel1->GetComponent<PowerConnectorComponent>());
    PowerConnectorComponent::AddConnection(battery->GetComponent<PowerConnectorComponent>(), panel2->GetComponent<PowerConnectorComponent>());
    PowerConnectorComponent::AddConnection(battery->GetComponent<PowerConnectorComponent>(), panel3->GetComponent<PowerConnectorComponent>());
    PowerConnectorComponent::AddConnection(battery->GetComponent<PowerConnectorComponent>(), oxygenProducer1->GetComponent<PowerConnectorComponent>());
    PowerConnectorComponent::AddConnection(battery->GetComponent<PowerConnectorComponent>(), oxygenProducer2->GetComponent<PowerConnectorComponent>());

    station->UpdateSpriteOffsets();

    station->effects.push_back(std::make_shared<FireEffect>(Vector2Int(12, 0), FireEffect::SIZE_INCREMENT));
    station->effects.push_back(std::make_shared<FoamEffect>(Vector2Int(13, 0), 1));

    return station;
}

void Station::UpdateSpriteOffsets() const
{
    for (const auto &tilesAtPos : tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            const Vector2Int &tilePos = tile->GetPosition();
            const std::string &tileId = tile->GetId();

            tile->RemoveComponent<DecorativeTile>();

            bool nSame = CheckAdjacentTile(tilePos, tileId, Direction::N);
            bool eSame = CheckAdjacentTile(tilePos, tileId, Direction::E);
            bool sSame = CheckAdjacentTile(tilePos, tileId, Direction::S);
            bool wSame = CheckAdjacentTile(tilePos, tileId, Direction::W);

            bool nExists = GetTileIdAtPosition(tilePos + Vector2Int(0, -1)) != "";
            bool sExists = GetTileIdAtPosition(tilePos + Vector2Int(0, 1)) != "";

            if (tileId == "BLUE_FLOOR")
            {
                if (eSame && wSame && !sSame && !nSame)
                    tile->SetSpriteOffset(Vector2Int(1, 0));
                else if (!eSame && !wSame && sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(2, 0));
                else if (eSame && !wSame && sSame && !nSame)
                    tile->SetSpriteOffset(Vector2Int(5, 0));
                else if (eSame && wSame && sSame && !nSame)
                    tile->SetSpriteOffset(Vector2Int(6, 0));
                else if (!eSame && wSame && sSame && !nSame)
                    tile->SetSpriteOffset(Vector2Int(7, 0));
                else if (eSame && !wSame && sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(5, 1));
                else if (eSame && wSame && sSame && nSame)
                {
                    bool neSame = CheckAdjacentTile(tilePos, tileId, Direction::N | Direction::E);
                    bool seSame = CheckAdjacentTile(tilePos, tileId, Direction::S | Direction::E);
                    bool nwSame = CheckAdjacentTile(tilePos, tileId, Direction::N | Direction::W);
                    bool swSame = CheckAdjacentTile(tilePos, tileId, Direction::S | Direction::W);

                    if (seSame && swSame && !neSame && nwSame)
                        tile->SetSpriteOffset(Vector2Int(0, 2));
                    else if (seSame && swSame && neSame && !nwSame)
                        tile->SetSpriteOffset(Vector2Int(1, 2));
                    else if (!seSame && swSame && neSame && nwSame)
                        tile->SetSpriteOffset(Vector2Int(2, 2));
                    else if (seSame && !swSame && neSame && nwSame)
                        tile->SetSpriteOffset(Vector2Int(3, 2));
                    else if (seSame && !swSame && neSame && !nwSame)
                        tile->SetSpriteOffset(Vector2Int(0, 1));
                    else if (!seSame && swSame && !neSame && nwSame)
                        tile->SetSpriteOffset(Vector2Int(1, 1));
                    else if (seSame && swSame && !neSame && !nwSame)
                        tile->SetSpriteOffset(Vector2Int(2, 1));
                    else if (seSame && swSame && !neSame && !nwSame)
                        tile->SetSpriteOffset(Vector2Int(3, 1));
                    else
                        tile->SetSpriteOffset(Vector2Int(6, 1));
                }
                else if (!eSame && wSame && sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(7, 1));
                else if (eSame && !wSame && !sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(5, 2));
                else if (eSame && wSame && !sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(6, 2));
                else if (!eSame && wSame && !sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(7, 2));
                else
                    tile->SetSpriteOffset(Vector2Int(0, 0));
            }
            else if (tileId == "WALL")
            {
                if (!eSame && !wSame && !sSame && !nSame)
                    tile->SetSpriteOffset(Vector2Int(3, 4));
                else if (eSame && !wSame && !sSame && !nSame)
                    tile->SetSpriteOffset(Vector2Int(0, 3));
                else if (!eSame && wSame && !sSame && !nSame)
                    tile->SetSpriteOffset(Vector2Int(1, 3));
                else if (!eSame && !wSame && sSame && !nSame)
                    tile->SetSpriteOffset(Vector2Int(2, 3));
                else if (!eSame && !wSame && !sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(3, 3));
                else if (eSame && !wSame && sSame && !nSame)
                {
                    if (!nExists)
                    {
                        tile->SetSpriteOffset(Vector2Int(2, 4));
                        auto dComp = tile->AddComponent<DecorativeComponent>(tile);
                        dComp->AddDecorativeTile(Vector2Int(0, -1), Vector2Int(5, 4));
                    }
                    else
                        tile->SetSpriteOffset(Vector2Int(5, 4));
                }
                else if (eSame && wSame && sSame && !nSame)
                    tile->SetSpriteOffset(Vector2Int(6, 4));
                else if (!eSame && wSame && sSame && !nSame)
                {
                    if (!nExists)
                    {
                        tile->SetSpriteOffset(Vector2Int(2, 4));
                        auto dComp = tile->AddComponent<DecorativeComponent>(tile);
                        dComp->AddDecorativeTile(Vector2Int(0, -1), Vector2Int(7, 4));
                    }
                    else
                        tile->SetSpriteOffset(Vector2Int(7, 4));
                }
                else if (eSame && wSame && !sSame && !nSame)
                {
                    if (!nExists)
                    {
                        tile->SetSpriteOffset(Vector2Int(4, 1));
                        auto dComp = tile->AddComponent<DecorativeComponent>(tile);
                        dComp->AddDecorativeTile(Vector2Int(0, -1), Vector2Int(0, 4));
                    }
                    else
                        tile->SetSpriteOffset(Vector2Int(0, 4));
                }
                else if (!eSame && !wSame && sSame && nSame)
                {
                    std::string ssId = GetTileIdAtPosition(tilePos + Vector2Int(0, 2));
                    bool seSame = GetTileIdAtPosition(tilePos + Vector2Int(1, 1)) == tileId;
                    bool swSame = GetTileIdAtPosition(tilePos + Vector2Int(-1, 1)) == tileId;

                    if (ssId != "" && ssId != tileId)
                    {
                        if (seSame && swSame)
                            tile->SetSpriteOffset(Vector2Int(6, 6));
                        else if (seSame && !swSame)
                            tile->SetSpriteOffset(Vector2Int(5, 6));
                        else if (!seSame && swSame)
                            tile->SetSpriteOffset(Vector2Int(7, 6));
                        else
                            tile->SetSpriteOffset(Vector2Int(2, 4));
                    }
                    else
                        tile->SetSpriteOffset(Vector2Int(2, 4));
                }
                else if (eSame && !wSame && sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(5, 5));
                else if (eSame && wSame && sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(6, 5));
                else if (!eSame && wSame && sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(7, 5));
                else if (eSame && !wSame && !sSame && nSame)
                {
                    if (sExists)
                        tile->SetSpriteOffset(Vector2Int(4, 1));
                    else
                        tile->SetSpriteOffset(Vector2Int(5, 6));
                }
                else if (eSame && wSame && !sSame && nSame)
                    tile->SetSpriteOffset(Vector2Int(6, 6));
                else if (!eSame && wSame && !sSame && nSame)
                {
                    if (sExists)
                        tile->SetSpriteOffset(Vector2Int(4, 1));
                    else
                        tile->SetSpriteOffset(Vector2Int(7, 6));
                }

                if (!sExists)
                {
                    auto dComp = tile->AddComponent<DecorativeComponent>(tile);
                    dComp->AddDecorativeTile(Vector2Int(0, 1), Vector2Int(Vector2IntToRandomInt(tilePos, 4, 5), 3));
                    dComp->AddDecorativeTile(Vector2Int(0, 2), Vector2Int(Vector2IntToRandomInt(tilePos, 6, 7), 3));
                }
            }
            else if (tileId == "OXYGEN_PRODUCER")
            {
                tile->SetSpriteOffset(Vector2Int(6, 7));
            }
            else if (tileId == "BATTERY")
            {
                tile->SetSpriteOffset(Vector2Int(5, 7));
            }
            else if (tileId == "SOLAR_PANEL")
            {
                tile->SetSpriteOffset(Vector2Int(7, 7));
            }
            else if (tileId == "FRAME")
            {
                tile->SetSpriteOffset(Vector2Int(4, 4));
            }
            else if (tileId == "DOOR")
            {
                tile->SetSpriteOffset(Vector2Int(0, 6));
                auto decorative = tile->AddComponent<DecorativeComponent>(tile);
                decorative->AddDecorativeTile(Vector2Int(0, -1), Vector2Int(0, 5));
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
