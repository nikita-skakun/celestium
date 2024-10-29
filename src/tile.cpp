#include "tile.hpp"
#include "station.hpp"
#include "game_state.hpp"
#include "asset_manager.hpp"

void BasicSprite::Draw(const Vector2Int &position, const Color &tint) const
{
    Vector2 screenPos = GameManager::WorldToScreen(position);
    Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * GameManager::GetCamera().GetZoom();
    Rectangle sourceRect = Rectangle(spriteOffset.x, spriteOffset.y, 1, 1) * TILE_SIZE;

    DrawTexturePro(AssetManager::GetTexture("STATION"), sourceRect, Vector2ToRect(screenPos, tileSize), Vector2(), 0, tint);
}

void NineSliceSprite::Draw(const Vector2Int &position, const Color &tint) const
{
    Vector2 screenPos = GameManager::WorldToScreen(position);

    for (const auto &slice : slices)
    {
        if (slice.sourceRect.width <= 0 || slice.sourceRect.height <= 0)
            continue;

        Rectangle destRect = Vector2ToRect(slice.destOffset, RectToSize(slice.sourceRect)) * GameManager::GetCamera().GetZoom();
        destRect.x += screenPos.x;
        destRect.y += screenPos.y;

        DrawTexturePro(AssetManager::GetTexture("STATION"), slice.sourceRect, destRect, Vector2(), 0, tint);
    }
}

Tile::Tile(const std::string &tileId, const Vector2Int &position, std::shared_ptr<Station> station, std::shared_ptr<Room> room)
    : position(position), room(room), station(station)
{
    tileDef = DefinitionManager::GetTileDefinition(tileId);
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
        for (const auto &existingTile : tilesAtPos)
        {
            if (existingTile && (magic_enum::enum_integer(existingTile->GetHeight() & tile->GetHeight()) > 0))
                throw std::runtime_error(std::format("A tile {} already exists at {} with overlapping height.", existingTile->GetName(), ToString(position)));
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
    auto self = shared_from_this();

    if (auto powerConnector = GetComponent<PowerConnectorComponent>())
        powerConnector->DisconnectFromAll();

    if (station)
    {
        auto &tilesAtPos = station->tileMap[position];
        tilesAtPos.erase(std::remove_if(tilesAtPos.begin(), tilesAtPos.end(),
                                        [&self](const std::shared_ptr<Tile> &tile)
                                        { return tile == self; }),
                         tilesAtPos.end());

        station->UpdateSpriteOffsets();
    }

    if (room)
    {
        auto &roomTiles = room->tiles;
        roomTiles.erase(std::remove_if(roomTiles.begin(), roomTiles.end(),
                                       [&self](const std::weak_ptr<Tile> &weakTile)
                                       { if (auto sharedTile = weakTile.lock())
                                               return sharedTile == self;
                                           return true; }),
                        roomTiles.end());
    }

    components.clear();
}
