#include "asset_manager.hpp"
#include "component.hpp"
#include "game_state.hpp"
#include "power_grid.hpp"
#include "sprite.hpp"
#include "station.hpp"
#include "tile.hpp"

void BasicSprite::Draw(const Vector2Int &position, const Color &tint, float rotation) const
{
    Vector2 screenPos = GameManager::WorldToScreen(position + offsetFromMainTile);
    Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * GameManager::GetCamera().GetZoom();
    Rectangle sourceRect = Rectangle(spriteOffset.x, spriteOffset.y, 1, 1) * TILE_SIZE;

    DrawTexturePro(AssetManager::GetTexture("STATION"), sourceRect, Vector2ToRect(screenPos, tileSize), tileSize / 2., rotation, tint);
}

void MultiSliceSprite::Draw(const Vector2Int &position, const Color &tint, float rotation) const
{
    Vector2 screenPos = GameManager::WorldToScreen(position + offsetFromMainTile);

    for (const auto &slice : slices)
    {
        if (slice.sourceRect.width <= 0 || slice.sourceRect.height <= 0)
            continue;

        Rectangle destRect = Vector2ToRect(slice.destOffset, RectToSize(slice.sourceRect)) * GameManager::GetCamera().GetZoom();
        Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * GameManager::GetCamera().GetZoom();
        destRect.x += screenPos.x;
        destRect.y += screenPos.y;

        DrawTexturePro(AssetManager::GetTexture("STATION"), slice.sourceRect, destRect, tileSize / 2., rotation, tint);
    }
}

Tile::Tile(const std::string &tileId, const Vector2Int &position, const std::shared_ptr<Station> &station)
    : position(position), station(station)
{
    tileDef = DefinitionManager::GetTileDefinition(tileId);
}

std::shared_ptr<Tile> Tile::CreateTile(const std::string &tileId, const Vector2Int &position, const std::shared_ptr<Station> &station)
{
    std::shared_ptr<Tile> tile = std::make_shared<Tile>(Tile(tileId, position, station));

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
            // Check if the existing tile and the new tile have overlapping heights
            if (existingTile && (magic_enum::enum_integer(existingTile->GetHeight() & tile->GetHeight()) > 0))
                throw std::runtime_error(std::format("A tile {} already exists at {} with overlapping height.", existingTile->GetName(), ToString(position)));
        }

        tilesAtPos.push_back(tile);
        std::sort(tilesAtPos.begin(), tilesAtPos.end(), Tile::CompareByHeight);

        if (magic_enum::enum_flags_test_any(tile->GetHeight(), TileDef::Height::POWER))
            station->RebuildPowerGridsFromInfrastructure();

        if (auto powerConnector = tile->GetComponent<PowerConnectorComponent>())
        {
            if (auto powerWireTile = station->GetTileWithHeightAtPosition(position, TileDef::Height::POWER))
            {
                if (auto powerWireConnector = powerWireTile->GetComponent<PowerConnectorComponent>())
                {
                    if (auto wireGrid = powerWireConnector->GetPowerGrid())
                    {
                        // Register this tile's components with the found grid
                        if (auto consumer = tile->GetComponent<PowerConsumerComponent>())
                            wireGrid->AddConsumer(position, consumer);
                        if (auto producer = tile->GetComponent<PowerProducerComponent>())
                            wireGrid->AddProducer(position, producer);
                        if (auto battery = tile->GetComponent<BatteryComponent>())
                            wireGrid->AddBattery(position, battery);
                        powerConnector->SetPowerGrid(wireGrid);
                    }
                }
            }
        }

        if (auto door = tile->GetComponent<DoorComponent>())
            door->SetOpenState(door->IsOpen());
    }

    return tile;
}

void Tile::MoveTile(const Vector2Int &newPosition)
{
    if (!station || position == newPosition)
        return;

    auto self = shared_from_this();

    auto &tilesAtOldPos = station->tileMap[position];
    std::erase_if(tilesAtOldPos, [&self](const std::shared_ptr<Tile> &tile)
                  { return tile == self; });

    position = newPosition;
    auto &tilesAtPos = station->tileMap[newPosition];
    tilesAtPos.push_back(self);
    std::sort(tilesAtPos.begin(), tilesAtPos.end(), Tile::CompareByHeight);

    station->UpdateSpriteOffsets();
}

void Tile::RotateTile()
{
    auto rotatable = GetComponent<RotatableComponent>();
    if (!rotatable || !station)
        return;

    rotatable->RotateClockwise();
    station->UpdateSpriteOffsets();
}

void Tile::DeleteTile()
{
    auto self = shared_from_this();

    if (auto powerConnector = GetComponent<PowerConnectorComponent>())
    {
        if (auto powerGrid = powerConnector->GetPowerGrid())
            powerGrid->Disconnect(self);
    }

    if (station)
    {
        auto &tilesAtPos = station->tileMap[position];
        std::erase_if(tilesAtPos, [&self](const std::shared_ptr<Tile> &tile)
                      { return tile == self; });

        if (magic_enum::enum_flags_test_any(GetHeight(), TileDef::Height::POWER))
            station->RebuildPowerGridsFromInfrastructure();

        station->UpdateSpriteOffsets();
    }

    components.clear();
}

bool Tile::IsActive() const
{
    if (auto powerConsumer = GetComponent<PowerConsumerComponent>())
        return powerConsumer->IsActive();

    return true;
}

std::string Tile::GetInfo() const
{
    std::string tileInfo = " - " + GetName();

    for (const auto &component : components)
    {
        if (auto info = component->GetInfo())
            tileInfo += "\n" + info.value();
    }

    return tileInfo;
}

bool Tile::HasComponent(ComponentType type) const
{
    return std::ranges::any_of(components, [&](const auto &component)
                               { return component->GetType() == type; });
}