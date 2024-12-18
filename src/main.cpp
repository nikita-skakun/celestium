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
                HandleCrewActions();
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

        std::this_thread::sleep_for(std::chrono::milliseconds(std::max(1, (int)(1000 * (FIXED_DELTA_TIME - timeSinceFixedUpdate)))));
    }
}

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

    auto &camera = GameManager::GetCamera();

    uint16_t currentFpsIndex = GameManager::GetCamera().GetFpsIndex();
    LogMessage(LogLevel::DEBUG, std::format("Target FPS: {}", FPS_OPTIONS.at(currentFpsIndex)));

    UiManager::InitializeElements();

    LogMessage(LogLevel::INFO, "Initialization Complete");

    double timeSinceFixedUpdate = 0; // Elapsed time since the last fixed update in seconds
    // Start the update thread
    std::thread updateThread([&]()
                             { FixedUpdate(timeSinceFixedUpdate); });
    double deltaTime = 0;

    while (GameManager::IsGameRunning())
    {
        bool isForcePaused = camera.GetUiState() != PlayerCam::UiState::NONE || GameManager::IsInBuildMode();
        GameManager::SetGameState(GameState::FORCE_PAUSED, isForcePaused);

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
                AssignCrewActions();
            }

            HandleMouseDrag();
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
            DrawCrew(timeSinceFixedUpdate);
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