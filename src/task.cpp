#include "astar.hpp"
#include "crew.hpp"
#include "station.hpp"
#include "task.hpp"

void MoveTask::Update(std::shared_ptr<Crew> crew)
{
    if (!crew)
        return;

    auto station = crew->GetCurrentTile()->GetStation();
    Vector2Int floorCrewPos = ToVector2Int(crew->GetPosition());

    if (path.empty())
    {
        path = AStar(floorCrewPos, targetPosition, station);

        if (path.size() <= 0)
        {
            if (ToVector2(targetPosition) == crew->GetPosition())
            {
                crew->RemoveFirstTask();
                return;
            }

            targetPosition = floorCrewPos;
            path.clear();
            path.push_back(floorCrewPos);
        }
    }

    constexpr float moveDelta = CREW_MOVE_SPEED * FIXED_DELTA_TIME;
    Vector2 stepPos = ToVector2(path.front());
    const float distanceLeftSq = Vector2DistanceSq(crew->GetPosition(), stepPos) - moveDelta * moveDelta;
    float distanceToTravel = moveDelta;
    if (distanceLeftSq <= 0)
    {
        if (auto doorTile = station->GetTileWithComponentAtPosition<DoorComponent>(floorCrewPos))
            doorTile->GetComponent<DoorComponent>()->SetMovingState(DoorComponent::MovingState::IDLE);

        crew->SetPosition(stepPos);
        path.pop_front();

        if (path.empty())
        {
            crew->RemoveFirstTask();
            return;
        }

        // If there are any tiles are in the way, clear path for recalculation
        if (DoesPathHaveObstacles(path, station))
        {
            path = {};
            return;
        }

        distanceToTravel = sqrtf(-distanceLeftSq);
        stepPos = ToVector2(path.front());
    }
    else
    {
        if (auto doorTile = station->GetTileWithComponentAtPosition<DoorComponent>(path.front()))
        {
            auto door = doorTile->GetComponent<DoorComponent>();
            door->SetMovingState(DoorComponent::MovingState::FORCED_OPEN);

            if (door->GetProgress() > 0)
                return;
        }
    }

    crew->SetPosition(crew->GetPosition() + Vector2Normalize(stepPos - crew->GetPosition()) * distanceToTravel);
}

void ExtinguishTask::Update(std::shared_ptr<Crew> crew)
{
    if (!crew)
        return;

    auto station = crew->GetCurrentTile()->GetStation();
    if (!station)
    {
        crew->RemoveFirstTask();
        return;
    }

    auto fire = station->GetEffectOfTypeAtPosition<FireEffect>(targetPosition);
    if (!fire)
    {
        crew->RemoveFirstTask();
        return;
    }

    if (progress > 1.)
    {
        station->RemoveEffect(fire);
        crew->RemoveFirstTask();
        return;
    }

    progress += CREW_EXTINGUISH_SPEED * FIXED_DELTA_TIME;
}

void RepairTask::Update(std::shared_ptr<Crew> crew)
{
    if (!crew)
        return;

    auto targetTile = _targetTile.lock();
    if (!targetTile)
    {
        crew->RemoveFirstTask();
        return;
    }

    auto durability = targetTile->GetComponent<DurabilityComponent>();
    if (!durability)
    {
        crew->RemoveFirstTask();
        return;
    }

    float newHitpoints = std::max(durability->GetHitpoints() + CREW_REPAIR_SPEED * (float)FIXED_DELTA_TIME, durability->GetMaxHitpoints());
    durability->SetHitpoints(newHitpoints);

    if (newHitpoints >= durability->GetMaxHitpoints())
    {
        crew->RemoveFirstTask();
        return;
    }
}
