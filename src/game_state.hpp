#pragma once
#include "camera.hpp"
#include "tile_def.hpp"
#include <thread>

struct Crew;
struct Station;
struct Tile;

enum class GameState : uint8_t
{
    NONE,
    MAIN_MENU, // Main menu is open
    GAME_SIM,  // Game simulation is running
};

struct GameManager
{
private:
    GameState state = GameState::MAIN_MENU;
    PlayerCam camera = PlayerCam();
    std::vector<std::shared_ptr<Crew>> crewList;
    std::vector<std::weak_ptr<Crew>> hoveredCrewList;
    std::vector<std::weak_ptr<Crew>> selectedCrewList;
    // std::vector<std::weak_ptr<Tile>> selectedTileList;
    // TileDef::Height selectedHeight = TileDef::Height::NONE;
    std::shared_ptr<Station> station;
    // std::weak_ptr<Tile> moveTile;
    bool buildMode = false;
    bool paused = false;
    bool forcePaused = false;
    bool horizontalSymmetry = true;
    bool verticalSymmetry = false;
    std::string buildTileId = "";

    double timeSinceFixedUpdate = 0;
    std::thread updateThread;

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
    static bool IsGameRunning() { return GetInstance().state != GameState::NONE; }
    static bool IsInGameSim() { return GetGameState() == GameState::GAME_SIM; }
    static bool IsInMainMenu() { return GetGameState() == GameState::MAIN_MENU; }
    static void SetGameState(GameState state);

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

    // static TileDef::Height GetSelectedHeight() { return GetInstance().selectedHeight; }
    // static void ToggleSelectedHeight(TileDef::Height height) { GetInstance().selectedHeight ^= height; }
    // static void ClearSelectedHeight() { GetInstance().selectedHeight = TileDef::Height::NONE; }

    // static const std::vector<std::weak_ptr<Tile>> &GetSelectedTiles() { return GetInstance().selectedTileList; }
    // static void ClearSelectedTiles() { GetInstance().selectedTileList.clear(); }
    // static void ToggleSelectedTile(const std::shared_ptr<Tile> &tile);
    // static void SetSelectedTile(const std::shared_ptr<Tile> &tile, bool select = true);
    // static bool IsTileSelected(const std::shared_ptr<Tile> &tile);

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

    // static bool IsInMoveMode() { return !GetInstance().moveTile.expired(); }
    // static std::shared_ptr<Tile> GetMoveTile() { return GetInstance().moveTile.lock(); }
    // static void ClearMoveTile() { GetInstance().moveTile.reset(); }
    // static void SetMoveTile()
    // {
    //     GetInstance().moveTile = GetSelectedTile();
    //     SetSelectedTile();
    // }

    // Utility function for Screen to World space transformations
    static Vector2 GetWorldMousePos();
    static Vector2 ScreenToWorld(const Vector2 &screenPos);
    static Vector2Int ScreenToTile(const Vector2 &screenPos);
    static Vector2 WorldToScreen(const Vector2 &worldPos);
    static Vector2 WorldToScreen(const Vector2Int &worldPos);
    static Rectangle WorldToScreen(const Rectangle &worldRect);
};
