#include "audio_manager.hpp"
#include "ui.hpp"
#include "update.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>

std::mutex updateMutex;
std::condition_variable fixedUpdateCondition;

void FixedUpdate(std::shared_ptr<Station> station, std::vector<Crew> &crewList, double &timeSinceFixedUpdate)
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
                HandleCrewTasks(crewList);
                HandleCrewEnvironment(crewList);
                UpdateCrewCurrentTile(crewList, station);
                UpdateEnvironmentalEffects(station);
                UpdateTiles(station);
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

    GameManager::SetGameState(GameState::RUNNING);

    AssetManager::Initialize();
    DefinitionManager::ParseTilesFromFile("../assets/definitions/tiles.yml");
    DefinitionManager::ParseEffectsFromFile("../assets/definitions/env_effects.yml");

    AudioManager::Initialize();
    AudioManager::LoadSoundEffect("../assets/audio/fire_alarm.opus", SoundEffect::Type::EFFECT, true, .05);

    auto &camera = GameManager::GetCamera();

    std::vector<Crew> crewList{
        Crew("ALICE", {-2, 2}, RED),
        Crew("BOB", {3, 2}, GREEN),
        Crew("CHARLIE", {-3, -3}, ORANGE)};

    std::shared_ptr<Station> station = CreateStation();

    UiManager::InitializeElements();

    LogMessage(LogLevel::INFO, "Initialization Complete");

    double timeSinceFixedUpdate = 0;
    // Start the update thread
    std::thread updateThread(FixedUpdate, station, std::ref(crewList), std::ref(timeSinceFixedUpdate));

    double deltaTime = 0;

    while (GameManager::IsGameRunning())
    {
        bool forcePaused = camera.GetUiState() != PlayerCam::UiState::NONE || camera.IsInBuildMode();
        GameManager::SetGameState(GameState::FORCE_PAUSED, forcePaused);

        deltaTime = GetFrameTime();

        // Handle all real-time input and camera logic in the main thread
        camera.HandleCamera();

        if (!UiManager::IsMouseOverUiElement())
        {
            if (!camera.IsInBuildMode())
            {
                HandleCrewHover(crewList);
                HandleCrewSelection(crewList);
                AssignCrewTasks(crewList);
                HandleMouseDrag(station);
            }

            MouseDeleteExistingConnection(station);
        }

        if (IsKeyPressed(KEY_SPACE))
            GameManager::ToggleGameState(GameState::PAUSED);

        // Render logic
        BeginDrawing();
        ClearBackground(Color(31, 40, 45));

        DrawTileGrid();
        DrawStationTiles(station);
        DrawStationOverlays(station);

        if (!camera.IsInBuildMode())
        {
            DrawEnvironmentalEffects(station);
            DrawCrew(timeSinceFixedUpdate, crewList);
        }
        else
        {
            DrawBuildUi(station);
        }

        if (camera.IsUiClear())
        {
            DrawDragSelectBox();
            DrawMainTooltip(crewList, station);
            DrawFpsCounter(deltaTime, 12, DEFAULT_FONT_SIZE);
        }

        UiManager::Update();
        UiManager::Render();

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