#include "action.hpp"
#include "crew.hpp"
#include "direction.hpp"
#include "fixed_update.hpp"
#include "game_server.hpp"
#include "planned_task.hpp"
#include "station.hpp"
#include "tile.hpp"

GameServer::GameServer() = default;
GameServer::~GameServer()
{
    StopSimulation();
}

void GameServer::Initialize()
{
    crewList.clear();
    station = nullptr;
    paused = false;

    isLocal = true;
    {
        std::lock_guard<std::mutex> lock(pendingActionsMutex);
        pendingActions.clear();
    }
    timeSinceFixedUpdate = 0;
}

void GameServer::PrepareTestWorld()
{
    station = CreateStation();
    std::vector<std::shared_ptr<Crew>> tmpCrewList = {
        std::make_shared<Crew>("ALICE", Vector2(-2, 2), RED),
        std::make_shared<Crew>("BOB", Vector2(3, 2), GREEN),
        std::make_shared<Crew>("CHARLIE", Vector2(-3, -3), ORANGE)};

    crewList.clear();
    for (const auto &crew : tmpCrewList)
        crewList[crew->GetInstanceId()] = crew;

    {
        std::lock_guard<std::mutex> lock(pendingActionsMutex);
        pendingActions.clear();
    }
    paused = false;

    isLocal = true;
    timeSinceFixedUpdate = 0;
}

std::shared_ptr<Crew> GameServer::GetCrewById(uint64_t id) const
{
    auto it = crewList.find(id);
    return it != crewList.end() ? it->second : nullptr;
}

void GameServer::StartSimulation()
{
    if (updateThread.joinable())
        return;

    updateThread = std::thread([this]()
                               { FixedUpdate(this->timeSinceFixedUpdate); });
}

void GameServer::StopSimulation()
{
    if (updateThread.joinable())
        updateThread.join();
}

void GameServer::SendPlayerAction(uint64_t crewId, std::unique_ptr<Action> action)
{
    std::lock_guard<std::mutex> lock(pendingActionsMutex);
    pendingActions.emplace_back(crewId, std::move(action));
}

void GameServer::ClearCrewActions(uint64_t crewId)
{
    // Lock updateMutex first, then pendingActionsMutex to match ProcessPendingActions lock order
    std::unique_lock<std::mutex> lock(updateMutex);
    {
        std::lock_guard<std::mutex> pendingLock(pendingActionsMutex);
        // Remove any pending actions for this crew so they don't get re-applied on next fixed update
        pendingActions.erase(
            std::remove_if(pendingActions.begin(), pendingActions.end(),
                           [crewId](const std::pair<uint64_t, std::unique_ptr<Action>> &p)
                           { return p.first == crewId; }),
            pendingActions.end());
    }

    if (auto _crew = Find(crewList, crewId))
        if (auto crew = _crew.value()->second)
            crew->GetActionQueue().clear();
}

void GameServer::ProcessPendingActions()
{
    std::deque<std::pair<uint64_t, std::unique_ptr<Action>>> actionsToProcess;
    {
        std::lock_guard<std::mutex> lock(pendingActionsMutex);
        actionsToProcess.swap(pendingActions);
    }

    for (auto &entry : actionsToProcess)
    {
        if (auto crew = GetCrewById(entry.first))
        {
            if (!crew->IsAlive())
            {
                TraceLog(TraceLogLevel::LOG_WARNING, std::format("Dropping action for dead crew {}", crew->GetName()).c_str());
                continue;
            }
            if (!crew->GetCurrentTile())
            {
                TraceLog(TraceLogLevel::LOG_WARNING, std::format("Dropping action for crew {} with no current tile", crew->GetName()).c_str());
                continue;
            }

            crew->GetActionQueue().push_back(std::move(entry.second));
        }
    }
}

void GameServer::HandleAutonomousCrewDecisions()
{
    if (!station)
        return;

    for (const auto &entry : crewList)
    {
        const auto &crew = entry.second;
        if (!crew->IsAlive() || !crew->GetActionQueue().empty() || !crew->GetCurrentTile())
            continue;

        auto stationPtr = crew->GetCurrentTile()->GetStation();
        Vector2Int crewPos = ToVector2Int(crew->GetPosition());

        if (stationPtr->GetEffectOfTypeAtPosition(crewPos, "FIRE"))
        {
            crew->GetActionQueue().push_back(std::make_shared<ExtinguishAction>(crewPos));
            continue;
        }

        for (const auto &direction : ALL_DIRECTIONS)
        {
            Vector2Int neighborPos = crewPos + DirectionToVector2Int(direction);
            if (stationPtr->GetEffectOfTypeAtPosition(neighborPos, "FIRE"))
            {
                crew->GetActionQueue().push_back(std::make_shared<ExtinguishAction>(neighborPos));
                break;
            }
        }

        for (const auto &task : stationPtr->plannedTasks)
        {
            if (Vector2IntChebyshev(crewPos, task->position) <= 1)
            {
                crew->GetActionQueue().push_back(std::make_shared<ConstructionAction>(task));
                break;
            }
        }
    }
}

void GameServer::RequestPlannedTask(const Vector2Int &pos, const std::string &tileId, bool place)
{
    std::unique_lock<std::mutex> lock(updateMutex);
    if (!station)
        return;
    station->AddPlannedTask(pos, tileId, place);
}

void GameServer::RequestCancelPlannedTask(const Vector2Int &pos)
{
    std::unique_lock<std::mutex> lock(updateMutex);
    if (!station)
        return;
    if (station->HasPlannedTaskAt(pos))
        station->CancelPlannedTask(pos);
}
