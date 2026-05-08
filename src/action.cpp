#include "action.hpp"
#include "astar.hpp"
#include "component.hpp"
#include "crew.hpp"
#include "planned_task.hpp"
#include "station.hpp"
#include "tile.hpp"

void MoveAction::Update(const std::shared_ptr<Crew> &crew)
{
    if (!crew)
        return;

    auto station = crew->GetCurrentTile()->GetStation();
    if (!station)
    {
        crew->RemoveFirstAction();
        return;
    }

    if (path.empty())
    {
        path = FindPath(crew->GetPosition(), targetPosition, station->navPolygons, 
            [](const ConvexPolygon::Link& link) {
                if (auto doorTile = link.door.lock()) {
                    return doorTile->IsActive(); // Only path through powered doors
                }
                return true;
            });

        if (path.empty())
        {
            TraceLog(LOG_INFO, "MoveAction: FindPath returned empty. Target: (%f, %f), Pos: (%f, %f)", targetPosition.x, targetPosition.y, crew->GetPosition().x, crew->GetPosition().y);
            if (targetPosition == crew->GetPosition())
            {
                crew->RemoveFirstAction();
                return;
            }

            targetPosition = crew->GetPosition();
            crew->RemoveFirstAction();
            return;
        }
    }

    const float moveDelta = CREW_MOVE_SPEED * FIXED_DELTA_TIME;
    Vector2 nextWaypoint = path.front();
    Vector2 dir = Vector2Normalize(nextWaypoint - crew->GetPosition());
    
    // Check for door at current position or slightly ahead
    std::shared_ptr<Tile> doorTile = nullptr;
    Vector2Int currentTilePos = {(int)std::floor(crew->GetPosition().x + 0.5f), (int)std::floor(crew->GetPosition().y + 0.5f)};
    doorTile = station->GetTileWithComponentAtPosition(currentTilePos, ComponentType::DOOR);
    
    if (!doorTile) {
        Vector2 checkPos = crew->GetPosition() + dir * 0.6f;
        Vector2Int checkTilePos = {(int)std::floor(checkPos.x + 0.5f), (int)std::floor(checkPos.y + 0.5f)};
        doorTile = station->GetTileWithComponentAtPosition(checkTilePos, ComponentType::DOOR);
    }

    if (doorTile)
    {
        if (auto door = doorTile->GetComponent<DoorComponent>())
        {
            door->Open(1.0f / CREW_MOVE_SPEED);

            if (!door->IsOpen())
                return; // Wait for the door to be fully open
        }
    }

    float distToWaypoint = Vector2Distance(crew->GetPosition(), nextWaypoint);
    
    if (distToWaypoint <= moveDelta)
    {
        // Reach waypoint
        crew->SetPosition(nextWaypoint);
        
        // Reset door state if we just passed through one
        Vector2Int currentTilePos = ToVector2Int(crew->GetPosition() / TILE_SIZE);
        if (auto doorTile = station->GetTileWithComponentAtPosition(currentTilePos, ComponentType::DOOR))
        {
            if (auto door = doorTile->GetComponent<DoorComponent>())
                door->Close();
        }

        path.pop_front();

        if (path.empty())
        {
            crew->RemoveFirstAction();
            return;
        }
    }
    else
    {
        // Move towards waypoint
        crew->SetPosition(crew->GetPosition() + Vector2Normalize(nextWaypoint - crew->GetPosition()) * moveDelta);
    }
}

void ExtinguishAction::Update(const std::shared_ptr<Crew> &crew)
{
    if (!crew)
        return;

    auto station = crew->GetCurrentTile()->GetStation();
    if (!station)
    {
        crew->RemoveFirstAction();
        return;
    }

    if (auto fire = station->GetEffectOfTypeAtPosition(targetPosition, "FIRE"))
    {
        // Fire effect exists at the target position
        if (progress > 1.f)
        {
            station->RemoveEffect(fire);
            crew->RemoveFirstAction();
            return;
        }

        progress += CREW_EXTINGUISH_SPEED * FIXED_DELTA_TIME;
    }
    else
    {
        // No fire effect at the target position; action cannot proceed
        crew->RemoveFirstAction();
    }
}

void RepairAction::Update(const std::shared_ptr<Crew> &crew)
{
    if (!crew)
        return;

    auto targetTile = _targetTile.lock();
    auto durability = targetTile ? targetTile->GetComponent<DurabilityComponent>() : nullptr;

    if (!targetTile || !durability)
    {
        crew->RemoveFirstAction();
        return;
    }

    const float maxHitpoints = durability->GetMaxHitpoints();
    float newHitpoints = std::min(durability->GetHitpoints() + CREW_REPAIR_SPEED * (float)FIXED_DELTA_TIME, maxHitpoints);

    durability->SetHitpoints(newHitpoints);

    if (newHitpoints >= maxHitpoints)
        crew->RemoveFirstAction();
}

void ConstructionAction::Update(const std::shared_ptr<Crew> &crew)
{
    if (!crew)
        return;

    auto task = _task.lock();
    if (!task)
    {
        crew->RemoveFirstAction();
        return;
    }

    auto station = crew->GetCurrentTile()->GetStation();
    if (!station)
    {
        crew->RemoveFirstAction();
        return;
    }

    task->progress += CREW_BUILD_SPEED * FIXED_DELTA_TIME;

    if (task->progress >= 1.f)
    {
        station->CompletePlannedTask(task->position);
        crew->RemoveFirstAction();
    }
}
