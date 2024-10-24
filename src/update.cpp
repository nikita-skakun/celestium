#include "update.hpp"

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

        if (camera.GetOverlay() == PlayerCam::Overlay::POWER && station &&
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
                std::vector<Vector2Int> neighbors = {
                    tile->GetPosition() + Vector2Int(1, 0),  // Right
                    tile->GetPosition() + Vector2Int(-1, 0), // Left
                    tile->GetPosition() + Vector2Int(0, 1),  // Down
                    tile->GetPosition() + Vector2Int(0, -1), // Up
                };

                for (int i = 0; i < (int)neighbors.size(); i++)
                {
                    std::shared_ptr<Tile> neighbor = station->GetTileWithComponentAtPosition<OxygenComponent>(neighbors[i]);
                    if (!neighbor)
                        continue;

                    if (station->GetTileWithComponentAtPosition<SolidComponent>(neighbors[i]))
                        continue;

                    auto neighborOxygen = neighbor->GetComponent<OxygenComponent>();
                    double oxygenDiff = oxygen->GetOxygenLevel() - neighborOxygen->GetOxygenLevel();

                    if (oxygenDiff <= 0)
                        continue;

                    float oxygenTransfer = std::min(oxygenDiff * OXYGEN_DIFFUSION_RATE * FIXED_DELTA_TIME, oxygenDiff);
                    oxygen->SetOxygenLevel(oxygen->GetOxygenLevel() - oxygenTransfer);
                    neighborOxygen->SetOxygenLevel(neighborOxygen->GetOxygenLevel() + oxygenTransfer);
                }
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
    if (!station || !IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || camera.GetOverlay() != PlayerCam::Overlay::POWER)
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
