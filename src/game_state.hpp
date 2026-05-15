#pragma once
#include "tile_enums.hpp"
#include "utils.hpp"
#include <atomic>

struct PlayerCam;
class GameServer;
struct RenderSnapshot;
namespace sol { class state; }

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
    std::unique_ptr<PlayerCam> camera;
    // Server: owns the authoritative simulation state. For single-player, this will
    // be a local server instance; for multiplayer, this can be a network client
    // forwarding requests to a remote server.
    std::unique_ptr<GameServer> server;

    bool buildMode = false;
    bool cancelMode = false;
    bool horizontalSymmetry = true;
    bool verticalSymmetry = false;
    TileCategory selectedCategory = TileCategory::NONE;
    std::string buildTileId = "";
    std::vector<uint64_t> hoveredPawnList;
    std::vector<uint64_t> selectedPawnList;
    Vector2 originalScreenSize;

    // Double-buffered render state
    std::atomic<std::shared_ptr<RenderSnapshot>> renderSnapshot;

public:
    static std::shared_ptr<RenderSnapshot> GetRenderSnapshot();
    static void SetRenderSnapshot(std::shared_ptr<RenderSnapshot> snap);

    GameManager();
    ~GameManager();
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
    static void ApplyPendingState();

    static const std::vector<uint64_t> &GetHoveredPawn() { return GetInstance().hoveredPawnList; }
    static void ClearHoveredPawn() { GetInstance().hoveredPawnList.clear(); }
    static void AddHoveredPawn(uint64_t pawnId) { GetInstance().hoveredPawnList.push_back(pawnId); }

    static const std::vector<uint64_t> &GetSelectedPawn() { return GetInstance().selectedPawnList; }
    static void ClearSelectedPawn() { GetInstance().selectedPawnList.clear(); }
    static void AddSelectedPawn(uint64_t pawnId) { GetInstance().selectedPawnList.push_back(pawnId); }
    static void ToggleSelectedPawn(uint64_t pawnId);

    static TileCategory GetSelectedCategory() { return GetInstance().selectedCategory; }
    static void ToggleSelectedCategory(TileCategory category);

    static const std::string &GetBuildTileId() { return GetInstance().buildTileId; }
    static void SetBuildTileId(const std::string &tileId) { GetInstance().buildTileId = tileId; }
    static void ToggleBuildTileId(const std::string &tileId) { GetInstance().buildTileId = GetInstance().buildTileId == tileId ? "" : tileId; }
    static bool IsBuildTileId(const std::string &tileId) { return GetInstance().buildTileId == tileId; }

    static std::vector<Vector2Int> GetSymmetryPositions(const Vector2Int &pos);

    static bool IsInBuildMode() { return GetInstance().buildMode; }
    static void SetBuildModeState(bool newState) { GetInstance().buildMode = newState; }
    static void ToggleBuildGameState() { GetInstance().buildMode = !GetInstance().buildMode; }

    static bool IsInCancelMode() { return GetInstance().cancelMode; }
    static void SetCancelMode(bool newState) { GetInstance().cancelMode = newState; }
    static void ToggleCancelMode() { GetInstance().cancelMode = !GetInstance().cancelMode; }

    static bool IsHorizontalSymmetry() { return GetInstance().horizontalSymmetry; }
    static void SetHorizontalSymmetry(bool newState) { GetInstance().horizontalSymmetry = newState; }
    static void ToggleHorizontalSymmetry() { GetInstance().horizontalSymmetry = !GetInstance().horizontalSymmetry; }

    static bool IsVerticalSymmetry() { return GetInstance().verticalSymmetry; }
    static void SetVerticalSymmetry(bool newState) { GetInstance().verticalSymmetry = newState; }
    static void ToggleVerticalSymmetry() { GetInstance().verticalSymmetry = !GetInstance().verticalSymmetry; }

    static PlayerCam &GetCamera();
    static void HandleStateInputs();

    static void Initialize();
    static void PrepareTestWorld();
    static GameServer &GetServer();

    // Utility function for Screen to World space transformations
    static Vector2 GetWorldMousePos();
    static Vector2 ScreenToWorld(const Vector2 &screenPos);
    static Vector2Int ScreenToTile(const Vector2 &screenPos);
    static Vector2 WorldToScreen(const Vector2 &worldPos);
    static Vector2 WorldToScreen(const Vector2Int &worldPos);
    static Rectangle WorldToScreen(const Rectangle &worldRect);

    static const Vector2 &GetOriginalScreenSize();
    static void SetOriginalScreenSize();
};
