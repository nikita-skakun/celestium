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
        auto &tilesAtPos = station->tileMap[position];
        for (const std::shared_ptr<Tile> &existingTile : tilesAtPos)
        {
            if (!existingTile)
                continue;
            if (magic_enum::enum_integer(existingTile->GetHeight() & tile->GetHeight()) > 0)
            {
                throw std::runtime_error(std::format("A tile {} already exists at {} with overlapping height.", existingTile->GetName(), ToString(position)));
                return nullptr;
            }
        }

        tilesAtPos.push_back(tile);
        std::sort(tilesAtPos.begin(), tilesAtPos.end(), Tile::CompareByHeight);

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
        auto &tilesAtPos = station->tileMap[position];
        tilesAtPos.erase(std::remove_if(tilesAtPos.begin(), tilesAtPos.end(),
                                        [self = shared_from_this()](const std::shared_ptr<Tile> &tile)
                                        { return tile == self; }),
                         tilesAtPos.end());
    }

    if (room)
    {
        auto &roomTiles = room->tiles;
        roomTiles.erase(std::remove_if(roomTiles.begin(), roomTiles.end(),
                                       [self = shared_from_this()](const std::weak_ptr<Tile> &weakTile)
                                       { if (auto sharedTile = weakTile.lock())
                                               return sharedTile == self;
                                           return true; }),
                        roomTiles.end());
    }

    components.clear();
}
