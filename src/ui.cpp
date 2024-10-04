#include "ui.hpp"

/**
 * Draws a grid of tiles on the screen based on the current camera position and zoom level.
 *
 * @param camera The current player camera, used to determine the position and zoom level.
 */
void DrawTileGrid(const PlayerCam &camera)
{
    float screenWidth = GetScreenWidth();
    float screenHeight = GetScreenHeight();

    // Calculate the top-left corner in world coordinates
    Vector2 topLeft = camera.position * TILE_SIZE - Vector2(screenWidth, screenHeight) / 2.0f / camera.zoom;

    // Draw vertical lines
    for (float x = floor(topLeft.x / TILE_SIZE) * TILE_SIZE; x <= ceil((topLeft.x + screenWidth / camera.zoom) / TILE_SIZE) * TILE_SIZE; x += TILE_SIZE)
    {
        float screenX = (x - camera.position.x * TILE_SIZE) * camera.zoom + screenWidth / 2.0f;
        DrawLineV({screenX, 0}, {screenX, (float)screenHeight}, GRID_COLOR);
    }

    // Draw horizontal lines
    for (float y = floor(topLeft.y / TILE_SIZE) * TILE_SIZE; y <= ceil((topLeft.y + screenHeight / camera.zoom) / TILE_SIZE) * TILE_SIZE; y += TILE_SIZE)
    {
        float screenY = (y - camera.position.y * TILE_SIZE) * camera.zoom + screenHeight / 2.0f;
        DrawLineV({0, screenY}, {(float)screenWidth, screenY}, GRID_COLOR);
    }
}

/**
 * Draws a path as a series of lines between waypoints.
 *
 * @param path     A queue of Vector2Int positions representing the path to draw.
 * @param startPos The starting position of the path.
 * @param camera   The PlayerCam used for converting world coordinates to screen coordinates.
 */
void DrawPath(const std::deque<Vector2Int> &path, const Vector2 &startPos, const PlayerCam &camera)
{
    if (path.empty())
        return;

    Vector2 a = startPos + Vector2(.5f, .5f);

    for (const auto &point : path)
    {
        Vector2 b = ToVector2(point) + Vector2(.5f, .5f);
        DrawLineV(camera.WorldToScreen(a), camera.WorldToScreen(b), Fade(GREEN, .5f));
        a = b;
    }
}

/**
 * Draws the station tiles and direct overlays.
 *
 * @param station The station to draw the tiles of.
 * @param tileset A texture containing all tile assets.
 * @param camera  The PlayerCam used for handling overlays and converting coordinates.
 */
void DrawStationTiles(std::shared_ptr<Station> station, const Texture2D &tileset, const PlayerCam &camera)
{
    if (!station)
        return;

    Vector2 sizeScreenPos = Vector2(1.f, 1.f) * TILE_SIZE * camera.zoom;

    for (std::shared_ptr<Tile> tile : station->tiles)
    {
        Vector2 startScreenPos = camera.WorldToScreen(ToVector2(tile->GetPosition()));

        Rectangle destRect = Vector2ToRect(startScreenPos, startScreenPos + sizeScreenPos);
        Rectangle sourceRec = Rectangle(tile->GetSpriteOffset().x, tile->GetSpriteOffset().y, 1, 1) * TILE_SIZE;

        DrawTexturePro(tileset, sourceRec, destRect, Vector2(), 0, WHITE);

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

/**
 * Draws the indirect visual overlays.
 *
 * @param station The station to draw the overlays of.
 * @param tileset A texture containing all tile assets.
 * @param camera  The PlayerCam used for handling overlays and converting coordinates.
 */
void DrawStationOverlays(std::shared_ptr<Station> station, const Texture2D &tileset, const PlayerCam &camera)
{
    Vector2 sizeScreenPos = Vector2(1.f, 1.f) * TILE_SIZE * camera.zoom;

    for (std::shared_ptr<Tile> tile : station->tiles)
    {
        if (auto decorative = tile->GetComponent<DecorativeComponent>())
        {
            for (const DecorativeTile &dTile : decorative->GetDecorativeTiles())
            {
                Rectangle sourceRec = Rectangle(dTile.spriteOffset.x, dTile.spriteOffset.y, 1, 1) * TILE_SIZE;
                Vector2 startPos = camera.WorldToScreen(ToVector2(tile->GetPosition() + dTile.offset));
                Rectangle destRect = Rectangle(startPos.x, startPos.y, sizeScreenPos.x, sizeScreenPos.y);

                DrawTexturePro(tileset, sourceRec, destRect, Vector2(), 0, WHITE);
            }
        }

        if (camera.overlay == PlayerCam::Overlay::POWER)
        {
            if (auto powerConsumer = tile->GetComponent<PowerConsumerComponent>())
            {
                if (!powerConsumer->IsActive())
                {
                    Vector2 startScreenPos = camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(.66f, 0.f));

                    Rectangle destRect = Vector2ToRect(startScreenPos, startScreenPos + sizeScreenPos / 3.f);
                    Rectangle sourceRec = Rectangle(0, 7, 1, 1) * TILE_SIZE;

                    DrawTexturePro(tileset, sourceRec, destRect, Vector2(), 0, Fade(YELLOW, .8f));
                }
            }

            if (auto battery = tile->GetComponent<BatteryComponent>())
            {
                float barProgress = battery->GetChargeLevel() / battery->GetMaxChargeLevel();
                Vector2 topLeftPos = camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(1.f / 16.f, 0.f));
                Vector2 barStartPos = camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(1.f / 16.f, 1.f - barProgress));

                Vector2 totalSize = Vector2(1.f / 8.f, 1.f) * TILE_SIZE * camera.zoom;
                Vector2 barSize = Vector2(1.f / 8.f, barProgress) * TILE_SIZE * camera.zoom;

                DrawRectangleV(topLeftPos, totalSize, Color(25, 25, 25, 200));
                DrawRectangleV(barStartPos, barSize, Fade(YELLOW, .8f));
            }

            if (auto powerConnector = tile->GetComponent<PowerConnectorComponent>())
            {
                auto connections = powerConnector->GetConnections();
                for (auto &&connection : connections)
                {
                    if (auto connectionTile = connection->_parent.lock())
                    {
                        DrawLineEx(camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(.5f, .5f)),
                                   camera.WorldToScreen(ToVector2(connectionTile->GetPosition()) + Vector2(.5f, .5f)),
                                   POWER_CONNECTION_WIDTH * std::max(camera.zoom, 1.f), POWER_CONNECTION_COLOR);
                    }
                }
            }
        }
    }
}

/**
 * Draws the crew members on the screen, accounting for their movement.
 *
 * @param timeSinceFixedUpdate The elapsed time since the last fixed update.
 * @param crewList             List of crew members to be drawn.
 * @param camera               The PlayerCam used for handling hover state and converting coordinates.
 */
void DrawCrew(double timeSinceFixedUpdate, const std::vector<Crew> &crewList, const PlayerCam &camera)
{
    for (const Crew &crew : crewList)
    {
        Vector2 drawPosition = crew.position;

        if (!crew.taskQueue.empty() && crew.taskQueue.front()->GetType() == Task::Type::MOVE)
        {
            const std::shared_ptr<MoveTask> moveTask = std::dynamic_pointer_cast<MoveTask>(crew.taskQueue.front());

            if (!moveTask->path.empty())
            {
                DrawPath(moveTask->path, crew.position, camera);
                Vector2 nextPosition = ToVector2(moveTask->path.front());

                const float moveDelta = timeSinceFixedUpdate * CREW_MOVE_SPEED;
                const float distanceLeftSq = Vector2DistanceSq(crew.position, nextPosition) - moveDelta * moveDelta;
                if (distanceLeftSq <= 0)
                {
                    drawPosition = nextPosition;

                    if (moveTask->path.size() > 1)
                    {
                        Vector2 futurePosition = ToVector2(moveTask->path.at(1));
                        drawPosition += Vector2Normalize(futurePosition - drawPosition) * sqrtf(-distanceLeftSq);
                    }
                }
                else
                    drawPosition += Vector2Normalize(nextPosition - crew.position) * moveDelta;
            }
        }

        Vector2 crewScreenPos = camera.WorldToScreen(drawPosition + Vector2(.5f, .5f));

        if (camera.selectedCrewList.contains(&crew - &crewList[0]))
            DrawCircleV(crewScreenPos, (CREW_RADIUS + OUTLINE_SIZE) * camera.zoom, OUTLINE_COLOR);

        DrawCircleV(crewScreenPos, CREW_RADIUS * camera.zoom, crew.isAlive ? crew.color : GRAY);
    }
}

/**
 * Draws a power connect line or a rectangle selection box based on mouse drag.
 *
 * @param camera   The PlayerCam that stores the mouse drag information.
 */
void DrawDragSelectBox(const PlayerCam &camera)
{
    if (!camera.isDragging)
        return;

    switch (camera.dragType)
    {
    case PlayerCam::DragType::SELECT:
        DrawRectangleLinesEx(Vector2ToRect(camera.WorldToScreen(camera.dragStartPos), camera.WorldToScreen(camera.dragEndPos)), 1.f, BLUE);
        break;

    case PlayerCam::DragType::POWER_CONNECT:
        DrawLineEx(camera.WorldToScreen(camera.dragStartPos), camera.WorldToScreen(camera.dragEndPos),
                   POWER_CONNECTION_WIDTH * std::max(camera.zoom, 1.f), POWER_CONNECTION_COLOR);
        break;

    default:
        break;
    }
}

/**
 * Displays the current FPS in the top-right corner.
 *
 * @param deltaTime The time taken for the last frame, used to display in milliseconds.
 * @param padding   The padding from the screen edges for positioning the text.
 * @param fontSize  The size of the text to be drawn.
 * @param font      The font to use when drawing the FPS counter, defaults to RayLib's default.
 */
void DrawFpsCounter(float deltaTime, int padding, int fontSize, const Font &font)
{
    std::string fpsText = std::format("FPS: {:} ({:.2f}ms)", GetFPS(), deltaTime * 1000.f);
    const char *text = fpsText.c_str();
    DrawTextEx(font, text, Vector2(GetScreenWidth() - MeasureTextEx(font, text, fontSize, 1).x - padding, padding), fontSize, 1, UI_TEXT_COLOR);
}

/**
 * Displays the current overlay in the top-left corner.
 *
 * @param camera    The PlayerCam that stores the overlay information.
 * @param padding   The padding from the screen edges for positioning the text.
 * @param fontSize  The size of the text to be drawn.
 * @param font      The font to use when drawing the FPS counter, defaults to RayLib's default.
 */
void DrawOverlay(const PlayerCam &camera, int padding, int fontSize, const Font &font)
{
    std::string overlayText = std::format("Overlay: {:}", StringToTitleCase(std::string(magic_enum::enum_name(camera.overlay))));
    const char *text = overlayText.c_str();
    DrawTextEx(font, text, Vector2(padding, padding), fontSize, 1, UI_TEXT_COLOR);
}

/**
 * Draws a tooltip with a background rectangle at the specified position.
 *
 * @param tooltip  The text to display in the tooltip.
 * @param pos      The position where the tooltip will be drawn.
 * @param font     The font to use when drawing the tooltip, defaults to RayLib's default.
 * @param padding  The padding around the text within the tooltip background.
 * @param fontSize The size of the text in the tooltip.
 */
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, const Font &font, float padding, int fontSize)
{
    int lineCount = 0;
    const char **lines = TextSplit(tooltip.c_str(), '\n', &lineCount);

    float textWidth = 0;
    for (int i = 0; i < lineCount; i++)
    {
        textWidth = std::max(textWidth, MeasureTextEx(font, lines[i], fontSize, 1).x);
    }

    Vector2 size = {textWidth + 2.f * padding, lineCount * fontSize + 2.f * padding};
    Vector2 tooltipPos = pos;

    // Check if the tooltip goes beyond the screen's right edge (considering padding)
    if (tooltipPos.x + size.x > GetScreenWidth())
    {
        // Slide the tooltip back to fit within the screen's right edge
        tooltipPos.x = GetScreenWidth() - size.x;
    }

    // Ensure the tooltip doesn't go beyond the left edge of the screen
    if (tooltipPos.x < 0)
    {
        tooltipPos.x = 0;
    }

    // Ensure the tooltip doesn't go beyond the bottom or top of the screen
    if (tooltipPos.y + size.y > GetScreenHeight())
    {
        tooltipPos.y = GetScreenHeight() - size.y;
    }
    else if (tooltipPos.y < 0)
    {
        tooltipPos.y = 0;
    }

    Rectangle backgroundRect = Vector2ToRect(tooltipPos, tooltipPos + size);
    DrawRectangleRec(backgroundRect, Fade(LIGHTGRAY, 0.7f));

    // Draw the tooltip with the calculated position and padding
    for (int i = 0; i < lineCount; i++)
    {
        DrawTextEx(font, lines[i], tooltipPos + Vector2(padding, padding + (i * fontSize)), fontSize, 1, BLACK);
    }
}

/**
 * Draws a tooltip about the crew member or station tile under the mouse cursor.
 *
 * @param crewList   A vector of Crew objects, used to retrieve the hovered crew member's information.
 * @param camera     The PlayerCam used for handling hover state and converting coordinates.
 * @param station    A shared pointer to the Station, used to fetch tiles and their components.
 * @param font       The font to use when drawing the tooltip, defaults to RayLib's default.
 */
void DrawMainTooltip(const std::vector<Crew> &crewList, const PlayerCam &camera, std::shared_ptr<Station> station, const Font &font)
{
    std::string hoverText;
    const Vector2 mousePos = GetMousePosition();

    // Add crew info we are hovering over
    if (camera.crewHoverIndex >= 0)
    {
        const Crew &crew = crewList[camera.crewHoverIndex];
        hoverText += "Name: " + crew.name;
        if (crew.isAlive)
        {
            hoverText += std::format("\nOxygen: {:.2f}", crew.oxygen);
        }
        else
        {
            hoverText += "\nDEAD";
        }
    }

    // Add tile info we are hovering over
    if (station)
    {
        Vector2Int tileHoverPos = camera.ScreenToTile(mousePos);
        std::vector<std::shared_ptr<Tile>> tiles = station->GetTilesAtPosition(tileHoverPos);

        for (const std::shared_ptr<Tile> &tile : tiles)
        {
            if (!hoverText.empty())
                hoverText += "\n";
            hoverText += " - " + tile->GetName();

            if (auto oxygenComp = tile->GetComponent<OxygenComponent>())
            {
                hoverText += std::format("\n   + Tile Ox: {:.2f}", oxygenComp->GetOxygenLevel());
            }
            if (auto batteryComp = tile->GetComponent<BatteryComponent>())
            {
                hoverText += std::format("\n   + Energy: {:.2f}", batteryComp->GetChargeLevel());
            }
            if (auto powerConnector = tile->GetComponent<PowerConnectorComponent>())
            {
                hoverText += std::format("\n   + Power Connector: {}", magic_enum::enum_flags_name(powerConnector->GetIO()));
            }
        }
    }

    if (hoverText.empty())
        return;

    DrawTooltip(hoverText, mousePos, font);
}