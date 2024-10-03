#include "station.hpp"

void DeleteTile(std::shared_ptr<Tile> tile)
{
    if (!tile)
        return;

    if (tile->GetStation())
    {
        auto &stationTiles = tile->GetStation()->tiles;
        stationTiles.erase(std::remove(stationTiles.begin(), stationTiles.end(), tile), stationTiles.end());
        tile->GetStation()->tileMap.erase(tile->GetPosition());
    }

    if (tile->GetRoom())
    {
        auto &roomTiles = tile->GetRoom()->tiles;
        roomTiles.erase(std::remove_if(roomTiles.begin(), roomTiles.end(),
                                       [&tile](const std::weak_ptr<Tile> &weakTile)
                                       {
                                           if (auto sharedTile = weakTile.lock())
                                               return sharedTile == tile;
                                           return true; }),
                        roomTiles.end());
    }
}

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

            if (oldTile && !isWall)
                DeleteTile(oldTile);

            Tile::CreateTile(isWall ? "WALL" : "BLUE_FLOOR", pos, station, room);
        }
    }

    return room;
}

std::shared_ptr<Station> CreateStation()
{
    std::shared_ptr<Station> station = std::make_shared<Station>();
    std::shared_ptr<Room> room1 = CreateRectRoom(Vector2Int(-4, -4), Vector2Int(9, 9), station);
    std::shared_ptr<Room> room2 = CreateRectRoom(Vector2Int(10, -4), Vector2Int(9, 9), station);
    CreateHorizontalCorridor(Vector2Int(4, 0), 7, 3, station);

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
    return station;
}

void Station::UpdateSpriteOffsets()
{
    for (std::shared_ptr<Tile> tile : tiles)
    {
        const std::shared_ptr<TileDef> &tileDef = tile->GetTileDefinition();

        tile->RemoveComponent<DecorativeTile>();

        std::shared_ptr<Tile> rTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(1, 0));
        bool right = rTile && rTile->GetTileDefinition()->GetId() == tileDef->GetId();
        std::shared_ptr<Tile> lTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(-1, 0));
        bool left = lTile && lTile->GetTileDefinition()->GetId() == tileDef->GetId();
        std::shared_ptr<Tile> dTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(0, 1));
        bool down = dTile && dTile->GetTileDefinition()->GetId() == tileDef->GetId();
        std::shared_ptr<Tile> uTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(0, -1));
        bool up = uTile && uTile->GetTileDefinition()->GetId() == tileDef->GetId();

        const std::string &tileId = tile->GetTileDefinition()->GetId();
        if (tileId == "BLUE_FLOOR")
        {
            if (right && left && !down && !up)
                tile->SetSpriteOffset(Vector2Int(1, 0));
            else if (!right && !left && down && up)
                tile->SetSpriteOffset(Vector2Int(2, 0));
            else if (right && !left && down && !up)
                tile->SetSpriteOffset(Vector2Int(5, 0));
            else if (right && left && down && !up)
                tile->SetSpriteOffset(Vector2Int(6, 0));
            else if (!right && left && down && !up)
                tile->SetSpriteOffset(Vector2Int(7, 0));
            else if (right && !left && down && up)
                tile->SetSpriteOffset(Vector2Int(5, 1));
            else if (right && left && down && up)
            {
                std::shared_ptr<Tile> rdTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(1, 1));
                bool rd = rdTile && rdTile->GetTileDefinition()->GetId() == tileDef->GetId();
                std::shared_ptr<Tile> ldTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(-1, 1));
                bool ld = ldTile && ldTile->GetTileDefinition()->GetId() == tileDef->GetId();
                std::shared_ptr<Tile> ruTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(1, -1));
                bool ru = ruTile && ruTile->GetTileDefinition()->GetId() == tileDef->GetId();
                std::shared_ptr<Tile> luTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(-1, -1));
                bool lu = luTile && luTile->GetTileDefinition()->GetId() == tileDef->GetId();

                if (rd && ld && !ru && lu)
                    tile->SetSpriteOffset(Vector2Int(0, 2));
                else if (rd && ld && ru && !lu)
                    tile->SetSpriteOffset(Vector2Int(1, 2));
                else if (!rd && ld && ru && lu)
                    tile->SetSpriteOffset(Vector2Int(2, 2));
                else if (rd && !ld && ru && lu)
                    tile->SetSpriteOffset(Vector2Int(3, 2));
                else if (rd && !ld && ru && !lu)
                    tile->SetSpriteOffset(Vector2Int(0, 1));
                else if (!rd && ld && !ru && lu)
                    tile->SetSpriteOffset(Vector2Int(1, 1));
                else if (rd && ld && !ru && !lu)
                    tile->SetSpriteOffset(Vector2Int(2, 1));
                else if (rd && ld && !ru && !lu)
                    tile->SetSpriteOffset(Vector2Int(3, 1));
                else
                    tile->SetSpriteOffset(Vector2Int(6, 1));
            }
            else if (!right && left && down && up)
                tile->SetSpriteOffset(Vector2Int(7, 1));
            else if (right && !left && !down && up)
                tile->SetSpriteOffset(Vector2Int(5, 2));
            else if (right && left && !down && up)
                tile->SetSpriteOffset(Vector2Int(6, 2));
            else if (!right && left && !down && up)
                tile->SetSpriteOffset(Vector2Int(7, 2));
            else
                tile->SetSpriteOffset(Vector2Int(0, 0));
        }
        else if (tileId == "WALL")
        {
            if (!right && !left && !down && !up)
                tile->SetSpriteOffset(Vector2Int(3, 4));
            else if (right && !left && !down && !up)
                tile->SetSpriteOffset(Vector2Int(0, 3));
            else if (!right && left && !down && !up)
                tile->SetSpriteOffset(Vector2Int(1, 3));
            else if (!right && !left && down && !up)
                tile->SetSpriteOffset(Vector2Int(2, 3));
            else if (!right && !left && !down && up)
                tile->SetSpriteOffset(Vector2Int(3, 3));
            else if (right && !left && down && !up)
            {
                if (!uTile)
                {
                    tile->SetSpriteOffset(Vector2Int(2, 4));
                    auto dComp = tile->AddComponent<DecorativeComponent>(tile);
                    dComp->AddDecorativeTile(Vector2Int(0, -1), Vector2Int(5, 4));
                }
                else
                    tile->SetSpriteOffset(Vector2Int(5, 4));
            }
            else if (right && left && down && !up)
                tile->SetSpriteOffset(Vector2Int(6, 4));
            else if (!right && left && down && !up)
            {
                if (!uTile)
                {
                    tile->SetSpriteOffset(Vector2Int(2, 4));
                    auto dComp = tile->AddComponent<DecorativeComponent>(tile);
                    dComp->AddDecorativeTile(Vector2Int(0, -1), Vector2Int(7, 4));
                }
                else
                    tile->SetSpriteOffset(Vector2Int(7, 4));
            }
            else if (right && left && !down && !up)
            {
                if (!uTile)
                {
                    tile->SetSpriteOffset(Vector2Int(4, 1));
                    auto dComp = tile->AddComponent<DecorativeComponent>(tile);
                    dComp->AddDecorativeTile(Vector2Int(0, -1), Vector2Int(0, 4));
                }
                else
                    tile->SetSpriteOffset(Vector2Int(0, 4));
            }
            else if (!right && !left && down && up)
            {
                std::shared_ptr<Tile> rdTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(1, 1));
                std::shared_ptr<Tile> ldTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(-1, 1));
                std::shared_ptr<Tile> ddTile = GetTileAtPosition(tile->GetPosition() + Vector2Int(0, 2));

                if (ddTile && ddTile->GetTileDefinition()->GetId() != tileDef->GetId())
                {
                    if (rdTile && rdTile->GetTileDefinition()->GetId() == tileDef->GetId() && ldTile && ldTile->GetTileDefinition()->GetId() == tileDef->GetId())
                        tile->SetSpriteOffset(Vector2Int(6, 6));
                    else if (rdTile && rdTile->GetTileDefinition()->GetId() == tileDef->GetId() && (!ldTile || ldTile->GetTileDefinition()->GetId() != tileDef->GetId()))
                        tile->SetSpriteOffset(Vector2Int(5, 6));
                    else if ((!rdTile || rdTile->GetTileDefinition()->GetId() != tileDef->GetId()) && ldTile && ldTile->GetTileDefinition()->GetId() == tileDef->GetId())
                        tile->SetSpriteOffset(Vector2Int(7, 6));
                    else
                        tile->SetSpriteOffset(Vector2Int(2, 4));
                }
                else
                    tile->SetSpriteOffset(Vector2Int(2, 4));
            }
            else if (right && !left && down && up)
                tile->SetSpriteOffset(Vector2Int(5, 5));
            else if (right && left && down && up)
                tile->SetSpriteOffset(Vector2Int(6, 5));
            else if (!right && left && down && up)
                tile->SetSpriteOffset(Vector2Int(7, 5));
            else if (right && !left && !down && up)
            {
                if (dTile)
                    tile->SetSpriteOffset(Vector2Int(4, 1));
                else
                    tile->SetSpriteOffset(Vector2Int(5, 6));
            }
            else if (right && left && !down && up)
                tile->SetSpriteOffset(Vector2Int(6, 6));
            else if (!right && left && !down && up)
            {
                if (dTile)
                    tile->SetSpriteOffset(Vector2Int(4, 1));
                else
                    tile->SetSpriteOffset(Vector2Int(7, 6));
            }

            if (!dTile)
            {
                auto dComp = tile->AddComponent<DecorativeComponent>(tile);
                dComp->AddDecorativeTile(Vector2Int(0, 1), Vector2Int(Vector2IntToRandomInt(tile->GetPosition(), 4, 5), 3));
                dComp->AddDecorativeTile(Vector2Int(0, 2), Vector2Int(Vector2IntToRandomInt(tile->GetPosition(), 6, 7), 3));
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
    }
}

std::shared_ptr<Tile> Station::GetTileAtPosition(const Vector2Int &pos, TileDef::Height height) const
{
    auto posIt = tileMap.find(pos);
    if (posIt != tileMap.end())
    {
        if (height == TileDef::Height::NONE)
        {
            if (!posIt->second.empty())
            {
                return posIt->second.begin()->second;
            }
        }
        else
        {
            for (const auto &[tileHeight, tilePtr] : posIt->second)
            {
                if ((magic_enum::enum_integer(tileHeight) & magic_enum::enum_integer(height)) != 0)
                {
                    return tilePtr;
                }
            }
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<Tile>> Station::GetTilesAtPosition(const Vector2Int &pos) const
{
    std::vector<std::shared_ptr<Tile>> result;

    auto posIt = tileMap.find(pos);
    if (posIt != tileMap.end())
    {
        for (const auto &pair : posIt->second)
        {
            result.push_back(pair.second);
        }
    }

    return result;
}