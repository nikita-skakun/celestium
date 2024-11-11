#include "asset_manager.hpp"
#include "audio_manager.hpp"
#include "def_manager.hpp"
#include "game_state.hpp"
#include "logging.hpp"
#include "ui_manager.hpp"
#include "ui.hpp"
#include "update.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>

std::mutex updateMutex;
std::condition_variable fixedUpdateCondition;

void FixedUpdate(double &timeSinceFixedUpdate)
{
    double previousTime = GetTime();

    while (GameManager::IsGameRunning())
    {
        if (!GameManager::IsGamePaused())
        {
            double currentTime = GetTime();
            double deltaTime = currentTime - previousTime;
            previousTime = currentTime;

            timeSinceFixedUpdate += deltaTime;

            while (timeSinceFixedUpdate >= FIXED_DELTA_TIME)
            {
                std::unique_lock<std::mutex> lock(updateMutex);

                // Perform the fixed updates
                HandleCrewTasks();
                HandleCrewEnvironment();
                UpdateCrewCurrentTile();
                UpdateEnvironmentalEffects();
                UpdateTiles();
                fixedUpdateCondition.notify_all();

                timeSinceFixedUpdate -= FIXED_DELTA_TIME;
            }
        }
        else
        {
            previousTime = GetTime();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main()
{
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Celestium");

    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    SetExitKey(0);

    AssetManager::Initialize();
    DefinitionManager::ParseTilesFromFile("../assets/definitions/tiles.yml");
    DefinitionManager::ParseEffectsFromFile("../assets/definitions/env_effects.yml");

    AudioManager::Initialize();

    GameManager::Initialize();
    GameManager::SetBuildModeState(true); // Done temporarily to test build mode

    auto &camera = GameManager::GetCamera();

    UiManager::InitializeElements();

    LogMessage(LogLevel::INFO, "Initialization Complete");

    double timeSinceFixedUpdate = 0;
    // Start the update thread
    std::thread updateThread(FixedUpdate, std::ref(timeSinceFixedUpdate));

    double deltaTime = 0;

    while (GameManager::IsGameRunning())
    {
        bool forcePaused = camera.GetUiState() != PlayerCam::UiState::NONE || GameManager::IsInBuildMode();
        GameManager::SetGameState(GameState::FORCE_PAUSED, forcePaused);

        deltaTime = GetFrameTime();

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
                AssignCrewTasks();
            }

            HandleMouseDrag();
            MouseDeleteExistingConnection();
        }

        if (IsKeyPressed(KEY_SPACE))
            GameManager::ToggleGameState(GameState::PAUSED);

        // Render logic
        BeginDrawing();
        ClearBackground(Color(31, 40, 45));

        DrawTileGrid();
        DrawStationTiles();
        DrawStationOverlays();

        if (!GameManager::IsInBuildMode())
        {
            DrawCrew(timeSinceFixedUpdate);
            DrawEnvironmentalEffects();
            DrawCrewTaskProgress();
        }
        else
        {
            DrawBuildUi();
        }

        if (camera.IsUiClear())
        {
            DrawDragSelectBox();
            DrawMainTooltip();
            DrawFpsCounter(deltaTime);
        }

        UiManager::Render();

        AudioManager::Update();

        EndDrawing();

        if (WindowShouldClose())
            GameManager::SetGameState(GameState::RUNNING, false);
    }

    updateThread.join();

    AssetManager::CleanUp();
    AudioManager::CleanUp();

    CloseWindow();

    LogMessage(LogLevel::INFO, "Clean-up Complete");
    return 0;
}