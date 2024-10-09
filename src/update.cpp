#include "update.hpp"

void HandleCrewHover(const std::vector<Crew> &crewList, PlayerCam &camera)
{
    const Vector2 worldMousePos = camera.GetWorldMousePos() - Vector2(.5f, .5f);
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

void HandleMouseDragStart(PlayerCam &camera)
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        camera.SetDragStart(camera.GetWorldMousePos());
}

void HandleMouseDrag(std::shared_ptr<Station> station, PlayerCam &camera)
{
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        return;

    if (!camera.IsDragging() && Vector2DistanceSq(camera.GetDragStart(), camera.GetWorldMousePos()) > DRAG_THRESHOLD * DRAG_THRESHOLD)
    {
        camera.SetDragType(PlayerCam::DragType::SELECT);

        if (camera.GetOverlay() == PlayerCam::Overlay::POWER && station &&
            station->GetTileWithComponentAtPosition<PowerConnectorComponent>(ToVector2Int(camera.GetDragStart())))
        {
            camera.SetDragType(PlayerCam::DragType::POWER_CONNECT);
            camera.SetDragStart(Vector2Floor(camera.GetDragStart()) + Vector2(.5f, .5f));
        }
    }

    if (camera.IsDragging())
        camera.SetDragEnd(camera.GetWorldMousePos());
}

void HandleMouseDragEnd(std::shared_ptr<Station> station, PlayerCam &camera)
{
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

void HandleCrewSelection(const std::vector<Crew> &crewList, PlayerCam &camera)
{
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    {
        if (camera.IsDragging() && camera.GetDragType() != PlayerCam::DragType::SELECT)
            return;

        if (!IsKeyDown(KEY_LEFT_SHIFT))
            camera.ClearSelectedCrew();
        if (camera.GetDragType() == PlayerCam::DragType::SELECT)
        {
            for (std::size_t i = 0; i < crewList.size(); i++)
            {
                Vector2 crewPos = crewList[i].GetPosition() + Vector2(.5f, .5f);
                if (IsVector2WithinBounds(crewPos, camera.GetDragStart(), camera.GetDragEnd()))
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
}

void AssignCrewTasks(std::vector<Crew> &crewList, const PlayerCam &camera)
{
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && camera.GetSelectedCrew().size() > 0)
    {
        Vector2Int worldPos = camera.ScreenToTile(GetMousePosition());
        for (int crewId : camera.GetSelectedCrew())
        {
            Crew &crew = crewList[crewId];
            if (crew.IsAlive() && crew.GetCurrentTile())
            {
                if (!IsKeyDown(KEY_LEFT_SHIFT))
                    crew.GetTaskQueue().clear();

                // Modify this to be adaptive to the selected tile (ie. move, operate, build)
                crew.GetTaskQueue().push_back(std::make_shared<MoveTask>(MoveTask(worldPos)));
            }
        }
    }
}

void HandleCrewTasks(std::vector<Crew> &crewList)
{
    for (Crew &crew : crewList)
    {
        if (crew.GetTaskQueue().empty() || !crew.IsAlive())
            continue;

        std::shared_ptr<Task> task = crew.GetTaskQueue()[0];
        switch (task->GetType())
        {
        case Task::Type::MOVE:
        {
            std::shared_ptr<MoveTask> moveTask = std::dynamic_pointer_cast<MoveTask>(task);
            std::shared_ptr<Station> station = crew.GetCurrentTile()->GetStation();

            Vector2Int floorCrewPos = ToVector2Int(crew.GetPosition());
            if (moveTask->path.empty())
            {
                moveTask->path = AStar(floorCrewPos, moveTask->targetPosition, station);

                if (moveTask->path.size() <= 0)
                {
                    if (ToVector2(moveTask->targetPosition) == crew.GetPosition())
                    {
                        crew.GetTaskQueue().erase(crew.GetTaskQueue().begin());
                        continue;
                    }

                    moveTask->targetPosition = floorCrewPos;
                    moveTask->path.clear();
                    moveTask->path.push_back(floorCrewPos);
                }
            }

            constexpr float moveDelta = CREW_MOVE_SPEED * FIXED_DELTA_TIME;
            const Vector2 stepPos = ToVector2(moveTask->path.front());
            const float distanceLeftSq = Vector2DistanceSq(crew.GetPosition(), stepPos) - moveDelta * moveDelta;
            float distanceToTravel = moveDelta;
            if (distanceLeftSq <= 0)
            {
                if (auto doorTile = station->GetTileWithComponentAtPosition<DoorComponent>(floorCrewPos))
                    doorTile->GetComponent<DoorComponent>()->SetMovingState(DoorComponent::MovingState::IDLE);

                crew.SetPosition(stepPos);
                moveTask->path.pop_front();

                if (moveTask->path.empty())
                {
                    crew.GetTaskQueue().erase(crew.GetTaskQueue().begin());
                    continue;
                }

                // Check if there are any tiles are in the way, if yes, clear path for recalculation
                if (DoesPathHaveObstacles(moveTask->path, station))
                {
                    moveTask->path = {};
                    continue;
                }

                distanceToTravel = sqrtf(-distanceLeftSq);
            }
            else
            {
                if (auto doorTile = station->GetTileWithComponentAtPosition<DoorComponent>(moveTask->path.front()))
                {
                    auto door = doorTile->GetComponent<DoorComponent>();
                    door->SetMovingState(DoorComponent::MovingState::FORCED_OPEN);

                    if (door->GetProgress() > 0.f)
                        continue;
                }
            }

            crew.SetPosition(crew.GetPosition() + Vector2Normalize(stepPos - crew.GetPosition()) * distanceToTravel);
            break;
        }
        default:
            break;
        }
    }
}

void HandleCrewEnvironment(std::vector<Crew> &crewList)
{
    for (Crew &crew : crewList)
    {
        if (crew.IsAlive())
        {
            crew.ConsumeOxygen(FIXED_DELTA_TIME);
            if (crew.GetCurrentTile())
            {
                if (auto oxygen = crew.GetCurrentTile()->GetComponent<OxygenComponent>())
                {
                    crew.RefillOxygen(FIXED_DELTA_TIME, oxygen->GetOxygenLevel());
                }
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
                    float oxygenDiff = oxygen->GetOxygenLevel() - neighborOxygen->GetOxygenLevel();

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

void UpdateEnvironmentalHazards(std::shared_ptr<Station> station)
{
    if (!station)
        return;

    for (int i = station->hazards.size() - 1; i >= 0; --i)
    {
        auto hazard = station->hazards.at(i);
        if (auto fire = std::dynamic_pointer_cast<FireHazard>(hazard))
        {
            auto tile = station->GetTileWithComponentAtPosition<OxygenComponent>(fire->GetPosition());
            if (!tile)
            {
                station->hazards.erase(station->hazards.begin() + i);
                continue;
            }

            auto oxygen = tile->GetComponent<OxygenComponent>();
            if (oxygen->GetOxygenLevel() < fire->GetOxygenConsumption() * FIXED_DELTA_TIME * 2.f)
                fire->SetSize(fire->GetSize() / 3.f * 2.f);

            float oxygenToConsume = fire->GetOxygenConsumption() * FIXED_DELTA_TIME;
            if (oxygen->GetOxygenLevel() < oxygenToConsume)
            {
                oxygen->SetOxygenLevel(0.f);
                station->hazards.erase(station->hazards.begin() + i);
                continue;
            }

            oxygen->SetOxygenLevel(oxygen->GetOxygenLevel() - oxygenToConsume);
            fire->SetSize(fire->GetSize() + FireHazard::GROWTH_IF_FED * FIXED_DELTA_TIME);

            if (CheckIfEventHappens(fire->SPREAD_CHANCE_PER_SECOND, FIXED_DELTA_TIME))
            {
                std::vector<Direction> neighborDirections = {Direction::N, Direction::E, Direction::S, Direction::W};
                for (int i = (int)neighborDirections.size() - 1; i >= 0; --i)
                {
                    Vector2Int neighborPos = fire->GetPosition() + DirectionToVector2Int(neighborDirections[i]);
                    if (!station->GetTileWithComponentAtPosition<OxygenComponent>(neighborPos))
                        neighborDirections.erase(neighborDirections.begin() + i);
                }

                if (neighborDirections.size() <= 0)
                    continue;

                int directionSelected = RandomIntWithRange(0, (int)neighborDirections.size() - 1);
                Vector2Int newFirePos = fire->GetPosition() + DirectionToVector2Int(neighborDirections[directionSelected]);
                if (station->GetTypeHazardsAtPosition<FireHazard>(newFirePos).empty())
                    station->hazards.push_back(std::make_shared<FireHazard>(newFirePos, FireHazard::SIZE_INCREMENT));
            }
        }
    }
}

void MouseDeleteExistingConnection(std::shared_ptr<Station> station, const PlayerCam &camera)
{
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

                Vector2 start = camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(.5f, .5f));
                Vector2 end = camera.WorldToScreen(ToVector2(otherConnectorTile->GetPosition()) + Vector2(.5f, .5f));

                if (DistanceSqFromPointToLine(start, end, mousePos) > threshold * threshold)
                    continue;

                LogMessage(LogLevel::DEBUG, std::format("Deleting connection between {} and {}!", tile->GetName(), otherConnectorTile->GetName()));
                PowerConnectorComponent::DeleteConnection(powerConnector, otherConnector);
                return;
            }
        }
    }
}
