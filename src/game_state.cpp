#include "crew.hpp"
#include "direction.hpp"
#include "fixed_update.hpp"
#include "game_state.hpp"
#include "station.hpp"
#include "ui.hpp"
#include "ui_manager.hpp"

void GameManager::SetGameState(GameState state)
{
    auto &manager = GetInstance();
    if (state == GameManager::GetGameState())
        return;

    manager.state = state;

    if (state != GameState::GAME_SIM && manager.updateThread.joinable())
    {
        manager.updateThread.join();
        manager.timeSinceFixedUpdate = 0;
    }

    UiManager::ClearAllElements();
    GameManager::SetRenderSnapshot(nullptr);
    ClearRenderSystems();
    ClearStarfield();

    switch (state)
    {
    case GameState::MAIN_MENU:
    {
        Initialize();
        UiManager::InitializeMainMenu();
        break;
    }
    case GameState::GAME_SIM:
    {
        CreateStarfield(12345); // TODO: Use seed from save file
        PrepareTestWorld();
        UiManager::InitializeGameSim();

        manager.updateThread = std::thread([&]()
                                           { FixedUpdate(manager.timeSinceFixedUpdate); });
        break;
    }
    default:
        break;
    }
}

/**
 * Toggles camera state based on user key input.
 */
void GameManager::HandleStateInputs()
{
    auto &camera = GetInstance().camera;

    if (IsKeyPressed(KEY_ESCAPE))
    {
        if (IsInGameSim())
            camera.ToggleUiState(PlayerCam::UiState::ESC_MENU);
        else
            camera.ToggleUiState(PlayerCam::UiState::NONE);
    }

    if (camera.IsUiState(PlayerCam::UiState::NONE) && IsInGameSim())
    {
        if (IsKeyPressed(KEY_SPACE) && !IsInBuildMode())
            ToggleGamePaused();

        if (IsKeyPressed(KEY_O))
            camera.ToggleOverlay(PlayerCam::Overlay::OXYGEN);

        if (IsKeyPressed(KEY_W))
            camera.ToggleOverlay(PlayerCam::Overlay::WALL);

        if (IsKeyPressed(KEY_P))
            camera.ToggleOverlay(PlayerCam::Overlay::POWER);

        if (IsKeyPressed(KEY_B))
            ToggleBuildGameState();
    }
}

void GameManager::Initialize()
{
    auto &manager = GetInstance();

    manager.camera = PlayerCam();
    manager.crewList.clear();
    manager.hoveredCrewList.clear();
    manager.selectedCrewList.clear();
    manager.station = nullptr;
    manager.buildMode = false;
    manager.cancelMode = false;
    manager.paused = false;
    manager.forcePaused = false;
    manager.horizontalSymmetry = false;
    manager.verticalSymmetry = false;
    manager.buildTileId = "";
}

void GameManager::PrepareTestWorld()
{
    auto &manager = GetInstance();

    manager.camera = PlayerCam();
    manager.station = CreateStation();
    manager.crewList = {
        std::make_shared<Crew>("ALICE", Vector2(-2, 2), RED),
        std::make_shared<Crew>("BOB", Vector2(3, 2), GREEN),
        std::make_shared<Crew>("CHARLIE", Vector2(-3, -3), ORANGE)};
    manager.selectedCrewList.clear();
    manager.hoveredCrewList.clear();
    manager.buildMode = false;
    manager.cancelMode = false;
    manager.buildTileId = "";
}

void GameManager::ToggleSelectedCrew(const std::shared_ptr<Crew> &crew)
{
    auto &selectedCrewList = GetInstance().selectedCrewList;
    const auto crewIter = std::find_if(selectedCrewList.begin(), selectedCrewList.end(), [&crew](std::weak_ptr<Crew> _crew)
                                       { return !_crew.expired() && _crew.lock() == crew; });

    if (crewIter == selectedCrewList.end())
        selectedCrewList.push_back(crew);
    else
        selectedCrewList.erase(crewIter);
}

void GameManager::ToggleSelectedCategory(TileDef::Category category)
{
    auto &selectedCategory = GetInstance().selectedCategory;
    selectedCategory = (selectedCategory == category) ? TileDef::Category::NONE : category;
}

/**
 * Converts the current mouse position from screen coordinates to world coordinates.
 *
 * @return The world position of the mouse as a Vector2.
 */
Vector2 GameManager::GetWorldMousePos()
{
    return ScreenToWorld(GetMousePosition());
}

/**
 * Converts screen coordinates to world coordinates based on the camera's position and zoom level.
 *
 * @param screenPos The position in screen coordinates.
 * @return The corresponding position in world coordinates as a Vector2.
 */
Vector2 GameManager::ScreenToWorld(const Vector2 &screenPos)
{
    auto &camera = GetInstance().camera;
    return (screenPos - (GetScreenSize() / 2.)) / camera.GetZoom() / TILE_SIZE + camera.GetPosition();
}

/**
 * Converts screen coordinates to the corresponding tile position in the world.
 *
 * @param screenPos The position in screen coordinates.
 * @return The tile position in world coordinates as a Vector2, rounded down to the nearest tile.
 */
Vector2Int GameManager::ScreenToTile(const Vector2 &screenPos)
{
    return ToVector2Int(ScreenToWorld(screenPos));
}

/**
 * Converts world coordinates to screen coordinates based on the camera's position and zoom level.
 *
 * @param worldPos The position in world coordinates.
 * @return The position in screen coordinates as a Vector2.
 */
Vector2 GameManager::WorldToScreen(const Vector2 &worldPos)
{
    auto &camera = GetInstance().camera;
    return (worldPos + Vector2(.5, .5) - camera.GetPosition()) * TILE_SIZE * camera.GetZoom() + (GetScreenSize() / 2.);
}

/**
 * Converts world coordinates to screen coordinates based on the camera's position and zoom level.
 *
 * @param worldPos The position in world coordinates.
 * @return The position in screen coordinates as a Vector2.
 */
Vector2 GameManager::WorldToScreen(const Vector2Int &worldPos)
{
    auto &camera = GetInstance().camera;
    return (ToVector2(worldPos) + Vector2(.5, .5) - camera.GetPosition()) * TILE_SIZE * camera.GetZoom() + (GetScreenSize() / 2.);
}

Rectangle GameManager::WorldToScreen(const Rectangle &worldRect)
{
    auto &camera = GetInstance().camera;
    Vector2 screenPos = (RectToPos(worldRect) - camera.GetPosition()) * TILE_SIZE * camera.GetZoom() + (GetScreenSize() / 2.);
    Vector2 screenSize = RectToSize(worldRect) * TILE_SIZE * camera.GetZoom();

    return Vector2ToRect(screenPos, screenSize);
}

const Vector2 &GameManager::GetOriginalScreenSize()
{
    return GetInstance().originalScreenSize;
}

void GameManager::SetOriginalScreenSize()
{
    GetInstance().originalScreenSize = GetScreenSize();
}

static sol::state lua;
sol::state &GameManager::GetLua() { return lua; }