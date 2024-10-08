#include "tile.hpp"
#include "station.hpp"

Tile::Tile(const std::string &tileId, const Vector2Int &position, std::shared_ptr<Station> station, std::shared_ptr<Room> room)
    : position(position), room(room), station(station)
{
    tileDef = TileDefinitionRegistry::GetInstance().GetTileDefinition(tileId);
}

std::shared_ptr<Tile> Tile::CreateTile(const std::string &tileId, const Vector2Int &position, std::shared_ptr<Station> station, std::shared_ptr<Room> room)
{
    std::shared_ptr<Tile> tile = std::make_shared<Tile>(Tile(tileId, position, station, room));

    const auto &refComponents = tile->GetTileDefinition()->GetReferenceComponents();
    tile->components.reserve(refComponents.size());

    for (const auto &refComponent : refComponents)
    {
        tile->components.insert(refComponent->Clone(tile));
    }

    if (station)
    {
        auto &heightMap = station->tileMap[position];
        for (const auto &[existingHeight, existingTile] : heightMap)
        {
            if (magic_enum::enum_integer(existingHeight & tile->GetTileDefinition()->GetHeight()) > 0)
            {
                throw std::runtime_error(std::format("A tile {} already exists at {} with overlapping height.", existingTile->GetName(), ToString(position)));
                return nullptr;
            }
        }

        station->tiles.push_back(tile);
        heightMap[tile->GetTileDefinition()->GetHeight()] = tile;

        if (auto door = tile->GetComponent<DoorComponent>())
            door->SetOpenState(door->IsOpen());
    }

    if (room)
        room->tiles.push_back(tile);

    return tile;
}

void Tile::DeleteTile()
{
    if (station)
    {
        auto &stationTiles = station->tiles;
        stationTiles.erase(std::remove(stationTiles.begin(), stationTiles.end(), shared_from_this()), stationTiles.end());
        station->tileMap.erase(position);
    }

    if (room)
    {
        auto &roomTiles = room->tiles;
        roomTiles.erase(std::remove_if(roomTiles.begin(), roomTiles.end(),
                                       [self = shared_from_this()](const std::weak_ptr<Tile> &weakTile)
                                       {
                                           if (auto sharedTile = weakTile.lock())
                                               return sharedTile == self;
                                           return true; }),
                        roomTiles.end());
    }

    components.clear();
}
