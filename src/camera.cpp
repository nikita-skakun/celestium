#include "camera.hpp"

/**
 * Converts the current mouse position from screen coordinates to world coordinates.
 *
 * @return The world position of the mouse as a Vector2.
 */
Vector2 PlayerCam::GetWorldMousePos() const
{
    return ScreenToWorld(GetMousePosition(), *this);
}

/**
 * Handles camera movement and zoom based on user input.
 *
 * @param camera The PlayerCam object containing the camera's properties.
 */
void HandleCameraMovement(PlayerCam &camera)
{
    // Update zoom based on mouse wheel input, clamping to min and max values
    camera.zoom = std::clamp(camera.zoom + GetMouseWheelMove() * ZOOM_SPEED * camera.zoom, MIN_ZOOM, MAX_ZOOM);

    // Move camera position if the middle mouse button is pressed
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON))
    {
        camera.position -= (GetMouseDelta() / camera.zoom / TILE_SIZE);
    }
}

/**
 * Toggles camera overlays based on user key input.
 *
 * @param camera The PlayerCam object containing the camera's properties.
 */
void HandleCameraOverlays(PlayerCam &camera)
{
    // Toggle oxygen overlay when 'O' is pressed
    if (IsKeyPressed(KEY_O))
    {
        camera.overlay = (camera.overlay != PlayerCam::Overlay::OXYGEN)
                             ? PlayerCam::Overlay::OXYGEN
                             : PlayerCam::Overlay::NONE;
    }

    // Toggle wall overlay when 'W' is pressed
    if (IsKeyPressed(KEY_W))
    {
        camera.overlay = (camera.overlay != PlayerCam::Overlay::WALL)
                             ? PlayerCam::Overlay::WALL
                             : PlayerCam::Overlay::NONE;
    }
}

/**
 * Converts screen coordinates to world coordinates based on the camera's position and zoom level.
 *
 * @param screenPos The position in screen coordinates.
 * @param camera The PlayerCam object containing the camera's properties.
 * @return The corresponding position in world coordinates as a Vector2.
 */
Vector2 ScreenToWorld(const Vector2 &screenPos, const PlayerCam &camera)
{
    return (screenPos - (Vector2(GetScreenWidth(), GetScreenHeight()) / 2.f)) / camera.zoom / TILE_SIZE + camera.position;
}

/**
 * Converts screen coordinates to the corresponding tile position in the world.
 *
 * @param screenPos The position in screen coordinates.
 * @param camera The PlayerCam object containing the camera's properties.
 * @return The tile position in world coordinates as a Vector2, rounded down to the nearest tile.
 */
Vector2Int ScreenToTile(const Vector2 &screenPos, const PlayerCam &camera)
{
    Vector2 worldPos = ScreenToWorld(screenPos, camera);
    return ToVector2Int(worldPos);
}

/**
 * Converts world coordinates to screen coordinates based on the camera's position and zoom level.
 *
 * @param worldPos The position in world coordinates.
 * @param camera The PlayerCam object containing the camera's properties.
 * @return The position in screen coordinates as a Vector2.
 */
Vector2 WorldToScreen(const Vector2 &worldPos, const PlayerCam &camera)
{
    return (worldPos - camera.position) * TILE_SIZE * camera.zoom + (Vector2(GetScreenWidth(), GetScreenHeight()) / 2.f);
}
