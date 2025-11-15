#pragma once
#include "tile_def.hpp"
#include <thread>
#include <deque>

class Action;
struct Crew;
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
    const std::unordered_map<uint64_t, std::shared_ptr<Crew>> &GetCrewList() const { return crewList; }
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
    void SendPlayerAction(uint64_t crewId, std::unique_ptr<Action> action);
    void ClearCrewActions(uint64_t crewId);
    void HandleAutonomousCrewDecisions();
    void ProcessPendingActions();

    double &GetTimeSinceFixedUpdate() { return timeSinceFixedUpdate; }

private:
    std::unordered_map<uint64_t, std::shared_ptr<Crew>> crewList;
    std::shared_ptr<Station> station;

    std::atomic<bool> paused = false;
    std::atomic<bool> isLocal = true;
    double timeSinceFixedUpdate = 0;

    std::thread updateThread;
    std::deque<std::pair<uint64_t, std::unique_ptr<Action>>> pendingActions;
    std::mutex pendingActionsMutex;

    std::shared_ptr<Crew> GetCrewById(uint64_t id) const;
};
