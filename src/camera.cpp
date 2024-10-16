#include "camera.hpp"

/**
 * Converts the current mouse position from screen coordinates to world coordinates.
 *
 * @return The world position of the mouse as a Vector2.
 */
Vector2 PlayerCam::GetWorldMousePos() const
{
    return ScreenToWorld(GetMousePosition());
}

/**
 * Converts screen coordinates to world coordinates based on the camera's position and zoom level.
 *
 * @param screenPos The position in screen coordinates.
 * @return The corresponding position in world coordinates as a Vector2.
 */
Vector2 PlayerCam::ScreenToWorld(const Vector2 &screenPos) const
{
    return (screenPos - (GetScreenSize() / 2.f)) / zoom / TILE_SIZE + position;
}

/**
 * Converts screen coordinates to the corresponding tile position in the world.
 *
 * @param screenPos The position in screen coordinates.
 * @return The tile position in world coordinates as a Vector2, rounded down to the nearest tile.
 */
Vector2Int PlayerCam::ScreenToTile(const Vector2 &screenPos) const
{
    return ToVector2Int(ScreenToWorld(screenPos));
}

/**
 * Converts world coordinates to screen coordinates based on the camera's position and zoom level.
 *
 * @param worldPos The position in world coordinates.
 * @return The position in screen coordinates as a Vector2.
 */
Vector2 PlayerCam::WorldToScreen(const Vector2 &worldPos) const
{
    return (worldPos - position) * TILE_SIZE * zoom + (GetScreenSize() / 2.f);
}

/**
 * Converts world coordinates to screen coordinates based on the camera's position and zoom level.
 *
 * @param worldPos The position in world coordinates.
 * @return The position in screen coordinates as a Vector2.
 */
Vector2 PlayerCam::WorldToScreen(const Vector2Int &worldPos) const
{
    return (ToVector2(worldPos) - position) * TILE_SIZE * zoom + (GetScreenSize() / 2.f);
}

/**
 * Handles camera movement and zoom based on user input.
 */
void PlayerCam::HandleMovement()
{
    if (uiState != UiState::NONE)
        return;

    // Update zoom based on mouse wheel input, clamping to min and max values
    zoom = std::clamp(zoom + GetMouseWheelMove() * ZOOM_SPEED * zoom, MIN_ZOOM, MAX_ZOOM);

    // Move camera position if the middle mouse button is pressed
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON))
    {
        position -= (GetMouseDelta() / zoom / TILE_SIZE);
    }
}

/**
 * Toggles camera overlays based on user key input.
 */
void PlayerCam::HandleOverlays()
{
    if (uiState != UiState::NONE)
        return;

    if (IsKeyPressed(KEY_O))
        ToggleOverlay(Overlay::OXYGEN);

    if (IsKeyPressed(KEY_W))
        ToggleOverlay(Overlay::WALL);

    if (IsKeyPressed(KEY_P))
        ToggleOverlay(Overlay::POWER);
}

void PlayerCam::HandleUiStates()
{
    if (IsKeyPressed(KEY_ESCAPE))
        uiState = uiState == UiState::NONE ? UiState::ESC_MENU : UiState::NONE;
}
