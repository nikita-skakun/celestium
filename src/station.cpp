#include "station.h"

const std::string Tile::names[int(ID::ID_LENGTH)] = {
    "Blue Floor",
    "Wall",
};

std::shared_ptr<Tile> CreateTile(Tile::Type type, const Vector2 &position, Tile::ID id, const std::shared_ptr<Station> &station, const std::shared_ptr<Room> &room = nullptr, const std::shared_ptr<Component> &component = nullptr)
{
    std::shared_ptr<Tile> tile;
    switch (type)
    {
    case Tile::Type::DEFAULT:
        tile = std::make_shared<Tile>(position, id, station, room, component);
        break;

    case Tile::Type::FLOOR:
        tile = std::make_shared<FloorTile>(position, id, station, room, component);
        break;

    default:
        return nullptr;
    }

    if (station)
    {
        station->tiles.push_back(tile);
        station->tileMap.insert(std::make_pair(position, tile));
    }

    if (room)
        room->tiles.push_back(tile);

    if (component)
        component->tiles.push_back(tile);

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

    if (tile->component)
    {
        auto &componentTiles = tile->component->tiles;
        componentTiles.erase(std::remove(componentTiles.begin(), componentTiles.end(), tile), componentTiles.end());
    }
}

std::shared_ptr<Room> CreateEmptyRoom(const std::shared_ptr<Station> &station)
{
    std::shared_ptr<Room> room = std::make_shared<Room>(station);

    if (station)
        station->rooms.push_back(room);

    return room;
}

std::shared_ptr<Room> CreateRectRoom(const Vector2 &pos, const Vector2Int &size, const std::shared_ptr<Station> &station)
{
    std::shared_ptr<Room> room = CreateEmptyRoom(station);

    for (int y = 0; y < size.y; y++)
    {
        for (int x = 0; x < size.x; x++)
        {
            if (x == 0 || y == 0 || x == size.x - 1 || y == size.y - 1)
            {
                CreateTile(Tile::Type::DEFAULT, pos + Vector2(x, y), Tile::ID::WALL, station, room);
            }
            else
            {
                CreateTile(Tile::Type::FLOOR, pos + Vector2(x, y), Tile::ID::BLUE_FLOOR, station, room);
            }
        }
    }

    return room;
}

std::shared_ptr<Room> CreateHorizontalCorridor(const Vector2 &startPos, int length, const std::shared_ptr<Station> &station)
{
    std::shared_ptr<Room> room = CreateEmptyRoom(station);

    for (int i = 0; i < length; i++)
    {
        for (int y = -1; y <= 1; y++)
        {
            auto oldTile = station->GetTileAtPosition(startPos + Vector2(i, y));
            if (oldTile)
            {
                if (y == 0)
                {
                    DeleteTile(oldTile);
                    CreateTile(Tile::Type::FLOOR, startPos + Vector2(i, y), Tile::ID::BLUE_FLOOR, station, room);
                    // Add door later
                }
            }
            else
            {
                if (y == 0)
                {
                    CreateTile(Tile::Type::FLOOR, startPos + Vector2(i, y), Tile::ID::BLUE_FLOOR, station, room);
                }
                else
                {
                    CreateTile(Tile::Type::DEFAULT, startPos + Vector2(i, y), Tile::ID::WALL, station, room);
                }
            }
        }
    }

    return room;
}

std::shared_ptr<Station> CreateStation()
{
    std::shared_ptr<Station> station = std::make_shared<Station>();
    CreateRectRoom(Vector2(-4, -4), Vector2Int(9, 9), station);
    CreateRectRoom(Vector2(10, -4), Vector2Int(9, 9), station);
    CreateHorizontalCorridor(Vector2(4, 0), 7, station);

    station->UpdateSpriteOffsets();
    return station;
}

void Station::UpdateSpriteOffsets()
{
    for (std::shared_ptr<Tile> tile : tiles)
    {
        tile->verticalTiles.clear();

        std::shared_ptr<Tile> rTile = GetTileAtPosition(tile->position + Vector2(1, 0));
        bool right = rTile && rTile->id == tile->id;
        std::shared_ptr<Tile> lTile = GetTileAtPosition(tile->position + Vector2(-1, 0));
        bool left = lTile && lTile->id == tile->id;
        std::shared_ptr<Tile> dTile = GetTileAtPosition(tile->position + Vector2(0, 1));
        bool down = dTile && dTile->id == tile->id;
        std::shared_ptr<Tile> uTile = GetTileAtPosition(tile->position + Vector2(0, -1));
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
                std::shared_ptr<Tile> rdTile = GetTileAtPosition(tile->position + Vector2(1, 1));
                bool rd = rdTile && rdTile->id == tile->id;
                std::shared_ptr<Tile> ldTile = GetTileAtPosition(tile->position + Vector2(-1, 1));
                bool ld = ldTile && ldTile->id == tile->id;
                std::shared_ptr<Tile> ruTile = GetTileAtPosition(tile->position + Vector2(1, -1));
                bool ru = ruTile && ruTile->id == tile->id;
                std::shared_ptr<Tile> luTile = GetTileAtPosition(tile->position + Vector2(-1, -1));
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
                    tile->verticalTiles.push_back(std::make_shared<VerticalTile>(Vector2(0, -1), Vector2Int(5, 3)));
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
                    tile->verticalTiles.push_back(std::make_shared<VerticalTile>(Vector2(0, -1), Vector2Int(7, 3)));
                }
                else
                    tile->spriteOffset = Vector2Int(7, 3);
            }
            else if (right && left && !down && !up)
            {
                if (!uTile)
                {
                    tile->spriteOffset = Vector2Int(4, 1);
                    tile->verticalTiles.push_back(std::make_shared<VerticalTile>(Vector2(0, -1), Vector2Int(0, 4)));
                }
                else
                    tile->spriteOffset = Vector2Int(0, 4);
            }
            else if (!right && !left && down && up)
            {
                std::shared_ptr<Tile> rdTile = GetTileAtPosition(tile->position + Vector2(1, 1));
                std::shared_ptr<Tile> ldTile = GetTileAtPosition(tile->position + Vector2(-1, 1));
                std::shared_ptr<Tile> ddTile = GetTileAtPosition(tile->position + Vector2(0, 2));

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
                tile->verticalTiles.push_back(std::make_shared<VerticalTile>(Vector2(0, 1), Vector2Int(Vector2ToRandomInt(tile->position, 0, 1), 2)));
                tile->verticalTiles.push_back(std::make_shared<VerticalTile>(Vector2(0, 2), Vector2Int(Vector2ToRandomInt(tile->position, 2, 3), 2)));
            }
            break;
        }
        default:
            break;
        }
    }
}

std::shared_ptr<Tile> Station::GetTileAtPosition(const Vector2 &pos) const
{
    auto tileIter = tileMap.find(pos);
    if (tileIter != tileMap.end())
        return tileIter->second;

    return nullptr;
}
