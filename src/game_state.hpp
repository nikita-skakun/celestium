#pragma once
#include "camera.hpp"
#include "tile_def.hpp"

struct Crew;
struct Station;
struct Tile;

enum class GameState : uint8_t
{
    NONE = 0,
    RUNNING = 1 << 0,
    PAUSED = 1 << 1,
    FORCE_PAUSED = 1 << 2,
};

template <>
struct magic_enum::customize::enum_range<GameState>
{
    static constexpr bool is_flags = true;
};

struct GameManager
{
private:
    GameState state = GameState::NONE;
    PlayerCam camera = PlayerCam();
    std::vector<std::shared_ptr<Crew>> crewList;
    std::vector<std::weak_ptr<Crew>> hoveredCrewList;
    std::vector<std::weak_ptr<Crew>> selectedCrewList;
    // std::vector<std::weak_ptr<Tile>> selectedTileList;
    // TileDef::Height selectedHeight = TileDef::Height::NONE;
    std::shared_ptr<Station> station;
    // std::weak_ptr<Tile> moveTile;
    bool buildMode = false;
    bool horizontalSymmetry = true;
    bool verticalSymmetry = false;
    std::string buildTileId = "";

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
    static bool IsGameRunning() { return magic_enum::enum_flags_test(GetGameState(), GameState::RUNNING); }
    static bool IsGamePaused() { return magic_enum::enum_flags_test_any(GetGameState(), GameState::PAUSED | GameState::FORCE_PAUSED); }
    static void SetGameState(GameState mask, bool bitState = true);
    static void ToggleGameState(GameState mask) { GetInstance().state ^= mask; }

    static PlayerCam &GetCamera() { return GetInstance().camera; }
    static void HandleStateInputs();

    static void Initialize();

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
