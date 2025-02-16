#include "action.hpp"
#include "crew.hpp"
#include "game_state.hpp"
#include "logging.hpp"
#include "station.hpp"
#include "update.hpp"

// void HandleTileMovement(const std::shared_ptr<Station> &station)
// {
//     auto moveTile = GameManager::GetMoveTile();
//     Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());

//     if (moveTile->GetPosition() != cursorPos)
//     {
//         bool canMove = true;
//         auto overlappingTiles = station->GetTilesWithHeightAtPosition(cursorPos, moveTile->GetHeight());
//         for (auto &tile : overlappingTiles)
//         {
//             if (tile->GetId() == moveTile->GetId())
//             {
//                 canMove = false;
//                 break;
//             }
//             tile->DeleteTile();
//         }

//         if (canMove)
//         {
//             moveTile->MoveTile(cursorPos);
//             LogMessage(LogLevel::DEBUG, std::format("Moved tile {} to {}", moveTile->GetId(), ToString(moveTile->GetPosition())));
//         }
//     }
//     // Clear the move tile to prevent further movement actions until a new tile is selected
//     GameManager::ClearMoveTile();
// }

void HandleSelectTile(const std::shared_ptr<Station> &station)
{
    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());

    if (!IsKeyDown(KEY_LEFT_SHIFT))
        GameManager::ClearSelectedTiles();

    if (auto allTiles = station->GetTilesWithHeightAtPosition(cursorPos, GameManager::GetSelectedHeight()); !allTiles.empty())
    {
        auto selectedTile = GameManager::GetSelectedTiles();

        GameManager::ToggleSelectedTile(allTiles[0]);

        // for (int i = allTiles.size() - 1; i >= 0; --i)
        // {
        //     if (allTiles[i] == selectedTile && i > 0)
        //     {
        //         GameManager::SetSelectedTile(allTiles[i - 1]);
        //         break;
        //     }
        //     else if (i == 0)
        //         GameManager::SetSelectedTile(allTiles[allTiles.size() - 1]);
        // }
    }
}

void HandlePlaceTile(const std::shared_ptr<Station> &station)
{
    const std::string &tileIdToPlace = GameManager::GetBuildTileId();
    const auto &tileDefinition = DefinitionManager::GetTileDefinition(tileIdToPlace);
    if (!tileDefinition)
        return;

    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());

    std::vector<Vector2Int> posListToPlace;
    posListToPlace.push_back(cursorPos);
    if (station->horizontalSymmetry)
        posListToPlace.push_back(Vector2Int(cursorPos.x, -cursorPos.y - 1));

    bool canBuild = true;
    for (const auto &pos : posListToPlace)
    {
        auto overlappingTiles = station->GetTilesWithHeightAtPosition(pos, tileDefinition->GetHeight());
        for (auto &tile : overlappingTiles)
        {
            if (tile->GetId() == tileIdToPlace)
            {
                canBuild = false;
                break;
            }
            tile->DeleteTile();
        }
    }

    if (canBuild)
    {
        for (const auto &pos : posListToPlace)
        {
            auto newTile = Tile::CreateTile(tileIdToPlace, pos, station);
            LogMessage(LogLevel::DEBUG, std::format("Placed tile {} at {}", tileIdToPlace, ToString(pos)));
        }

        station->UpdateSpriteOffsets();
    }
}

void HandleBuildMode()
{
    auto station = GameManager::GetStation();

    if (!station || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return;

    // if (GameManager::GetMoveTile())
    //     HandleTileMovement(station);
    // else 
    if (GameManager::GetBuildTileId().empty())
        HandleSelectTile(station);
    else
        HandlePlaceTile(station);
}

void HandleCrewHover()
{
    const auto &crewList = GameManager::GetCrewList();
    const Vector2 worldMousePos = GameManager::GetWorldMousePos() - Vector2(.5, .5);
    const float crewSize = CREW_RADIUS / TILE_SIZE;
    const float crewSizeSq = crewSize * crewSize;

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
        Vector2Int startPos = ToVector2Int(camera.GetDragStart());
        Vector2Int endPos = ToVector2Int(camera.GetDragEnd());

        if (startPos == endPos)
            return;

        auto startTile = station->GetTileWithComponentAtPosition<PowerConnectorComponent>(startPos);
        auto endTile = station->GetTileWithComponentAtPosition<PowerConnectorComponent>(endPos);

        if (startTile && endTile)
        {
            auto startConnector = startTile->GetComponent<PowerConnectorComponent>();
            auto endConnector = endTile->GetComponent<PowerConnectorComponent>();

            if (PowerConnectorComponent::AddConnection(startConnector, endConnector))
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
            bool isWithinRect = IsVector2WithinRect(camera.GetDragRect(), crew->GetPosition() + Vector2(.5, .5));
            if (!crew || !isWithinRect)
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

void AssignCrewActions()
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
                crew->GetActionQueue().clear();

            crew->GetActionQueue().push_back(std::make_shared<MoveAction>(worldPos));
        }
    }

    const auto &crewList = GameManager::GetCrewList();
    for (const auto &crew : crewList)
    {
        if (!crew->IsAlive() || !crew->GetActionQueue().empty() || !crew->GetCurrentTile())
            continue;

        auto station = crew->GetCurrentTile()->GetStation();
        Vector2Int crewPos = ToVector2Int(crew->GetPosition());

        if (station->GetEffectOfTypeAtPosition<FireEffect>(crewPos))
        {
            crew->GetActionQueue().push_back(std::make_shared<ExtinguishAction>(crewPos));
            continue;
        }

        for (const auto &direction : CARDINAL_DIRECTIONS)
        {
            Vector2Int neighborPos = crewPos + DirectionToVector2Int(direction);
            if (station->GetEffectOfTypeAtPosition<FireEffect>(neighborPos))
            {
                crew->GetActionQueue().push_back(std::make_shared<ExtinguishAction>(neighborPos));
                break;
            }
        }
    }
}

void HandleCrewActions()
{
    const auto &crewList = GameManager::GetCrewList();
    for (const auto &crew : crewList)
    {
        if (!crew->IsAlive() || crew->GetActionQueue().empty())
            continue;

        std::shared_ptr<Action> currentAction = crew->GetActionQueue().front();
        currentAction->Update(crew);
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
        auto tile = crew->GetCurrentTile();
        if (!tile)
            continue;

        auto oxygen = tile->GetComponent<OxygenComponent>();
        if (oxygen)
            crew->RefillOxygen(FIXED_DELTA_TIME, oxygen->GetOxygenLevel());

        auto effects = tile->GetStation()->GetEffectsAtPosition(tile->GetPosition());
        for (const auto &effect : effects)
        {
            effect->EffectCrew(crew, FIXED_DELTA_TIME);
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
