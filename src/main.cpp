#include "asset_manager.hpp"
#include "audio_manager.hpp"
#include "def_manager.hpp"
#include "game_state.hpp"
#include "logging.hpp"
#include "ui_manager.hpp"
#include "ui.hpp"
#include "update.hpp"

int main()
{
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Celestium");

    GameManager::GetCamera().SetFps(GetMonitorRefreshRate(GetCurrentMonitor()));
    SetExitKey(0);

    AssetManager::Initialize();
    DefinitionManager::ParseTilesFromFile("../assets/definitions/tiles.yml");
    DefinitionManager::ParseEffectsFromFile("../assets/definitions/env_effects.yml");

    AudioManager::Initialize();

    GameManager::Initialize();
    GameManager::SetGameState(GameState::GAME_SIM);

    auto &camera = GameManager::GetCamera();

    uint16_t currentFpsIndex = GameManager::GetCamera().GetFpsIndex();
    LogMessage(LogLevel::DEBUG, std::format("Target FPS: {}", FPS_OPTIONS.at(currentFpsIndex)));

    UiManager::InitializeElements();

    LogMessage(LogLevel::INFO, "Initialization Complete");

    while (GameManager::IsGameRunning())
    {
        bool isForcePaused = camera.GetUiState() != PlayerCam::UiState::NONE || GameManager::IsInBuildMode();
        GameManager::SetForcePaused(isForcePaused);

        // Handle all real-time input and camera logic in the main thread
        camera.HandleMovement();
        UiManager::Update();
        GameManager::HandleStateInputs();

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

            MouseDeleteExistingConnection();
        }

        // Render logic
        BeginDrawing();
        ClearBackground(Color(31, 40, 45));

        DrawTileGrid();
        DrawStationTiles();
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
        }

        UiManager::Render();

        AudioManager::Update();

        EndDrawing();

        if (WindowShouldClose())
            GameManager::SetGameState(GameState::NONE);
    }

    AssetManager::CleanUp();
    AudioManager::CleanUp();

    CloseWindow();

    LogMessage(LogLevel::INFO, "Clean-up Complete");
    return 0;
}