#include "action.hpp"
#include "astar.hpp"
#include "crew.hpp"
#include "station.hpp"

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

    Vector2Int floorCrewPos = ToVector2Int(crew->GetPosition());

    if (path.empty())
    {
        path = AStar(floorCrewPos, targetPosition, Vector2IntDistanceSq,
                     [station](const Vector2Int &p)
                     { return station->IsPositionPathable(p); });

        if (path.empty())
        {
            if (ToVector2(targetPosition) == crew->GetPosition())
            {
                crew->RemoveFirstAction();
                return;
            }

            targetPosition = floorCrewPos;
            path = {floorCrewPos};
        }
    }

    const float moveDelta = CREW_MOVE_SPEED * FIXED_DELTA_TIME;
    Vector2 stepPos = ToVector2(path.front());
    const float distanceLeftSq = Vector2DistanceSq(crew->GetPosition(), stepPos) - moveDelta * moveDelta;
    float distanceToTravel = moveDelta;

    // Crew can reach or pass the current waypoint
    if (distanceLeftSq <= 0)
    {
        if (auto doorTile = station->GetTileWithComponentAtPosition<DoorComponent>(floorCrewPos))
        {
            if (auto door = doorTile->GetComponent<DoorComponent>())
            {
                door->SetMovingState(DoorComponent::MovingState::IDLE);
            }
        }

        crew->SetPosition(stepPos);
        path.pop_front();

        if (path.empty())
        {
            crew->RemoveFirstAction();
            return;
        }

        // If there are any tiles in the way, clear path for recalculation
        if (DoesPathHaveObstacles(path, [station](const Vector2Int &p)
                                  { return station->IsPositionPathable(p); }))
        {
            path.clear();
            return;
        }

        distanceToTravel = std::sqrt(-distanceLeftSq);
        stepPos = ToVector2(path.front());
    }
    // Crew cannot reach the current waypoint in this frame
    else
    {
        if (auto doorTile = station->GetTileWithComponentAtPosition<DoorComponent>(path.front()))
        {
            if (auto door = doorTile->GetComponent<DoorComponent>())
            {
                door->SetMovingState(DoorComponent::MovingState::FORCED_OPEN);

                if (door->GetProgress() > 0) // Door is still opening/closing
                    return;                  // Wait for the door
            }
        }
    }

    crew->SetPosition(crew->GetPosition() + Vector2Normalize(stepPos - crew->GetPosition()) * distanceToTravel);
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

    if (auto fire = station->GetEffectOfTypeAtPosition<FireEffect>(targetPosition))
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

    auto station = crew->GetCurrentTile()->GetStation();
    if (!station)
    {
        crew->RemoveFirstAction();
        return;
    }

    Vector2Int crewPos = ToVector2Int(crew->GetPosition());
    if (crewPos != targetPosition)
    {
        // Crew not at position, perhaps move first, but for now, assume they are.
        crew->RemoveFirstAction();
        return;
    }

    // Find the planned task
    auto it = std::find_if(station->plannedTasks.begin(), station->plannedTasks.end(),
                           [this](const PlannedTask &task) { return task.position == targetPosition; });

    if (it == station->plannedTasks.end())
    {
        // No planned task, action done
        crew->RemoveFirstAction();
        return;
    }

    // Progress
    it->progress += FIXED_DELTA_TIME / 5.0f;

    if (it->progress >= 1.0f)
    {
        // Complete the task
        station->CompletePlannedTask(targetPosition);
        crew->RemoveFirstAction();
    }
}
