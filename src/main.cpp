#include "ui.h"
#include "update.h"

int main()
{
    InitWindow(0, 0, "Celestium");

    ToggleFullscreen();
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

    Texture2D tileset = LoadTexture("../assets/tilesets/station.png");
    Font font = LoadFontEx("../assets/fonts/Inconsolata.ttf", DEFAULT_FONT_SIZE, NULL, 0);

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

        // Vector2 hoveredTileScreenPos = WorldToScreen(ScreenToTile(mousePos, cameraPos, zoom), cameraPos, zoom);
        // DrawRectangleLines(hoveredTileScreenPos.x, hoveredTileScreenPos.y, TILE_SIZE * zoom, TILE_SIZE * zoom, BLUE);

        DrawTileGrid(camera);

        if (station)
        {
            for (std::shared_ptr<Tile> tile : station->tiles)
            {
                Vector2 startScreenPos = WorldToScreen(ToVector2(tile->position), camera);
                Vector2 sizeScreenPos = Vector2(1.f, 1.f) * TILE_SIZE * camera.zoom;

                Rectangle destRect = Vector2ToRect(startScreenPos, startScreenPos + sizeScreenPos);
                Rectangle sourceRec = Rectangle(tile->spriteOffset.x, tile->spriteOffset.y, 1, 1) * TILE_SIZE;

                DrawTexturePro(tileset, sourceRec, destRect, Vector2(), 0, WHITE);

                if (auto decorativeComp = tile->GetComponent<DecorativeComponent>())
                {
                    for (const DecorativeTile &dTile : decorativeComp->GetDecorativeTiles())
                    {
                        Vector2 v_startScreenPos = WorldToScreen(ToVector2(tile->position + dTile.offset), camera);
                        Rectangle v_destRect = Vector2ToRect(v_startScreenPos, v_startScreenPos + sizeScreenPos);
                        Rectangle v_sourceRec = Rectangle(dTile.spriteOffset.x, dTile.spriteOffset.y, 1, 1) * TILE_SIZE;

                        DrawTexturePro(tileset, v_sourceRec, v_destRect, Vector2(), 0, WHITE);
                    }
                }

                if (camera.overlay == PlayerCam::Overlay::OXYGEN)
                {
                    if (auto oxygenComp = tile->GetComponent<OxygenComponent>())
                    {
                        Color color = Color(50, 150, 255, oxygenComp->GetOxygenLevel() / TILE_OXYGEN_MAX * 255 * .8f);
                        DrawRectangleV(startScreenPos, sizeScreenPos, color);
                    }
                }

                if (camera.overlay == PlayerCam::Overlay::WALL && tile->HasComponent<SolidComponent>())
                {
                    DrawRectangleV(startScreenPos, sizeScreenPos, Color(255, 0, 0, 64));
                }
            }
        }

        for (const Crew &crew : crewList)
        {
            if (crew.taskQueue.size() > 0 && crew.taskQueue[0]->GetType() == Task::Type::MOVE)
            {
                const std::shared_ptr<MoveTask> moveTask = std::dynamic_pointer_cast<MoveTask>(crew.taskQueue[0]);
                if (!moveTask->path.empty())
                    DrawPath(moveTask->path, crew.position, camera);
            }

            Vector2 crewScreenPos = WorldToScreen(crew.position + Vector2(.5f, .5f), camera);

            if (camera.selectedCrewList.contains(&crew - &crewList[0]))
                DrawCircleV(crewScreenPos, (CREW_RADIUS + OUTLINE_SIZE) * camera.zoom, OUTLINE_COLOR);

            DrawCircleV(crewScreenPos, CREW_RADIUS * camera.zoom, crew.isAlive ? crew.color : GRAY);
        }

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
