#include "asset_manager.hpp"
#include "audio_manager.hpp"
#include "def_manager.hpp"
#include "game_state.hpp"
#include "logging.hpp"
#include "lua_bindings.hpp"
#include "ui_manager.hpp"
#include "ui.hpp"
#include "update.hpp"

int main()
{
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Celestium");

    DefinitionManager::ParseConstantsFromFile("../assets/definitions/constants.yml");

    GameManager::GetCamera().SetFps(GetMonitorRefreshRate(GetCurrentMonitor()));
    SetExitKey(0);

    AssetManager::Initialize();
    GameManager::GetLua().open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string);
    RegisterAllLuaBindings(GameManager::GetLua());
    DefinitionManager::ParseResourcesFromFile("../assets/definitions/resources.yml");
    DefinitionManager::ParseTilesFromFile("../assets/definitions/tiles.yml");
    DefinitionManager::ParseEffectsFromFile("../assets/definitions/env_effects.yml");

    AudioManager::Initialize();

    GameManager::SetGameState(GameState::MAIN_MENU);

    auto &camera = GameManager::GetCamera();

    LogMessage(LogLevel::INFO, "Initialization Complete");

    while (GameManager::IsGameRunning())
    {
        BeginDrawing();
        ClearBackground(SPACE_COLOR);

        GameManager::HandleStateInputs();
        UiManager::Update();

        if (GameManager::IsInGameSim())
        {
            bool shouldForcePause = camera.GetUiState() != PlayerCam::UiState::NONE || GameManager::IsInBuildMode();
            GameManager::SetForcePaused(shouldForcePause);

            camera.HandleMovement();

            if (!UiManager::IsMouseOverUiElement())
            {
                if (GameManager::IsInBuildMode())
                {
                    HandleBuildMode();
                }
                else
                {
                    HandleCrewHover();
                    HandleCrewSelection();
                    AssignCrewActions();
                    HandleMouseDrag();
                }
            }

            if (GameManager::IsInBuildMode())
                DrawTileGrid();

            DrawStationTiles();
            DrawPlannedTasks();
            DrawStationOverlays();

            if (!GameManager::IsInBuildMode())
            {
                DrawCrew();
                DrawEnvironmentalEffects();
                DrawCrewActionProgress();
            }
            else
            {
                DrawBuildUi();
            }

            if (camera.IsUiClear())
            {
                DrawDragSelectBox();
                DrawMainTooltip();
                DrawFpsCounter();
                DrawResourceUI();
            }
        }

        UiManager::Render();

        AudioManager::Update();

        EndDrawing();

        GameManager::ApplyPendingState();

        if (WindowShouldClose())
            GameManager::SetGameState(GameState::NONE);
    }

    AssetManager::CleanUp();
    AudioManager::CleanUp();

    CloseWindow();

    LogMessage(LogLevel::INFO, "Clean-up Complete");
    return 0;
}