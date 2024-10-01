#include "ui.hpp"
#include "update.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>

std::mutex updateMutex;
std::condition_variable fixedUpdateCondition;
bool isGameRunning = true;

void FixedUpdate(std::shared_ptr<Station> station, std::vector<Crew> &crewList, double &timeSinceFixedUpdate)
{
    double previousTime = GetTime();

    while (isGameRunning)
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
            UpdateTiles(station);

            timeSinceFixedUpdate -= FIXED_DELTA_TIME;
        }

        fixedUpdateCondition.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main()
{
    InitWindow(0, 0, "Celestium");

    ToggleFullscreen();
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    SetExitKey(0);

    Texture2D tileset = LoadTexture("../assets/tilesets/station.png");
    Font font = LoadFontEx("../assets/fonts/Inconsolata-Bold.ttf", DEFAULT_FONT_SIZE, NULL, 0);

    PlayerCam camera = PlayerCam();

    std::vector<Crew> crewList{
        Crew("ALICE", {-2, 2}, RED),
        Crew("BOB", {3, 2}, GREEN),
        Crew("CHARLIE", {-3, -3}, ORANGE)};

    std::shared_ptr<Station> station = CreateStation();

    LogMessage(LogLevel::INFO, "Initialization Complete");

    double timeSinceFixedUpdate = 0;
    // Start the update thread
    std::thread updateThread(FixedUpdate, station, std::ref(crewList), std::ref(timeSinceFixedUpdate));

    double deltaTime = 0;

    while (isGameRunning)
    {
        deltaTime = GetFrameTime();

        // Handle all real-time input and camera logic in the main thread
        HandleCameraMovement(camera);
        HandleCameraOverlays(camera);
        HandleCrewHover(crewList, camera);
        HandleCrewSelection(crewList, camera);
        AssignCrewTasks(crewList, camera);
        HandleMouseDragging(station, camera);
        MouseDeleteExistingConnection(station, camera);

        // Render logic
        BeginDrawing();
        ClearBackground(Color(31, 40, 45));

        DrawTileGrid(camera);
        DrawStation(station, tileset, camera);
        DrawCrew(timeSinceFixedUpdate, crewList, camera);
        DrawDragSelectBox(camera);
        DrawMainTooltip(crewList, camera, station, font);
        DrawFpsCounter(deltaTime, 12, DEFAULT_FONT_SIZE, font);
        DrawOverlay(camera, 12, DEFAULT_FONT_SIZE, font);

        EndDrawing();

        isGameRunning = !WindowShouldClose();
    }

    fixedUpdateCondition.notify_all();
    updateThread.join();

    CloseWindow();

    UnloadTexture(tileset);
    UnloadFont(font);

    LogMessage(LogLevel::INFO, "Clean-up Complete");
    return 0;
}
