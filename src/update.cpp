#include "update.hpp"

void HandleMoveTile(std::shared_ptr<Station> station)
{
    auto moveTile = GameManager::GetMoveTile();
    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());

    if (moveTile->GetPosition() != cursorPos)
    {
        bool canMove = true;
        auto overlappingTiles = station->GetTilesWithHeightAtPosition(cursorPos, moveTile->GetHeight());
        for (auto &tile : overlappingTiles)
        {
            if (tile->GetId() == moveTile->GetId())
            {
                canMove = false;
                break;
            }
            tile->DeleteTile();
        }

        if (canMove)
        {
            moveTile->MoveTile(cursorPos);
            LogMessage(LogLevel::DEBUG, std::format("Moved tile {} to {}", moveTile->GetId(), ToString(moveTile->GetPosition())));
        }
    }
    GameManager::ClearMoveTile();
}

void HandleSelectTile(std::shared_ptr<Station> station)
{
    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());

    if (auto allTiles = station->GetAllTilesAtPosition(cursorPos); !allTiles.empty())
    {
        auto selectedTile = GameManager::GetSelectedTile();

        for (int i = allTiles.size() - 1; i >= 0; --i)
        {
            if (allTiles[i] == selectedTile && i > 0)
            {
                GameManager::SetSelectedTile(allTiles[i - 1]);
                break;
            }
            if (i == 0)
                GameManager::SetSelectedTile(allTiles[allTiles.size() - 1]);
        }
    }
    else
        GameManager::SetSelectedTile(nullptr);
}

void HandlePlaceTile(std::shared_ptr<Station> station)
{
    const std::string &buildTileId = GameManager::GetBuildTileId();
    const auto &tileDef = DefinitionManager::GetTileDefinition(buildTileId);
    if (!tileDef)
        return;

    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());
    bool canBuild = true;
    auto overlappingTiles = station->GetTilesWithHeightAtPosition(cursorPos, tileDef->GetHeight());
    for (auto &tile : overlappingTiles)
    {
        if (tile->GetId() == buildTileId)
        {
            canBuild = false;
            break;
        }
        tile->DeleteTile();
    }

    if (canBuild && Tile::CreateTile(buildTileId, cursorPos, station))
    {
        station->UpdateSpriteOffsets();
        LogMessage(LogLevel::DEBUG, std::format("Placed tile {} at {}", buildTileId, ToString(cursorPos)));
    }
}

void HandleBuildMode(std::shared_ptr<Station> station)
{
    if (!station || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return;

    if (GameManager::GetMoveTile())
        HandleMoveTile(station);
    else if (GameManager::GetBuildTileId().empty())
        HandleSelectTile(station);
    else
        HandlePlaceTile(station);
}

void HandleCrewHover(const std::vector<Crew> &crewList)
{
    auto &camera = GameManager::GetCamera();
    const Vector2 worldMousePos = GameManager::GetWorldMousePos() - Vector2(.5, .5);
    const float crewSizeSq = CREW_RADIUS / TILE_SIZE * CREW_RADIUS / TILE_SIZE;

    camera.SetCrewHoverIndex(-1);
    for (int i = crewList.size() - 1; i >= 0; --i)
    {
        if (Vector2DistanceSq(worldMousePos, crewList[i].GetPosition()) <= crewSizeSq)
        {
            camera.SetCrewHoverIndex(i);
            break;
        }
    }
}

void HandleMouseDragStart()
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        GameManager::GetCamera().SetDragStart(GameManager::GetWorldMousePos());
}

void HandleMouseDragDuring(std::shared_ptr<Station> station)
{
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        return;

    auto &camera = GameManager::GetCamera();

    if (!camera.IsDragging() && Vector2DistanceSq(camera.GetDragStart(), GameManager::GetWorldMousePos()) > DRAG_THRESHOLD * DRAG_THRESHOLD)
    {
        camera.SetDragType(PlayerCam::DragType::SELECT);

        if (camera.IsOverlay(PlayerCam::Overlay::POWER) && station &&
            station->GetTileWithComponentAtPosition<PowerConnectorComponent>(ToVector2Int(camera.GetDragStart())))
        {
            camera.SetDragType(PlayerCam::DragType::POWER_CONNECT);
            camera.SetDragStart(Vector2Floor(camera.GetDragStart()) + Vector2(.5, .5));
        }
    }

    if (camera.IsDragging())
        camera.SetDragEnd(GameManager::GetWorldMousePos());
}

void HandleMouseDragEnd(std::shared_ptr<Station> station)
{
    auto &camera = GameManager::GetCamera();
    if (!IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || !camera.IsDragging())
        return;

    if (camera.GetDragType() == PlayerCam::DragType::POWER_CONNECT && station)
    {
        Vector2Int dragStart = ToVector2Int(camera.GetDragStart());
        Vector2Int dragEnd = ToVector2Int(camera.GetDragEnd());

        if (dragStart == dragEnd)
            return;

        auto startTile = station->GetTileWithComponentAtPosition<PowerConnectorComponent>(dragStart);
        auto endTile = station->GetTileWithComponentAtPosition<PowerConnectorComponent>(dragEnd);

        if (startTile && endTile)
        {
            auto start = startTile->GetComponent<PowerConnectorComponent>();
            auto end = endTile->GetComponent<PowerConnectorComponent>();

            if (PowerConnectorComponent::AddConnection(start, end))
                LogMessage(LogLevel::DEBUG, std::format("{} connected to {}!", startTile->GetName(), endTile->GetName()));
        }
    }

    camera.SetDragType(PlayerCam::DragType::NONE);
}

void HandleMouseDrag(std::shared_ptr<Station> station)
{
    HandleMouseDragStart();
    HandleMouseDragDuring(station);
    HandleMouseDragEnd(station);
}

void HandleCrewSelection(const std::vector<Crew> &crewList)
{
    if (!IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        return;

    auto &camera = GameManager::GetCamera();
    if (camera.IsDragging() && camera.GetDragType() != PlayerCam::DragType::SELECT)
        return;

    if (!IsKeyDown(KEY_LEFT_SHIFT))
        camera.ClearSelectedCrew();
    if (camera.GetDragType() == PlayerCam::DragType::SELECT)
    {
        for (std::size_t i = 0; i < crewList.size(); i++)
        {
            Vector2 crewPos = crewList[i].GetPosition() + Vector2(.5, .5);
            if (IsVector2WithinRect(Vector2ToBoundingBox(camera.GetDragStart(), camera.GetDragEnd()), crewPos))
            {
                camera.AddSelectedCrew(i);
            }
        }
    }
    else
    {
        if (camera.GetCrewHoverIndex() >= 0)
            camera.ToggleSelectedCrew(camera.GetCrewHoverIndex());
    }
}

void AssignCrewTasks(std::vector<Crew> &crewList)
{
    auto &camera = GameManager::GetCamera();
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && camera.GetSelectedCrew().size() > 0)
    {
        Vector2Int worldPos = ToVector2Int(GameManager::GetWorldMousePos());
        for (int crewId : camera.GetSelectedCrew())
        {
            Crew &crew = crewList[crewId];
            if (!crew.IsAlive() || !crew.GetCurrentTile())
                continue;

            if (!IsKeyDown(KEY_LEFT_SHIFT))
                crew.GetTaskQueue().clear();

            // Modify this to be adaptive to the selected tile (ie. move, operate, build)
            crew.GetTaskQueue().push_back(std::make_shared<MoveTask>(worldPos));
        }
    }

    for (Crew &crew : crewList)
    {
        if (!crew.IsAlive() || !crew.GetTaskQueue().empty() || !crew.GetCurrentTile())
            continue;

        auto station = crew.GetCurrentTile()->GetStation();
        Vector2Int crewPos = ToVector2Int(crew.GetPosition());

        if (station->GetEffectOfTypeAtPosition<FireEffect>(crewPos))
        {
            crew.GetTaskQueue().push_back(std::make_shared<ExtinguishTask>(crewPos));
            continue;
        }

        for (const auto &direction : CARDINAL_DIRECTIONS)
        {
            Vector2Int neighborPos = crewPos + DirectionToVector2Int(direction);
            if (station->GetEffectOfTypeAtPosition<FireEffect>(neighborPos))
            {
                crew.GetTaskQueue().push_back(std::make_shared<ExtinguishTask>(neighborPos));
                break;
            }
        }
    }
}

void HandleCrewTasks(std::vector<Crew> &crewList)
{
    for (Crew &crew : crewList)
    {
        if (!crew.IsAlive() || crew.GetTaskQueue().empty())
            continue;

        std::shared_ptr<Task> task = crew.GetTaskQueue().at(0);
        task->Update(crew);
    }
}

void HandleCrewEnvironment(std::vector<Crew> &crewList)
{
    for (Crew &crew : crewList)
    {
        if (!crew.IsAlive())
            continue;

        crew.ConsumeOxygen(FIXED_DELTA_TIME);
        if (auto tile = crew.GetCurrentTile())
        {
            if (auto oxygen = tile->GetComponent<OxygenComponent>())
                crew.RefillOxygen(FIXED_DELTA_TIME, oxygen->GetOxygenLevel());

            auto effects = tile->GetStation()->GetEffectsAtPosition(tile->GetPosition());
            for (const auto &effect : effects)
            {
                effect->EffectCrew(crew, FIXED_DELTA_TIME);
            }
        }
    }
}

void UpdateCrewCurrentTile(std::vector<Crew> &crewList, std::shared_ptr<Station> station)
{
    if (!station || station->tileMap.empty())
        return;

    for (Crew &crew : crewList)
    {
        if (!crew.IsAlive())
            continue;

        Vector2Int floorCrewPos = ToVector2Int(crew.GetPosition());

        if (crew.GetCurrentTile() && crew.GetCurrentTile()->GetPosition() == floorCrewPos)
            continue;

        crew.SetCurrentTile(station->GetTileAtPosition(floorCrewPos, TileDef::Height::FLOOR));
    }
}

void UpdateTiles(std::shared_ptr<Station> station)
{
    if (!station)
        return;

    for (const auto &tilesAtPos : station->tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            if (auto door = tile->GetComponent<DoorComponent>())
            {
                door->KeepClosed();
                door->Animate(FIXED_DELTA_TIME);
            }

            if (auto powerProducer = tile->GetComponent<PowerProducerComponent>())
            {
                powerProducer->ProducePower(FIXED_DELTA_TIME);
            }

            if (auto powerConsumer = tile->GetComponent<PowerConsumerComponent>())
            {
                powerConsumer->ConsumePower(FIXED_DELTA_TIME);
            }

            if (auto battery = tile->GetComponent<BatteryComponent>())
            {
                battery->Charge();
            }

            if (auto oxygenProducer = tile->GetComponent<OxygenProducerComponent>())
            {
                oxygenProducer->ProduceOxygen(FIXED_DELTA_TIME);
            }

            if (auto oxygen = tile->GetComponent<OxygenComponent>())
            {
                oxygen->Diffuse(FIXED_DELTA_TIME);
            }
        }
    }
}

void UpdateEnvironmentalEffects(std::shared_ptr<Station> station)
{
    if (!station)
        return;

    for (int i = station->effects.size() - 1; i >= 0; --i)
    {
        auto &effect = station->effects.at(i);
        effect->Update(station, i);
    }
}

void MouseDeleteExistingConnection(std::shared_ptr<Station> station)
{
    auto &camera = GameManager::GetCamera();
    if (!station || !IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || !camera.IsOverlay(PlayerCam::Overlay::POWER))
        return;

    Vector2 mousePos = GetMousePosition();

    const float threshold = std::max(POWER_CONNECTION_WIDTH * camera.GetZoom(), 2.f);

    for (const auto &tilesAtPos : station->tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            auto powerConnector = tile->GetComponent<PowerConnectorComponent>();
            if (!powerConnector)
                continue;

            for (const auto &connection : powerConnector->_connections)
            {
                auto otherConnector = connection.lock();
                if (!otherConnector)
                    continue;

                auto otherConnectorTile = otherConnector->_parent.lock();
                if (!otherConnectorTile)
                    continue;

                Vector2 start = GameManager::WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(.5, .5));
                Vector2 end = GameManager::WorldToScreen(ToVector2(otherConnectorTile->GetPosition()) + Vector2(.5, .5));

                if (DistanceSqFromPointToLine(start, end, mousePos) > threshold * threshold)
                    continue;

                LogMessage(LogLevel::DEBUG, std::format("Deleting connection between {} and {}!", tile->GetName(), otherConnectorTile->GetName()));
                PowerConnectorComponent::DeleteConnection(powerConnector, otherConnector);
                return;
            }
        }
    }
}
