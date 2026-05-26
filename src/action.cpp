#include "action.hpp"
#include "astar.hpp"
#include "component.hpp"
#include "pawn.hpp"
#include "planned_task.hpp"
#include "station.hpp"
#include "tile.hpp"

bool MoveAction::Update(const std::shared_ptr<Pawn> &pawn)
{
    isMoving = false;

    if (!pawn)
        return true;

    auto currentTile = pawn->GetCurrentTile();
    auto station = currentTile ? currentTile->GetStation() : nullptr;
    if (!station)
        return true;

    if (path.empty())
    {
        path = FindPath(pawn->GetPosition(), targetPosition, station->navPolygons,
                        [](const ConvexPolygon::Link &link)
                        {
                            if (auto doorTile = link.door.lock())
                                return doorTile->IsActive(); // Only path through powered doors
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

    // Update pawn facing direction based on movement direction
    // Determine the closest cardinal direction to the movement vector
    Direction closestDir = Direction::S; // Default
    float maxDot = -2.f;
    for (auto cardinalDir : CARDINAL_DIRECTIONS)
    {
        Vector2Int dirVec = DirectionToVector2Int(cardinalDir);
        Vector2 normDir = Vector2Normalize(Vector2(static_cast<float>(dirVec.x), static_cast<float>(dirVec.y)));
        float dot = Vector2Dot(dir, normDir);
        if (dot > maxDot)
        {
            maxDot = dot;
            closestDir = cardinalDir;
        }
    }
    pawn->SetFacingDirection(closestDir);

    // Check for door at current position or slightly ahead
    std::shared_ptr<Tile> doorTile = nullptr;
    Vector2Int currentTilePos = {(int)std::floor(pawn->GetPosition().x + .5f), (int)std::floor(pawn->GetPosition().y + .5f)};
    doorTile = station->GetTileWithComponentAtPosition(currentTilePos, ComponentType::DOOR);

    if (!doorTile)
    {
        Vector2 checkPos = pawn->GetPosition() + dir * .6f;
        Vector2Int checkTilePos = {(int)std::floor(checkPos.x + .5f), (int)std::floor(checkPos.y + .5f)};
        doorTile = station->GetTileWithComponentAtPosition(checkTilePos, ComponentType::DOOR);
    }

    if (doorTile)
    {
        if (auto door = doorTile->GetComponent<DoorComponent>())
        {
            door->Open(1.f / PAWN_MOVE_SPEED);

            if (!door->IsOpen())
                return false; // Wait for the door to be fully open
        }
    }

    float distToWaypoint = Vector2Distance(pawn->GetPosition(), nextWaypoint);

    if (distToWaypoint <= moveDelta)
    {
        // Reach waypoint
        isMoving = distToWaypoint > 0.f;
        pawn->SetPosition(nextWaypoint);

        // Reset door state if we just passed through one
        Vector2Int currentTilePos = ToVector2Int(pawn->GetPosition());
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
        isMoving = true;
        pawn->SetPosition(pawn->GetPosition() + Vector2Normalize(nextWaypoint - pawn->GetPosition()) * moveDelta);
    }
    return false;
}

bool ExtinguishAction::Update(const std::shared_ptr<Pawn> &pawn)
{
    if (!pawn)
        return true;

    auto currentTile = pawn->GetCurrentTile();
    auto station = currentTile ? currentTile->GetStation() : nullptr;
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

    auto currentTile = pawn->GetCurrentTile();
    auto station = currentTile ? currentTile->GetStation() : nullptr;
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
