#include "ui.h"
#include "update.h"

int main()
{
    InitWindow(0, 0, "Celestium");

    ToggleFullscreen();
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

    Texture2D tileset = LoadTexture("../assets/tilesets/station.png");
    Font font = LoadFontEx("../assets/fonts/Inconsolata-Bold.ttf", DEFAULT_FONT_SIZE, NULL, 0);

    PlayerCam camera = PlayerCam();

    std::vector<Crew> crewList{
        Crew("ALICE", {0, 0}, RED),
        Crew("BOB", {3, 2}, GREEN),
        Crew("CHARLIE", {-3, -3}, ORANGE)};

    std::shared_ptr<Station> station = CreateStation();

    LogMessage(LogLevel::INFO, "Initialization Complete");

    float previousTime = GetTime();

    while (!WindowShouldClose())
    {
        float currentTime = GetTime();
        float deltaTime = currentTime - previousTime;
        previousTime = currentTime;

        HandleCameraMovement(camera);
        HandleCameraOverlays(camera);
        HandleCrewHover(crewList, camera);
        HandleCrewSelection(crewList, camera);
        AssignCrewTasks(crewList, camera);
        HandleCrewTasks(deltaTime, crewList);
        HandleCrewEnvironment(deltaTime, crewList);
        UpdateCrewCurrentTile(crewList, station);
        UpdateTiles(deltaTime, station);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawTileGrid(camera);
        DrawStation(station, tileset, camera);
        DrawCrew(crewList, camera);
        DrawDragSelectBox(camera);
        DrawMainTooltip(crewList, camera, station, font);
        DrawFpsCounter(deltaTime, 6, DEFAULT_FONT_SIZE, font);

        EndDrawing();
    }

    CloseWindow();

    UnloadTexture(tileset);
    UnloadFont(font);

    LogMessage(LogLevel::INFO, "Clean-up Complete");
    return 0;
}
