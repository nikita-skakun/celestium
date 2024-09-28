#include "update.h"

void HandleCrewHover(const std::vector<Crew> &crewList, PlayerCam &camera)
{
    const Vector2 worldMousePos = ScreenToWorld(GetMousePosition(), camera) - Vector2(.5f, .5f);
    const float crewSizeSq = CREW_RADIUS / TILE_SIZE * CREW_RADIUS / TILE_SIZE;

    camera.crewHoverIndex = -1;
    for (int i = crewList.size() - 1; i >= 0; --i)
    {
        if (Vector2DistanceSq(worldMousePos, crewList[i].position) <= crewSizeSq)
        {
            camera.crewHoverIndex = i;
            break;
        }
    }
}

void HandleCrewSelection(const std::vector<Crew> &crewList, PlayerCam &camera)
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        camera.dragStartPos = camera.GetWorldMousePos();
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        if (!camera.isDragging && Vector2DistanceSq(camera.dragStartPos, camera.GetWorldMousePos()) > DRAG_THRESHOLD * DRAG_THRESHOLD)
        {
            camera.isDragging = true;
        }
        if (camera.isDragging)
        {
            camera.dragEndPos = camera.GetWorldMousePos();
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    {
        if (!IsKeyDown(KEY_LEFT_SHIFT))
            camera.selectedCrewList.clear();
        if (camera.isDragging)
        {
            for (std::size_t i = 0; i < crewList.size(); i++)
            {
                Vector2 crewPos = crewList[i].position + Vector2(.5f, .5f);
                if (IsVector2WithinBounds(crewPos, camera.dragStartPos, camera.dragEndPos))
                {
                    camera.selectedCrewList.insert(i);
                }
            }
            camera.isDragging = false;
        }
        else
        {
            if (camera.crewHoverIndex >= 0)
            {
                const auto selectedCrewListIter = camera.selectedCrewList.find(camera.crewHoverIndex);
                if (selectedCrewListIter != camera.selectedCrewList.end())
                    camera.selectedCrewList.erase(selectedCrewListIter);
                else
                    camera.selectedCrewList.insert(camera.crewHoverIndex);
            }
        }
    }
}

void AssignCrewTasks(std::vector<Crew> &crewList, const PlayerCam &camera)
{

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && camera.selectedCrewList.size() > 0)
    {
        Vector2Int worldPos = ScreenToTile(GetMousePosition(), camera);
        for (int crewId : camera.selectedCrewList)
        {
            Crew &crew = crewList[crewId];
            if (crew.isAlive)
            {
                if (!IsKeyDown(KEY_LEFT_SHIFT))
                    crew.taskQueue.clear();

                // Modify this to be adaptive to the selected tile (ie. move, operate, build)
                crew.taskQueue.push_back(std::make_shared<MoveTask>(MoveTask(worldPos)));
            }
        }
    }
}

void HandleCrewTasks(std::vector<Crew> &crewList)
{
    for (Crew &crew : crewList)
    {
        if (!crew.taskQueue.empty() && crew.isAlive)
        {
            std::shared_ptr<Task> task = crew.taskQueue[0];
            switch (task->GetType())
            {
            case Task::Type::MOVE:
            {
                std::shared_ptr<MoveTask> moveTask = std::dynamic_pointer_cast<MoveTask>(task);

                Vector2Int floorCrewPos = ToVector2Int(crew.position);
                if (moveTask->path.empty())
                {
                    moveTask->path = AStar(floorCrewPos, moveTask->targetPosition, crew.currentTile->station);

                    if (moveTask->path.size() <= 0)
                    {
                        if (moveTask->targetPosition != floorCrewPos)
                        {
                            moveTask->targetPosition = floorCrewPos;
                            moveTask->path = {};
                            moveTask->path.push_back(floorCrewPos);
                        }
                        else
                        {
                            crew.taskQueue.erase(crew.taskQueue.begin());
                            continue;
                        }
                    }
                }

                constexpr float moveDelta = CREW_MOVE_SPEED * FIXED_DELTA_TIME;
                const float distanceLeftSq = Vector2DistanceSq(crew.position, ToVector2(moveTask->path.front())) - moveDelta * moveDelta;
                if (distanceLeftSq <= 0)
                {
                    crew.position = ToVector2(moveTask->path.front());
                    moveTask->path.pop_front();

                    if (moveTask->path.empty())
                    {
                        crew.taskQueue.erase(crew.taskQueue.begin());
                        continue;
                    }

                    // Check if there are any tiles are in the way, if yes, clear path for recalculation
                    if (DoesPathHaveObstacles(moveTask->path, crew.currentTile->station, crew.CanPathInSpace()))
                    {
                        moveTask->path = {};
                        continue;
                    }

                    crew.position += Vector2Normalize(ToVector2(moveTask->path.front()) - crew.position) * sqrtf(-distanceLeftSq);
                }
                else
                {
                    crew.position += Vector2Normalize(ToVector2(moveTask->path.front()) - crew.position) * moveDelta;
                }
                break;
            }
            default:
                break;
            }
        }
    }
}

void HandleCrewEnvironment(std::vector<Crew> &crewList)
{
    for (Crew &crew : crewList)
    {
        if (crew.isAlive)
        {
            crew.ConsumeOxygen(FIXED_DELTA_TIME);
            if (crew.currentTile)
            {
                if (auto oxygenComp = crew.currentTile->GetComponent<OxygenComponent>())
                {
                    crew.RefillOxygen(FIXED_DELTA_TIME, oxygenComp->GetOxygenLevel());
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
        if (!crew.isAlive)
            continue;

        Vector2Int floorCrewPos = ToVector2Int(crew.position);

        if (crew.currentTile && crew.currentTile->position == floorCrewPos)
            continue;

        crew.currentTile = station->GetTileAtPosition(floorCrewPos, Tile::Height::FLOOR);
    }
}

void UpdateTiles(std::shared_ptr<Station> station)
{
    for (auto &tile : station->tiles)
    {
        if (auto oxProdComp = tile->GetComponent<OxygenProducerComponent>())
        {
            if (!oxProdComp->output)
                continue;

            oxProdComp->output->SetOxygenLevel(std::min(oxProdComp->output->GetOxygenLevel() + OXYGEN_PRODUCTION_RATE * FIXED_DELTA_TIME, TILE_OXYGEN_MAX));
        }

        if (auto oxygenComp = tile->GetComponent<OxygenComponent>())
        {
            std::vector<Vector2Int> neighbors = {
                tile->position + Vector2Int(1, 0),  // Right
                tile->position + Vector2Int(-1, 0), // Left
                tile->position + Vector2Int(0, 1),  // Down
                tile->position + Vector2Int(0, -1), // Up
            };

            for (int i = 0; i < 4; i++)
            {
                std::shared_ptr<Tile> neighbor = station->GetTileAtPosition(neighbors[i], Tile::Height::FLOOR);

                if (!neighbor)
                    continue;

                if (auto neighborOxygenComp = neighbor->GetComponent<OxygenComponent>())
                {
                    float oxygenDiff = oxygenComp->GetOxygenLevel() - neighborOxygenComp->GetOxygenLevel();

                    if (oxygenDiff > 0)
                    {
                        float oxygenTransfer = std::min(oxygenDiff * OXYGEN_DIFFUSION_RATE * FIXED_DELTA_TIME, oxygenDiff);
                        oxygenComp->SetOxygenLevel(oxygenComp->GetOxygenLevel() - oxygenTransfer);
                        neighborOxygenComp->SetOxygenLevel(neighborOxygenComp->GetOxygenLevel() + oxygenTransfer);
                    }
                }
            }
        }
    }
}
