#include "action.hpp"
#include "direction.hpp"
#include "fixed_update.hpp"
#include "game_server.hpp"
#include "pawn.hpp"
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
    pawnList.clear();
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
    std::vector<std::shared_ptr<Pawn>> tmpPawnList = {
        std::make_shared<Pawn>("ALICE", Vector2(-2, 2), RED),
        std::make_shared<Pawn>("BOB", Vector2(3, 2), GREEN),
        std::make_shared<Pawn>("CHARLIE", Vector2(-3, -3), ORANGE)};

    pawnList.clear();
    for (const auto &pawn : tmpPawnList)
        pawnList[pawn->GetInstanceId()] = pawn;

    {
        std::lock_guard<std::mutex> lock(pendingActionsMutex);
        pendingActions.clear();
    }
    paused = false;

    isLocal = true;
    timeSinceFixedUpdate = 0;
}

std::shared_ptr<Pawn> GameServer::GetPawnById(uint64_t id) const
{
    auto it = pawnList.find(id);
    return it != pawnList.end() ? it->second : nullptr;
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

void GameServer::SendPlayerAction(uint64_t pawnId, std::unique_ptr<Action> action)
{
    std::lock_guard<std::mutex> lock(pendingActionsMutex);
    pendingActions.emplace_back(pawnId, std::move(action));
}

void GameServer::ClearPawnActions(uint64_t pawnId)
{
    // Lock updateMutex first, then pendingActionsMutex to match ProcessPendingActions lock order
    std::unique_lock<std::mutex> lock(updateMutex);
    {
        std::lock_guard<std::mutex> pendingLock(pendingActionsMutex);
        // Remove any pending actions for this pawn so they don't get re-applied on next fixed update
        pendingActions.erase(
            std::remove_if(pendingActions.begin(), pendingActions.end(),
                           [pawnId](const std::pair<uint64_t, std::unique_ptr<Action>> &p)
                           { return p.first == pawnId; }),
            pendingActions.end());
    }

    if (auto _pawn = Find(pawnList, pawnId))
        if (auto pawn = _pawn.value()->second)
            pawn->GetActionQueue().clear();
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
        if (auto pawn = GetPawnById(entry.first))
        {
            if (!pawn->IsAlive())
            {
                TraceLog(TraceLogLevel::LOG_WARNING, std::format("Dropping action for dead pawn {}", pawn->GetName()).c_str());
                continue;
            }
            if (!pawn->GetCurrentTile())
            {
                TraceLog(TraceLogLevel::LOG_WARNING, std::format("Dropping action for pawn {} with no current tile", pawn->GetName()).c_str());
                continue;
            }

            pawn->GetActionQueue().push_back(std::move(entry.second));
        }
    }
}

void GameServer::HandleAutonomousPawnDecisions()
{
    if (!station)
        return;

    for (const auto &entry : pawnList)
    {
        const auto &pawn = entry.second;
        if (!pawn->IsAlive() || !pawn->GetActionQueue().empty() || !pawn->GetCurrentTile())
            continue;

        auto stationPtr = pawn->GetCurrentTile()->GetStation();
        Vector2Int pawnPos = ToVector2Int(pawn->GetPosition());

        if (stationPtr->GetEffectOfTypeAtPosition(pawnPos, "FIRE"))
        {
            pawn->GetActionQueue().push_back(std::make_shared<ExtinguishAction>(pawnPos));
            continue;
        }

        for (const auto &direction : ALL_DIRECTIONS)
        {
            Vector2Int neighborPos = pawnPos + DirectionToVector2Int(direction);
            if (stationPtr->GetEffectOfTypeAtPosition(neighborPos, "FIRE"))
            {
                pawn->GetActionQueue().push_back(std::make_shared<ExtinguishAction>(neighborPos));
                break;
            }
        }

        for (const auto &task : stationPtr->plannedTasks)
        {
            if (Vector2IntChebyshev(pawnPos, task->position) <= 1)
            {
                pawn->GetActionQueue().push_back(std::make_shared<ConstructionAction>(task));
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
