#include "station.h"

std::shared_ptr<Tile> CreateTile(Tile::ID id, const Vector2Int &position, std::shared_ptr<Station> station, std::shared_ptr<Room> room = nullptr)
{
    std::shared_ptr<Tile> tile;
    switch (id)
    {
    case Tile::ID::BLUE_FLOOR:
        tile = std::make_shared<FloorTile>(id, position, station, room);
        break;

    case Tile::ID::WALL:
        tile = std::make_shared<WallTile>(id, position, station, room);
        break;

    case Tile::ID::OXYGEN_PRODUCER:
    {
        if (!station)
            return nullptr;
        std::shared_ptr<Tile> tileBelow = station->GetTileAtPosition(position, Tile::Height::FLOOR);
        std::shared_ptr<FloorTile> floorTile;

        if (tileBelow && tileBelow->GetType() == Tile::Type::FLOOR)
            floorTile = std::dynamic_pointer_cast<FloorTile>(tileBelow);
        else
            return nullptr;

        tile = std::make_shared<OxygenProducingTile>(id, position, floorTile, station, room);
        break;
    }
    default:
        return nullptr;
    }

    if (station)
    {
        auto &heightMap = station->tileMap[position];
        for (const auto &[existingHeight, existingTile] : heightMap)
        {
            if (magic_enum::enum_integer(existingHeight & tile->GetHeight()) > 0)
            {
                LogMessage(LogLevel::ERROR, "A tile already exists at " + ToString(position) + " with overlapping height.");
                return nullptr;
            }
        }

        station->tiles.push_back(tile);
        heightMap[tile->GetHeight()] = tile;
    }

    if (room)
        room->tiles.push_back(tile);

    return tile;
}

void DeleteTile(std::shared_ptr<Tile> tile)
{
    if (!tile)
        return;

    if (tile->station)
    {
        auto &stationTiles = tile->station->tiles;
        stationTiles.erase(std::remove(stationTiles.begin(), stationTiles.end(), tile), stationTiles.end());
        tile->station->tileMap.erase(tile->position);
    }

    if (tile->room)
    {
        auto &roomTiles = tile->room->tiles;
        roomTiles.erase(std::remove(roomTiles.begin(), roomTiles.end(), tile), roomTiles.end());
    }
}

std::shared_ptr<Room> CreateEmptyRoom(std::shared_ptr<Station> station)
{
    std::shared_ptr<Room> room = std::make_shared<Room>(station);

    if (station)
        station->rooms.push_back(room);

    return room;
}

std::shared_ptr<Room> CreateRectRoom(const Vector2Int &pos, const Vector2Int &size, std::shared_ptr<Station> station)
{
    std::shared_ptr<Room> room = CreateEmptyRoom(station);

    for (int y = 0; y < size.y; y++)
    {
        for (int x = 0; x < size.x; x++)
        {
            if (x == 0 || y == 0 || x == size.x - 1 || y == size.y - 1)
            {
                CreateTile(Tile::ID::WALL, pos + Vector2Int(x, y), station, room);
            }
            else
            {
                CreateTile(Tile::ID::BLUE_FLOOR, pos + Vector2Int(x, y), station, room);
            }
        }
    }

    return room;
}

std::shared_ptr<Room> CreateHorizontalCorridor(const Vector2Int &startPos, int length, std::shared_ptr<Station> station)
{
    std::shared_ptr<Room> room = CreateEmptyRoom(station);

    for (int i = 0; i < length; i++)
    {
        for (int y = -1; y <= 1; y++)
        {
            auto oldTile = station->GetTileAtPosition(startPos + Vector2Int(i, y));
            if (oldTile)
            {
                if (y == 0)
                {
                    DeleteTile(oldTile);
                    CreateTile(Tile::ID::BLUE_FLOOR, startPos + Vector2Int(i, y), station, room);
                    // Add door later
                }
            }
            else
            {
                if (y == 0)
                {
                    CreateTile(Tile::ID::BLUE_FLOOR, startPos + Vector2Int(i, y), station, room);
                }
                else
                {
                    CreateTile(Tile::ID::WALL, startPos + Vector2Int(i, y), station, room);
                }
            }
        }
    }

    return room;
}

std::shared_ptr<Station> CreateStation()
{
    std::shared_ptr<Station> station = std::make_shared<Station>();
    CreateRectRoom(Vector2Int(-4, -4), Vector2Int(9, 9), station);
    std::shared_ptr<Room> room = CreateRectRoom(Vector2Int(10, -4), Vector2Int(9, 9), station);
    CreateHorizontalCorridor(Vector2Int(4, 0), 7, station);

    CreateTile(Tile::ID::OXYGEN_PRODUCER, Vector2Int(14, 0), station, room);

    station->UpdateSpriteOffsets();
    return station;
}

void Station::UpdateSpriteOffsets()
{
    for (std::shared_ptr<Tile> tile : tiles)
    {
        tile->verticalTiles.clear();

        std::shared_ptr<Tile> rTile = GetTileAtPosition(tile->position + Vector2Int(1, 0));
        bool right = rTile && rTile->id == tile->id;
        std::shared_ptr<Tile> lTile = GetTileAtPosition(tile->position + Vector2Int(-1, 0));
        bool left = lTile && lTile->id == tile->id;
        std::shared_ptr<Tile> dTile = GetTileAtPosition(tile->position + Vector2Int(0, 1));
        bool down = dTile && dTile->id == tile->id;
        std::shared_ptr<Tile> uTile = GetTileAtPosition(tile->position + Vector2Int(0, -1));
        bool up = uTile && uTile->id == tile->id;

        switch (tile->id)
        {
        case Tile::ID::BLUE_FLOOR:
        {
            if (right && left && !down && !up)
                tile->spriteOffset = Vector2Int(1, 0);
            else if (!right && !left && down && up)
                tile->spriteOffset = Vector2Int(2, 0);
            else if (right && !left && down && !up)
                tile->spriteOffset = Vector2Int(5, 0);
            else if (right && left && down && !up)
                tile->spriteOffset = Vector2Int(6, 0);
            else if (!right && left && down && !up)
                tile->spriteOffset = Vector2Int(7, 0);
            else if (right && !left && down && up)
                tile->spriteOffset = Vector2Int(5, 1);
            else if (right && left && down && up)
            {
                std::shared_ptr<Tile> rdTile = GetTileAtPosition(tile->position + Vector2Int(1, 1));
                bool rd = rdTile && rdTile->id == tile->id;
                std::shared_ptr<Tile> ldTile = GetTileAtPosition(tile->position + Vector2Int(-1, 1));
                bool ld = ldTile && ldTile->id == tile->id;
                std::shared_ptr<Tile> ruTile = GetTileAtPosition(tile->position + Vector2Int(1, -1));
                bool ru = ruTile && ruTile->id == tile->id;
                std::shared_ptr<Tile> luTile = GetTileAtPosition(tile->position + Vector2Int(-1, -1));
                bool lu = luTile && luTile->id == tile->id;

                if (rd && !ld && ru && !lu)
                    tile->spriteOffset = Vector2Int(0, 1);
                else if (!rd && ld && !ru && lu)
                    tile->spriteOffset = Vector2Int(1, 1);
                else if (rd && ld && !ru && !lu)
                    tile->spriteOffset = Vector2Int(2, 1);
                else if (rd && ld && !ru && !lu)
                    tile->spriteOffset = Vector2Int(3, 1);
                else
                    tile->spriteOffset = Vector2Int(6, 1);
            }
            else if (!right && left && down && up)
                tile->spriteOffset = Vector2Int(7, 1);
            else if (right && !left && !down && up)
                tile->spriteOffset = Vector2Int(5, 2);
            else if (right && left && !down && up)
                tile->spriteOffset = Vector2Int(6, 2);
            else if (!right && left && !down && up)
                tile->spriteOffset = Vector2Int(7, 2);
            else
                tile->spriteOffset = Vector2Int(0, 0);
            break;
        }

        case Tile::ID::WALL:
        {
            if (!right && !left && !down && !up)
                tile->spriteOffset = Vector2Int(0, 3);
            else if (right && !left && !down && !up)
                tile->spriteOffset = Vector2Int(1, 3);
            else if (!right && left && !down && !up)
                tile->spriteOffset = Vector2Int(2, 3);
            else if (!right && !left && down && !up)
                tile->spriteOffset = Vector2Int(3, 3);
            else if (!right && !left && !down && up)
                tile->spriteOffset = Vector2Int(4, 3);
            else if (right && !left && down && !up)
            {
                if (!uTile)
                {
                    tile->spriteOffset = Vector2Int(2, 4);
                    tile->verticalTiles.push_back(std::make_shared<VerticalTile>(Vector2Int(0, -1), Vector2Int(5, 3)));
                }
                else
                    tile->spriteOffset = Vector2Int(5, 3);
            }
            else if (right && left && down && !up)
                tile->spriteOffset = Vector2Int(6, 3);
            else if (!right && left && down && !up)
            {
                if (!uTile)
                {
                    tile->spriteOffset = Vector2Int(2, 4);
                    tile->verticalTiles.push_back(std::make_shared<VerticalTile>(Vector2Int(0, -1), Vector2Int(7, 3)));
                }
                else
                    tile->spriteOffset = Vector2Int(7, 3);
            }
            else if (right && left && !down && !up)
            {
                if (!uTile)
                {
                    tile->spriteOffset = Vector2Int(4, 1);
                    tile->verticalTiles.push_back(std::make_shared<VerticalTile>(Vector2Int(0, -1), Vector2Int(0, 4)));
                }
                else
                    tile->spriteOffset = Vector2Int(0, 4);
            }
            else if (!right && !left && down && up)
            {
                std::shared_ptr<Tile> rdTile = GetTileAtPosition(tile->position + Vector2Int(1, 1));
                std::shared_ptr<Tile> ldTile = GetTileAtPosition(tile->position + Vector2Int(-1, 1));
                std::shared_ptr<Tile> ddTile = GetTileAtPosition(tile->position + Vector2Int(0, 2));

                if (ddTile && ddTile->id != tile->id)
                {
                    if (rdTile && rdTile->id == tile->id && ldTile && ldTile->id == tile->id)
                        tile->spriteOffset = Vector2Int(6, 5);
                    else if (rdTile && rdTile->id == tile->id && (!ldTile || ldTile->id != tile->id))
                        tile->spriteOffset = Vector2Int(5, 5);
                    else if ((!rdTile || rdTile->id != tile->id) && ldTile && ldTile->id == tile->id)
                        tile->spriteOffset = Vector2Int(7, 5);
                    else
                        tile->spriteOffset = Vector2Int(2, 4);
                }
                else
                    tile->spriteOffset = Vector2Int(2, 4);
            }
            else if (right && !left && down && up)
                tile->spriteOffset = Vector2Int(5, 4);
            else if (right && left && down && up)
                tile->spriteOffset = Vector2Int(6, 4);
            else if (!right && left && down && up)
                tile->spriteOffset = Vector2Int(7, 4);
            else if (right && !left && !down && up)
            {
                if (dTile)
                    tile->spriteOffset = Vector2Int(4, 1);
                else
                    tile->spriteOffset = Vector2Int(5, 5);
            }
            else if (right && left && !down && up)
                tile->spriteOffset = Vector2Int(6, 5);
            else if (!right && left && !down && up)
            {
                if (dTile)
                    tile->spriteOffset = Vector2Int(4, 1);
                else
                    tile->spriteOffset = Vector2Int(7, 5);
            }

            if (!dTile)
            {
                tile->verticalTiles.push_back(std::make_shared<VerticalTile>(Vector2Int(0, 1), Vector2Int(Vector2IntToRandomInt(tile->position, 0, 1), 2)));
                tile->verticalTiles.push_back(std::make_shared<VerticalTile>(Vector2Int(0, 2), Vector2Int(Vector2IntToRandomInt(tile->position, 2, 3), 2)));
            }
            break;
        }
        case Tile::ID::OXYGEN_PRODUCER:
            tile->spriteOffset = Vector2Int(6, 7);
            break;
        default:
            break;
        }
    }
}

std::shared_ptr<Tile> Station::GetTileAtPosition(const Vector2Int &pos, Tile::Height height) const
{
    auto posIt = tileMap.find(pos);
    if (posIt != tileMap.end())
    {
        if (height == Tile::Height::NONE)
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
