#include "camera.hpp"

std::string PlayerCam::GetFpsOptions() const
{
    unsigned int monitorFps = GetMonitorRefreshRate(GetCurrentMonitor());
    std::string availableFpsOptions;
    for (unsigned int fpsOption : FPS_OPTIONS)
    {
        if (fpsOption > monitorFps)
        {
            if (!availableFpsOptions.empty())
                availableFpsOptions += ";";
            availableFpsOptions.append(std::to_string(monitorFps));
            break;
        }

        if (!availableFpsOptions.empty())
            availableFpsOptions += ";";
        availableFpsOptions.append(std::to_string(fpsOption));

        if (fpsOption == monitorFps)
            break;
    }

    return availableFpsOptions;
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

    // Keyboard arrow movement
    Vector2 keyMove{0, 0};
    if (IsKeyDown(KEY_UP))
        keyMove.y -= 1.f;
    if (IsKeyDown(KEY_DOWN))
        keyMove.y += 1.f;
    if (IsKeyDown(KEY_RIGHT))
        keyMove.x += 1.f;
    if (IsKeyDown(KEY_LEFT))
        keyMove.x -= 1.f;

    if (keyMove.x != 0.f || keyMove.y != 0.f)
        position += Vector2Normalize(keyMove) * CAMERA_KEY_MOVE_SPEED * GetFrameTime() / zoom;

    // Move camera position if the middle mouse button is pressed
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON))
        position -= (GetMouseDelta() / zoom / TILE_SIZE);
}
