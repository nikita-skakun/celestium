#pragma once
#include "camera.hpp"
#include "render_snapshot.hpp"
#include "tile_def.hpp"
#include <sol/sol.hpp>
#include <thread>

enum class GameState : uint8_t
{
    NONE,
    MAIN_MENU, // Main menu is open
    GAME_SIM,  // Game simulation is running
};

struct GameManager
{
private:
    GameState state = GameState::NONE;
    std::optional<GameState> pendingState = std::nullopt;
    PlayerCam camera = PlayerCam();
    std::vector<std::shared_ptr<Crew>> crewList;
    std::vector<std::weak_ptr<Crew>> hoveredCrewList;
    std::vector<std::weak_ptr<Crew>> selectedCrewList;
    std::shared_ptr<Station> station;
    bool buildMode = false;
    bool paused = false;
    bool forcePaused = false;
    bool horizontalSymmetry = true;
    bool verticalSymmetry = false;
    TileDef::Category selectedCategory = TileDef::Category::NONE;
    std::string buildTileId = "";

    double timeSinceFixedUpdate = 0;
    std::thread updateThread;

    // Double-buffered render state
    std::atomic<std::shared_ptr<RenderSnapshot>> renderSnapshot;

public:
    // Render thread: get current snapshot
    static std::shared_ptr<RenderSnapshot> GetRenderSnapshot()
    {
        return GetInstance().renderSnapshot.load(std::memory_order_acquire);
    }
    // Simulation thread: set new snapshot
    static void SetRenderSnapshot(std::shared_ptr<RenderSnapshot> snap)
    {
        GetInstance().renderSnapshot.store(std::move(snap), std::memory_order_release);
    }

    GameManager() = default;
    ~GameManager() = default;
    GameManager(const GameManager &) = delete;
    GameManager &operator=(const GameManager &) = delete;

    static GameManager &GetInstance()
    {
        static GameManager instance;
        return instance;
    }

public:
    static GameState GetGameState() { return GetInstance().state; }
    static sol::state &GetLua();
    static bool IsGameRunning() { return GetInstance().state != GameState::NONE; }
    static bool IsInGameSim() { return GetGameState() == GameState::GAME_SIM; }
    static bool IsInMainMenu() { return GetGameState() == GameState::MAIN_MENU; }
    static void SetGameState(GameState state);

    static void RequestStateChange(GameState newState) { GetInstance().pendingState = newState; }
    static void ApplyPendingState()
    {
        auto &instance = GetInstance();
        if (instance.pendingState.has_value())
        {
            SetGameState(instance.pendingState.value());
            instance.pendingState.reset();
        }
    }

    static bool IsGamePaused() { return GetInstance().paused || GetInstance().forcePaused; }
    static void SetGamePaused(bool newState) { GetInstance().paused = newState; }
    static void ToggleGamePaused() { GetInstance().paused = !GetInstance().paused; }
    static void SetForcePaused(bool newState) { GetInstance().forcePaused = newState; }

    static double GetTimeSinceFixedUpdate() { return GetInstance().timeSinceFixedUpdate; }

    static PlayerCam &GetCamera() { return GetInstance().camera; }
    static void HandleStateInputs();

    static void Initialize();
    static void PrepareTestWorld();

    static const std::vector<std::shared_ptr<Crew>> &GetCrewList() { return GetInstance().crewList; }

    static std::shared_ptr<Station> GetStation() { return GetInstance().station; }

    static const std::vector<std::weak_ptr<Crew>> &GetHoveredCrew() { return GetInstance().hoveredCrewList; }
    static void ClearHoveredCrew() { GetInstance().hoveredCrewList.clear(); }
    static void AddHoveredCrew(const std::shared_ptr<Crew> &crew) { GetInstance().hoveredCrewList.push_back(crew); }

    static const std::vector<std::weak_ptr<Crew>> &GetSelectedCrew() { return GetInstance().selectedCrewList; }
    static void ClearSelectedCrew() { GetInstance().selectedCrewList.clear(); }
    static void AddSelectedCrew(std::weak_ptr<Crew> crew) { GetInstance().selectedCrewList.push_back(crew); }
    static void ToggleSelectedCrew(const std::shared_ptr<Crew> &crew);

    static TileDef::Category GetSelectedCategory() { return GetInstance().selectedCategory; }
    static void ToggleSelectedCategory(TileDef::Category category);

    static const std::string &GetBuildTileId() { return GetInstance().buildTileId; }
    static void SetBuildTileId(const std::string &tileId) { GetInstance().buildTileId = tileId; }
    static void ToggleBuildTileId(const std::string &tileId) { GetInstance().buildTileId = GetInstance().buildTileId == tileId ? "" : tileId; }
    static bool IsBuildTileId(const std::string &tileId) { return GetInstance().buildTileId == tileId; }

    static bool IsInBuildMode() { return GetInstance().buildMode; }
    static void SetBuildModeState(bool newState) { GetInstance().buildMode = newState; }
    static void ToggleBuildGameState() { GetInstance().buildMode = !GetInstance().buildMode; }

    static bool IsHorizontalSymmetry() { return GetInstance().horizontalSymmetry; }
    static void SetHorizontalSymmetry(bool newState) { GetInstance().horizontalSymmetry = newState; }
    static void ToggleHorizontalSymmetry() { GetInstance().horizontalSymmetry = !GetInstance().horizontalSymmetry; }

    static bool IsVerticalSymmetry() { return GetInstance().verticalSymmetry; }
    static void SetVerticalSymmetry(bool newState) { GetInstance().verticalSymmetry = newState; }
    static void ToggleVerticalSymmetry() { GetInstance().verticalSymmetry = !GetInstance().verticalSymmetry; }

    // Utility function for Screen to World space transformations
    static Vector2 GetWorldMousePos();
    static Vector2 ScreenToWorld(const Vector2 &screenPos);
    static Vector2Int ScreenToTile(const Vector2 &screenPos);
    static Vector2 WorldToScreen(const Vector2 &worldPos);
    static Vector2 WorldToScreen(const Vector2Int &worldPos);
    static Rectangle WorldToScreen(const Rectangle &worldRect);
};
