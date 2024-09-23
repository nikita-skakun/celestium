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
 * Displays the current FPS and the time taken for the last frame in milliseconds.
 *
 * @param textSize  The size of the text to be drawn.
 * @param padding   The padding from the screen edges for positioning the text.
 * @param deltaTime The time taken for the last frame, used to display in milliseconds.
 */
void DrawFpsCounter(int textSize, int padding, float deltaTime)
{
    std::string fpsText = "FPS: " + std::to_string(GetFPS()) + " (" + fmt::format("{:.2f}", deltaTime * 1000.f) + "ms)";
    const char *text = fpsText.c_str();
    DrawText(text, GetScreenWidth() - MeasureText(text, textSize) - padding, padding, textSize, BLACK);
}

/**
 * Draws a tooltip with a background rectangle at the specified position.
 *
 * @param tooltip  The text to display in the tooltip.
 * @param pos      The position where the tooltip will be drawn.
 * @param fontSize The size of the text in the tooltip.
 * @param padding  The padding around the text within the tooltip background.
 */
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, int fontSize, float padding)
{
    int lineCount = 0;
    const char **lines = TextSplit(tooltip.c_str(), '\n', &lineCount);

    int textWidth = 0;
    for (int i = 0; i < lineCount; i++)
    {
        textWidth = std::max(textWidth, MeasureText(lines[i], fontSize));
    }

    Rectangle backgroundRect = {
        pos.x,
        pos.y,
        textWidth + 2.f * padding,
        lineCount * fontSize + 2.f * padding};

    DrawRectangleRec(backgroundRect, Fade(LIGHTGRAY, 0.7f));

    for (int i = 0; i < lineCount; i++)
    {
        DrawText(lines[i], pos.x + padding, pos.y + padding + (i * fontSize), fontSize, BLACK);
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
