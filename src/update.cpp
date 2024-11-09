#include "crew.hpp"
#include "game_state.hpp"
#include "logging.hpp"
#include "station.hpp"
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

void HandleBuildMode()
{
    auto station = GameManager::GetStation();

    if (!station || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return;

    if (GameManager::GetMoveTile())
        HandleMoveTile(station);
    else if (GameManager::GetBuildTileId().empty())
        HandleSelectTile(station);
    else
        HandlePlaceTile(station);
}

void HandleCrewHover()
{
    const auto &crewList = GameManager::GetCrewList();
    const Vector2 worldMousePos = GameManager::GetWorldMousePos() - Vector2(.5, .5);
    const float crewSizeSq = CREW_RADIUS / TILE_SIZE * CREW_RADIUS / TILE_SIZE;

    GameManager::ClearHoveredCrew();

    for (const auto &crew : crewList)
    {
        if (!crew || Vector2DistanceSq(worldMousePos, crew->GetPosition()) > crewSizeSq)
            continue;

        GameManager::AddHoveredCrew(crew);
    }
}

void HandleMouseDragStart()
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        GameManager::GetCamera().SetDragStart(GameManager::GetWorldMousePos());
}

void HandleMouseDragDuring()
{
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        return;

    auto &camera = GameManager::GetCamera();

    if (!camera.IsDragging() && Vector2DistanceSq(camera.GetDragStart(), GameManager::GetWorldMousePos()) > DRAG_THRESHOLD * DRAG_THRESHOLD)
    {
        camera.SetDragType(PlayerCam::DragType::SELECT);

        auto station = GameManager::GetStation();
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

void HandleMouseDragEnd()
{
    auto &camera = GameManager::GetCamera();
    if (!IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || !camera.IsDragging())
        return;

    auto station = GameManager::GetStation();
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

void HandleMouseDrag()
{
    HandleMouseDragStart();
    HandleMouseDragDuring();
    HandleMouseDragEnd();
}

void HandleCrewSelection()
{
    if (!IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        return;

    auto &camera = GameManager::GetCamera();
    if (camera.IsDragging() && camera.GetDragType() != PlayerCam::DragType::SELECT)
        return;

    if (!IsKeyDown(KEY_LEFT_SHIFT))
        GameManager::ClearSelectedCrew();
    if (camera.GetDragType() == PlayerCam::DragType::SELECT)
    {
        auto &crewList = GameManager::GetCrewList();
        for (const auto &crew : crewList)
        {
            if (!crew || !IsVector2WithinRect(camera.GetDragRect(), crew->GetPosition() + Vector2(.5, .5)))
                continue;

            GameManager::AddSelectedCrew(crew);
        }

        return;
    }

    const auto &hoveredCrew = GameManager::GetHoveredCrew();
    if (hoveredCrew.empty())
        return;

    if (auto nextHoveredCrew = hoveredCrew.at(0).lock())
        GameManager::ToggleSelectedCrew(nextHoveredCrew);
}

void AssignCrewTasks()
{
    const auto &selectedCrew = GameManager::GetSelectedCrew();
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !selectedCrew.empty())
    {
        Vector2Int worldPos = ToVector2Int(GameManager::GetWorldMousePos());
        for (const auto &_crew : selectedCrew)
        {
            auto crew = _crew.lock();
            if (!crew || !crew->IsAlive() || !crew->GetCurrentTile())
                continue;

            if (!IsKeyDown(KEY_LEFT_SHIFT))
                crew->GetTaskQueue().clear();

            crew->GetTaskQueue().push_back(std::make_shared<MoveTask>(worldPos));
        }
    }

    const auto &crewList = GameManager::GetCrewList();
    for (const auto &crew : crewList)
    {
        if (!crew->IsAlive() || !crew->GetTaskQueue().empty() || !crew->GetCurrentTile())
            continue;

        auto station = crew->GetCurrentTile()->GetStation();
        Vector2Int crewPos = ToVector2Int(crew->GetPosition());

        if (station->GetEffectOfTypeAtPosition<FireEffect>(crewPos))
        {
            crew->GetTaskQueue().push_back(std::make_shared<ExtinguishTask>(crewPos));
            continue;
        }

        for (const auto &direction : CARDINAL_DIRECTIONS)
        {
            Vector2Int neighborPos = crewPos + DirectionToVector2Int(direction);
            if (station->GetEffectOfTypeAtPosition<FireEffect>(neighborPos))
            {
                crew->GetTaskQueue().push_back(std::make_shared<ExtinguishTask>(neighborPos));
                break;
            }
        }
    }
}

void HandleCrewTasks()
{
    const auto &crewList = GameManager::GetCrewList();
    for (const auto &crew : crewList)
    {
        if (!crew->IsAlive() || crew->GetTaskQueue().empty())
            continue;

        std::shared_ptr<Task> task = crew->GetTaskQueue().at(0);
        task->Update(crew);
    }
}

void HandleCrewEnvironment()
{
    const auto &crewList = GameManager::GetCrewList();
    for (const auto &crew : crewList)
    {
        if (!crew->IsAlive())
            continue;

        crew->ConsumeOxygen(FIXED_DELTA_TIME);
        if (auto tile = crew->GetCurrentTile())
        {
            if (auto oxygen = tile->GetComponent<OxygenComponent>())
                crew->RefillOxygen(FIXED_DELTA_TIME, oxygen->GetOxygenLevel());

            auto effects = tile->GetStation()->GetEffectsAtPosition(tile->GetPosition());
            for (const auto &effect : effects)
            {
                effect->EffectCrew(crew, FIXED_DELTA_TIME);
            }
        }
    }
}

void UpdateCrewCurrentTile()
{
    auto station = GameManager::GetStation();
    if (!station || station->tileMap.empty())
        return;

    const auto &crewList = GameManager::GetCrewList();
    for (const auto &crew : crewList)
    {
        if (!crew->IsAlive())
            continue;

        Vector2Int floorCrewPos = ToVector2Int(crew->GetPosition());

        if (crew->GetCurrentTile() && crew->GetCurrentTile()->GetPosition() == floorCrewPos)
            continue;

        crew->SetCurrentTile(station->GetTileAtPosition(floorCrewPos, TileDef::Height::FLOOR));
    }
}

void UpdateTiles()
{
    auto station = GameManager::GetStation();
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

void UpdateEnvironmentalEffects()
{
    auto station = GameManager::GetStation();
    if (!station)
        return;

    for (int i = station->effects.size() - 1; i >= 0; --i)
    {
        auto &effect = station->effects.at(i);
        effect->Update(station, i);
    }
}

void MouseDeleteExistingConnection()
{
    auto &camera = GameManager::GetCamera();
    auto station = GameManager::GetStation();
    if (!station || !IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || !camera.IsOverlay(PlayerCam::Overlay::POWER) || !GameManager::IsInBuildMode())
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

                Vector2 start = GameManager::WorldToScreen(tile->GetPosition());
                Vector2 end = GameManager::WorldToScreen(otherConnectorTile->GetPosition());

                if (DistanceSqFromPointToLine(start, end, mousePos) > threshold * threshold)
                    continue;

                LogMessage(LogLevel::DEBUG, std::format("Deleting connection between {} and {}!", tile->GetName(), otherConnectorTile->GetName()));
                PowerConnectorComponent::DeleteConnection(powerConnector, otherConnector);
                return;
            }
        }
    }
}
