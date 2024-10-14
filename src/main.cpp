#include "ui.hpp"
#include "update.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>

std::mutex updateMutex;
std::condition_variable fixedUpdateCondition;

void FixedUpdate(std::shared_ptr<Station> station, std::vector<Crew> &crewList, double &timeSinceFixedUpdate, GameState &state)
{
    double previousTime = GetTime();

    while (IsGameRunning(state))
    {
        if (!IsGamePaused(state))
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
                UpdateEnvironmentalHazards(station);
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

    GameState state = GameState::RUNNING;

    Texture2D stationTileset = LoadTexture("../assets/tilesets/station.png");
    Texture2D iconTileset = LoadTexture("../assets/tilesets/icons.png");
    Texture2D fireSpritesheet = LoadTexture("../assets/tilesets/fire.png");
    Font font = LoadFontEx("../assets/fonts/Jersey25.ttf", DEFAULT_FONT_SIZE, NULL, 0);

    TileDefinitionRegistry::GetInstance().ParseTilesFromFile("../assets/definitions/tiles.yml");

    PlayerCam camera = PlayerCam();

    std::vector<Crew> crewList{
        Crew("ALICE", {-2, 2}, RED),
        Crew("BOB", {3, 2}, GREEN),
        Crew("CHARLIE", {-3, -3}, ORANGE)};

    std::shared_ptr<Station> station = CreateStation();

    UiManager::GetInstance().InitializeElements(iconTileset, state, camera, font);

    LogMessage(LogLevel::INFO, "Initialization Complete");

    double timeSinceFixedUpdate = 0;
    // Start the update thread
    std::thread updateThread(FixedUpdate, station, std::ref(crewList), std::ref(timeSinceFixedUpdate), std::ref(state));

    double deltaTime = 0;

    while (IsGameRunning(state))
    {
        bool forcePaused = camera.GetUiState() != PlayerCam::UiState::NONE || camera.IsInBuildMode();
        SetBit(state, forcePaused, GameState::FORCE_PAUSED);

        deltaTime = GetFrameTime();

        // Handle all real-time input and camera logic in the main thread
        camera.HandleCamera();
        if (!camera.IsInBuildMode())
        {
            HandleCrewHover(crewList, camera);
            HandleCrewSelection(crewList, camera);
            AssignCrewTasks(crewList, camera);
            HandleMouseDrag(station, camera);
        }

        if (camera.IsUiClear())
        {
            MouseDeleteExistingConnection(station, camera);
        }

        if (IsKeyPressed(KEY_SPACE))
            ToggleBit(state, GameState::PAUSED);

        // Render logic
        BeginDrawing();
        ClearBackground(Color(31, 40, 45));

        DrawTileGrid(camera);
        DrawStationTiles(station, stationTileset, camera);
        DrawStationOverlays(station, stationTileset, iconTileset, camera);

        if (!camera.IsInBuildMode())
        {
            DrawEnvironmentalHazards(station, fireSpritesheet, camera);
            DrawCrew(timeSinceFixedUpdate, crewList, camera);
        }
        else
        {
            Vector2Int cursorPos = ToVector2Int(camera.GetWorldMousePos());
            if (station)
            {
                auto allTiles = station->GetAllTilesAtPosition(cursorPos);
                if (!allTiles.empty())
                {
                    DrawTileOutline(allTiles.at(allTiles.size() - 1), camera);
                }
            }
        }

        if (camera.IsUiClear())
        {
            DrawDragSelectBox(camera);
            DrawMainTooltip(crewList, camera, station, font);
            DrawFpsCounter(deltaTime, 12, DEFAULT_FONT_SIZE, font);
        }

        UiManager::GetInstance().Update();
        UiManager::GetInstance().Render();

        EndDrawing();

        if (WindowShouldClose())
            SetBit(state, false, GameState::RUNNING);
    }

    updateThread.join();

    CloseWindow();

    UnloadTexture(stationTileset);
    UnloadTexture(iconTileset);
    UnloadTexture(fireSpritesheet);
    UnloadFont(font);

    LogMessage(LogLevel::INFO, "Clean-up Complete");
    return 0;
}