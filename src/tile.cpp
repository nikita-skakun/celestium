#include "asset_manager.hpp"
#include "camera.hpp"
#include "component.hpp"
#include "def_manager.hpp"
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
    : tileDef(DefinitionManager::GetTileDefinition(tileId)), position(position), station(station) {}

std::shared_ptr<Tile> Tile::CreateTile(const std::string &tileId, const Vector2Int &position, const std::shared_ptr<Station> &station, bool overwriteExisting, bool useResources, Rotation rotation)
{
    if (!station)
        throw std::runtime_error("Cannot create tile without a valid station.");
    auto tileDef = DefinitionManager::GetTileDefinition(tileId);
    if (!tileDef)
        throw std::runtime_error(std::format("Tile definition not found: {}", tileId));

    std::vector<Vector2Int> occupiedPositions = {position};
    for (const auto &cell : tileDef->GetExtraParts())
        if (cell.blocksPlacement)
            occupiedPositions.push_back(position + OffsetWithRotation(rotation, cell.offset));

    if (useResources)
    {
        const auto &requiredResources = tileDef->GetBuildResources();
        if (!station->HasResources(requiredResources))
            return nullptr;
        station->ConsumeResources(requiredResources);
    }

    for (const auto &pos : occupiedPositions)
    {
        for (const auto &existingTile : station->tileMap[pos])
        {
            if (existingTile && (magic_enum::enum_integer(existingTile->GetHeight() & tileDef->GetHeight()) > 0))
            {
                if (overwriteExisting)
                    existingTile->DeleteTile(useResources);
                else
                    return nullptr;
            }
        }
    }

    std::shared_ptr<Tile> tile = std::shared_ptr<Tile>(new Tile(tileId, position, station));
    for (const auto &refComponent : tileDef->GetReferenceComponents())
        tile->components[refComponent->GetType()] = refComponent->Clone(tile);

    if (auto rotatable = tile->GetComponent<RotatableComponent>())
        rotatable->SetRotation(rotation);

    for (const auto &pos : occupiedPositions)
    {
        station->tileMap[pos].push_back(tile);
        std::sort(station->tileMap[pos].begin(), station->tileMap[pos].end(), Tile::CompareByHeight);
    }

    if (magic_enum::enum_flags_test_any(tile->GetHeight(), TileHeight::POWER))
        station->RebuildPowerGridsFromInfrastructure();
    else if (auto powerConnector = tile->GetComponent<PowerConnectorComponent>())
    {
        if (auto powerWireTile = station->GetTileAtPosition(position, TileHeight::POWER))
        {
            if (auto powerWireConnector = powerWireTile->GetComponent<PowerConnectorComponent>())
            {
                if (auto wireGrid = powerWireConnector->GetPowerGrid())
                {
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
    {
        if (door->IsOpen())
            tile->RemoveComponent<SolidComponent>();
        else if (!tile->HasComponent(ComponentType::SOLID))
            tile->AddComponent<SolidComponent>();
    }
    return tile;
}

std::shared_ptr<Tile> Tile::CreatePreviewTile(const std::string &tileId, const Vector2Int &position, const std::shared_ptr<Station> &station)
{
    auto tileDef = DefinitionManager::GetTileDefinition(tileId);
    if (!tileDef)
        return nullptr;

    std::shared_ptr<Tile> tile = std::shared_ptr<Tile>(new Tile(tileId, position, station));
    for (const auto &refComponent : tileDef->GetReferenceComponents())
        tile->components[refComponent->GetType()] = refComponent->Clone(tile);

    return tile;
}

std::vector<Vector2Int> Tile::GetOccupiedPositions() const
{
    std::vector<Vector2Int> pos = {position};
    Rotation r = GetComponent<RotatableComponent>() ? GetComponent<RotatableComponent>()->GetRotation() : Rotation::UP;
    for (const auto &cell : tileDef->GetExtraParts())
        if (cell.blocksPlacement)
            pos.push_back(position + OffsetWithRotation(r, cell.offset));
    return pos;
}

void Tile::MoveTile(const Vector2Int &newPosition)
{
    if (!station || position == newPosition)
        return;
    auto self = shared_from_this();

    for (const auto &pos : GetOccupiedPositions())
        std::erase_if(station->tileMap[pos], [&self](const std::shared_ptr<Tile> &t)
                      { return t == self; });

    position = newPosition;

    for (const auto &pos : GetOccupiedPositions())
    {
        station->tileMap[pos].push_back(self);
        std::sort(station->tileMap[pos].begin(), station->tileMap[pos].end(), Tile::CompareByHeight);
    }
    station->UpdateSpriteOffsets();
}

void Tile::RotateTile()
{
    auto rotatable = GetComponent<RotatableComponent>();
    if (!rotatable || !station)
        return;
    auto self = shared_from_this();

    for (const auto &pos : GetOccupiedPositions())
        std::erase_if(station->tileMap[pos], [&self](const std::shared_ptr<Tile> &t)
                      { return t == self; });

    rotatable->RotateClockwise();

    for (const auto &pos : GetOccupiedPositions())
    {
        station->tileMap[pos].push_back(self);
        std::sort(station->tileMap[pos].begin(), station->tileMap[pos].end(), Tile::CompareByHeight);
    }

    station->UpdateSpriteOffsets();
}

void Tile::DeleteTile(bool returnResources)
{
    auto self = shared_from_this();
    if (auto powerConnector = GetComponent<PowerConnectorComponent>())
        if (auto powerGrid = powerConnector->GetPowerGrid())
            powerGrid->Disconnect(self);

    if (station)
    {
        for (const auto &pos : GetOccupiedPositions())
            std::erase_if(station->tileMap[pos], [&self](const std::shared_ptr<Tile> &t)
                          { return t == self; });

        if (magic_enum::enum_flags_test_any(GetHeight(), TileHeight::POWER))
            station->RebuildPowerGridsFromInfrastructure();
        if (returnResources)
            station->ReturnResourcesFromTile(self);
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

    for (const auto &[type, component] : components)
        if (auto info = component->GetInfo())
            tileInfo += "\n" + info.value();

    return tileInfo;
}

bool Tile::HasComponent(ComponentType type) const { return components.count(type) > 0; }

bool Tile::CompareByHeight(const std::shared_ptr<Tile> &a, const std::shared_ptr<Tile> &b)
{
    return magic_enum::enum_underlying(a->GetHeight()) < magic_enum::enum_underlying(b->GetHeight());
}
