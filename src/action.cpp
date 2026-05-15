#include "action.hpp"
#include "astar.hpp"
#include "component.hpp"
#include "pawn.hpp"
#include "planned_task.hpp"
#include "station.hpp"
#include "tile.hpp"

bool MoveAction::Update(const std::shared_ptr<Pawn> &pawn)
{
    if (!pawn)
        return true;

    auto station = pawn->GetCurrentTile()->GetStation();
    if (!station)
        return true;

    if (path.empty())
    {
        path = FindPath(pawn->GetPosition(), targetPosition, station->navPolygons, 
            [](const ConvexPolygon::Link& link) {
                if (auto doorTile = link.door.lock()) {
                    return doorTile->IsActive(); // Only path through powered doors
                }
                return true;
            });

        if (path.empty())
        {
            TraceLog(LOG_INFO, "MoveAction: FindPath returned empty. Target: (%f, %f), Pos: (%f, %f)", targetPosition.x, targetPosition.y, pawn->GetPosition().x, pawn->GetPosition().y);
            return true;
        }
    }

    const float moveDelta = PAWN_MOVE_SPEED * FIXED_DELTA_TIME;
    Vector2 nextWaypoint = path.front();
    Vector2 dir = Vector2Normalize(nextWaypoint - pawn->GetPosition());
    
    // Check for door at current position or slightly ahead
    std::shared_ptr<Tile> doorTile = nullptr;
    Vector2Int currentTilePos = {(int)std::floor(pawn->GetPosition().x + 0.5f), (int)std::floor(pawn->GetPosition().y + 0.5f)};
    doorTile = station->GetTileWithComponentAtPosition(currentTilePos, ComponentType::DOOR);
    
    if (!doorTile) {
        Vector2 checkPos = pawn->GetPosition() + dir * 0.6f;
        Vector2Int checkTilePos = {(int)std::floor(checkPos.x + 0.5f), (int)std::floor(checkPos.y + 0.5f)};
        doorTile = station->GetTileWithComponentAtPosition(checkTilePos, ComponentType::DOOR);
    }

    if (doorTile)
    {
        if (auto door = doorTile->GetComponent<DoorComponent>())
        {
            door->Open(1.0f / PAWN_MOVE_SPEED);

            if (!door->IsOpen())
                return false; // Wait for the door to be fully open
        }
    }

    float distToWaypoint = Vector2Distance(pawn->GetPosition(), nextWaypoint);
    
    if (distToWaypoint <= moveDelta)
    {
        // Reach waypoint
        pawn->SetPosition(nextWaypoint);
        
        // Reset door state if we just passed through one
        Vector2Int currentTilePos = ToVector2Int(pawn->GetPosition() / TILE_SIZE);
        if (auto doorTile = station->GetTileWithComponentAtPosition(currentTilePos, ComponentType::DOOR))
        {
            if (auto door = doorTile->GetComponent<DoorComponent>())
                door->Close();
        }

        path.pop_front();

        if (path.empty())
            return true;
    }
    else
    {
        // Move towards waypoint
        pawn->SetPosition(pawn->GetPosition() + Vector2Normalize(nextWaypoint - pawn->GetPosition()) * moveDelta);
    }
    return false;
}

bool ExtinguishAction::Update(const std::shared_ptr<Pawn> &pawn)
{
    if (!pawn)
        return true;

    auto station = pawn->GetCurrentTile()->GetStation();
    if (!station)
        return true;

    if (auto fire = station->GetEffectOfTypeAtPosition(targetPosition, "FIRE"))
    {
        if (progress > 1.f)
        {
            station->RemoveEffect(fire);
            return true;
        }

        progress += PAWN_EXTINGUISH_SPEED * FIXED_DELTA_TIME;
    }
    else
        return true;

    return false;
}

bool RepairAction::Update(const std::shared_ptr<Pawn> &pawn)
{
    if (!pawn)
        return true;

    auto targetTile = _targetTile.lock();
    auto durability = targetTile ? targetTile->GetComponent<DurabilityComponent>() : nullptr;

    if (!targetTile || !durability)
        return true;

    const float maxHitpoints = durability->GetMaxHitpoints();
    float newHitpoints = std::min(durability->GetHitpoints() + PAWN_REPAIR_SPEED * (float)FIXED_DELTA_TIME, maxHitpoints);

    durability->SetHitpoints(newHitpoints);

    return newHitpoints >= maxHitpoints;
}

bool ConstructionAction::Update(const std::shared_ptr<Pawn> &pawn)
{
    if (!pawn)
        return true;

    auto task = _task.lock();
    if (!task)
        return true;

    auto station = pawn->GetCurrentTile()->GetStation();
    if (!station)
        return true;

    task->progress += PAWN_BUILD_SPEED * FIXED_DELTA_TIME;

    if (task->progress >= 1.f)
    {
        station->CompletePlannedTask(task->position);
        return true;
    }
    return false;
}
