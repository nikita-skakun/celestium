#pragma once
#include "tile_def.hpp"
#include <deque>
#include <mutex>
#include <thread>

struct Action;
struct Pawn;
struct Station;

class GameServer
{
public:
    GameServer();
    ~GameServer();

    // Lifecycle
    void Initialize();
    void PrepareTestWorld();
    void StartSimulation();
    void StopSimulation();

    // Simulation getters
    const std::unordered_map<uint64_t, std::shared_ptr<Pawn>> &GetPawnList() const { return pawnList; }
    std::shared_ptr<Station> GetStation() const { return station; }
    bool IsGamePaused() const { return paused.load(); }

    void RequestPlannedTask(const Vector2Int &pos, const std::string &tileId, bool place);
    void RequestCancelPlannedTask(const Vector2Int &pos);
    void SetGamePaused(bool newState)
    {
        if (isLocal.load())
            paused.store(newState);
    }
    void ToggleGamePaused()
    {
        if (isLocal.load())
            paused.store(!paused.load());
    }
    bool IsLocal() const { return isLocal.load(); }
    void SendPlayerAction(uint64_t pawnId, std::unique_ptr<Action> action);
    void ClearPawnActions(uint64_t pawnId);
    void HandleAutonomousPawnDecisions();
    void ProcessPendingActions();

    double &GetTimeSinceFixedUpdate() { return timeSinceFixedUpdate; }

private:
    std::unordered_map<uint64_t, std::shared_ptr<Pawn>> pawnList;
    std::shared_ptr<Station> station;

    std::atomic<bool> paused = false;
    std::atomic<bool> isLocal = true;
    double timeSinceFixedUpdate = 0;

    std::thread updateThread;
    std::deque<std::pair<uint64_t, std::unique_ptr<Action>>> pendingActions;
    std::mutex pendingActionsMutex;

    std::shared_ptr<Pawn> GetPawnById(uint64_t id) const;
};
