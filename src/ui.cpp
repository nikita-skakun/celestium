#include "ui.h"

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
        DrawLineV({screenX, 0}, {screenX, (float)screenHeight}, LIGHTGRAY);
    }

    // Draw horizontal lines
    for (float y = floor(topLeft.y / TILE_SIZE) * TILE_SIZE; y <= ceil((topLeft.y + screenHeight / camera.zoom) / TILE_SIZE) * TILE_SIZE; y += TILE_SIZE)
    {
        float screenY = (y - camera.position.y * TILE_SIZE) * camera.zoom + screenHeight / 2.0f;
        DrawLineV({0, screenY}, {(float)screenWidth, screenY}, LIGHTGRAY);
    }
}

/**
 * Draws a path as a series of lines between waypoints.
 *
 * @param path     A queue of Vector2Int positions representing the path to draw.
 * @param startPos The starting position of the path.
 * @param camera   The PlayerCam used for converting world coordinates to screen coordinates.
 */
void DrawPath(const std::queue<Vector2Int> &path, const Vector2 &startPos, const PlayerCam &camera)
{
    if (path.empty())
        return;

    std::queue<Vector2Int> p = path;

    Vector2 a = startPos + Vector2(.5f, .5f);
    while (!p.empty())
    {
        Vector2 b = ToVector2(p.front()) + Vector2(.5f, .5f);
        p.pop();

        DrawLineV(WorldToScreen(a, camera), WorldToScreen(b, camera), Fade(GREEN, .5f));
        a = b;
    }
}

/**
 * Draws a rectangle selection box on the screen when the user is dragging the mouse.
 *
 * @param camera   The PlayerCam that stores the mouse drag information.
 */
void DrawDragSelectBox(const PlayerCam &camera)
{
    if (camera.isDragging)
    {
        Rectangle selectBox = Vector2ToRect(WorldToScreen(camera.dragStartPos, camera), WorldToScreen(camera.dragEndPos, camera));
        DrawRectangleLines(selectBox.x, selectBox.y, selectBox.width, selectBox.height, BLUE);
    }
}

/**
 * Displays the current FPS and the time taken for the last frame in milliseconds.
 *
 * @param deltaTime The time taken for the last frame, used to display in milliseconds.
 * @param padding   The padding from the screen edges for positioning the text.
 * @param fontSize  The size of the text to be drawn.
 * @param font      The font to use when drawing the FPS counter, defaults to RayLib's default.
 */
void DrawFpsCounter(float deltaTime, int padding, int fontSize, const Font &font)
{
    std::string fpsText = fmt::format("FPS: {:} ({:.2f}ms)", GetFPS(), deltaTime * 1000.f);
    const char *text = fpsText.c_str();
    DrawTextEx(font, text, Vector2(GetScreenWidth() - MeasureTextEx(font, text, fontSize, 1).x - padding, padding), fontSize, 1, BLACK);
}

/**
 * Draws a tooltip with a background rectangle at the specified position.
 *
 * @param tooltip  The text to display in the tooltip.
 * @param pos      The position where the tooltip will be drawn.
 * @param padding  The padding around the text within the tooltip background.
 * @param fontSize The size of the text in the tooltip.
 * @param font     The font to use when drawing the tooltip, defaults to RayLib's default.
 */
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, float padding, int fontSize, const Font &font)
{
    int lineCount = 0;
    const char **lines = TextSplit(tooltip.c_str(), '\n', &lineCount);

    float textWidth = 0;
    for (int i = 0; i < lineCount; i++)
    {
        textWidth = std::max(textWidth, MeasureTextEx(font, lines[i], fontSize, 1).x);
    }

    Rectangle backgroundRect = {
        pos.x,
        pos.y,
        textWidth + 2.f * padding,
        lineCount * fontSize + 2.f * padding};

    DrawRectangleRec(backgroundRect, Fade(LIGHTGRAY, 0.7f));

    for (int i = 0; i < lineCount; i++)
    {
        DrawTextEx(font, lines[i], pos + Vector2(padding, padding + (i * fontSize)), fontSize, 1, BLACK);
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
    const float padding = 10.0f;
    const Vector2 mousePos = GetMousePosition();

    // Check if we're hovering over a crew member
    if (camera.crewHoverIndex >= 0)
    {
        const Crew &crew = crewList[camera.crewHoverIndex];
        hoverText += "Name: " + crew.name;
        if (crew.isAlive)
        {
            hoverText += fmt::format("\nOxygen: {:.2f}", crew.oxygen);
        }
        else
        {
            hoverText += "\nDEAD";
        }
    }

    // Check if we're hovering over a station tile
    if (station)
    {
        Vector2Int tileHoverPos = ScreenToTile(mousePos, camera);
        std::vector<std::shared_ptr<Tile>> tiles = station->GetTilesAtPosition(tileHoverPos);

        for (const std::shared_ptr<Tile> &tile : tiles)
        {
            if (!hoverText.empty())
                hoverText += "\n";
            hoverText += " - " + tile->GetName();

            if (auto oxygenComp = tile->GetComponent<OxygenComponent>())
            {
                hoverText += fmt::format("\n   + Tile Ox: {:.2f}", oxygenComp->GetOxygenLevel());
            }
            if (auto batteryComp = tile->GetComponent<BatteryComponent>())
            {
                hoverText += fmt::format("\n   + Energy: {:.2f}", batteryComp->GetChargeLevel());
            }
        }
    }

    // If there is text to display in the tooltip
    if (!hoverText.empty())
    {
        int lineCount = 0;
        const char **lines = TextSplit(hoverText.c_str(), '\n', &lineCount);

        float textWidth = 0;
        for (int i = 0; i < lineCount; i++)
        {
            textWidth = std::max(textWidth, MeasureTextEx(font, lines[i], DEFAULT_FONT_SIZE, 1).x);
        }

        Vector2 size = {textWidth + 2.f * padding, lineCount * DEFAULT_FONT_SIZE + 2.f * padding};
        Vector2 tooltipPos = mousePos;

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
            DrawTextEx(font, lines[i], tooltipPos + Vector2(padding, padding + (i * DEFAULT_FONT_SIZE)), DEFAULT_FONT_SIZE, 1, BLACK);
        }
    }
}