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
        Vector2 worldPos = ScreenToTile(GetMousePosition(), camera);
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

void HandleCrewTasks(float deltaTime, std::vector<Crew> &crewList)
{
    for (Crew &crew : crewList)
    {
        if (!crew.taskQueue.empty() && crew.isAlive)
        {
            Task *task = crew.taskQueue[0].get();
            switch (task->GetType())
            {
            case Task::Type::MOVE:
            {
                auto moveTask = dynamic_cast<MoveTask *>(task);

                if (moveTask->path.empty())
                {
                    moveTask->path = AStar(crew.position, moveTask->targetPosition, crew.currentTile->station);

                    if (moveTask->path.size() <= 0)
                    {
                        if (moveTask->targetPosition != Vector2Floor(crew.position))
                        {
                            moveTask->targetPosition = Vector2Floor(crew.position);
                            moveTask->path = std::queue<Vector2>();
                            moveTask->path.push(Vector2Floor(crew.position));
                        }
                        else
                        {
                            crew.taskQueue.erase(crew.taskQueue.begin());
                            continue;
                        }
                    }
                }

                const float moveDelta = CREW_MOVE_SPEED * deltaTime;
                if (Vector2DistanceSq(crew.position, moveTask->path.front()) <= moveDelta * moveDelta)
                {
                    crew.position = moveTask->path.front();
                    moveTask->path.pop();

                    if (moveTask->path.empty())
                    {
                        crew.taskQueue.erase(crew.taskQueue.begin());
                        continue;
                    }

                    // Check if there are any tiles are in the way, if yes, clear path for recalculation
                    if (DoesPathHaveObstacles(moveTask->path, crew.currentTile->station, crew.CanPathInSpace()))
                    {
                        moveTask->path = std::queue<Vector2>();
                        continue;
                    }
                }
                else
                {
                    crew.position += Vector2Normalize(moveTask->path.front() - crew.position) * moveDelta;
                }
                break;
            }
            default:
                break;
            }
        }
    }
}

void HandleCrewEnvironment(float deltaTime, std::vector<Crew> &crewList)
{
    for (Crew &crew : crewList)
    {
        if (crew.isAlive)
        {
            crew.ConsumeOxygen(deltaTime);
            if (crew.currentTile && crew.currentTile->GetType() == Tile::Type::FLOOR)
            {
                FloorTile *floorTile = dynamic_cast<FloorTile *>(crew.currentTile.get());
                crew.RefillOxygen(deltaTime, floorTile->oxygen);
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

        Vector2 roundCrewPos = Vector2Round(crew.position);

        if (crew.currentTile && crew.currentTile->position == roundCrewPos)
            continue;

        auto it = station->tileMap.find(roundCrewPos);
        if (it != station->tileMap.end())
        {
            crew.currentTile = it->second;
        }
        else
        {
            crew.currentTile = nullptr;
        }
    }
}

void UpdateTiles(float deltaTime, std::shared_ptr<Station> station)
{
    for (auto &tile : station->tiles)
    {
        if (tile->GetType() != Tile::Type::FLOOR)
            continue;

        FloorTile *floorTile = dynamic_cast<FloorTile *>(tile.get());

        std::vector<Vector2> neighbors = {
            tile->position + Vector2(1.f, 0),  // Right
            tile->position + Vector2(-1.f, 0), // Left
            tile->position + Vector2(0, 1.f),  // Down
            tile->position + Vector2(0, -1.f), // Up
        };

        for (int i = 0; i < 4; i++)
        {
            std::shared_ptr<Tile> neighbor = station->GetTileAtPosition(neighbors[i]);
            if (!neighbor || neighbor->GetType() != Tile::Type::FLOOR)
                continue;

            FloorTile *neighbourFloorTile = dynamic_cast<FloorTile *>(neighbor.get());
            float oxygenDiff = floorTile->oxygen - neighbourFloorTile->oxygen;

            if (oxygenDiff > 0)
            {
                float oxygenTransfer = oxygenDiff * OXYGEN_DIFFUSION_RATE * deltaTime;

                floorTile->oxygen -= oxygenTransfer;
                neighbourFloorTile->oxygen += oxygenTransfer;
            }
        }
    }
}
