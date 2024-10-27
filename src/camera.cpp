#include "camera.hpp"

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
 * Toggles camera state based on user key input.
 */
void PlayerCam::HandleStateInputs()
{
    if (IsKeyPressed(KEY_ESCAPE))
        ToggleUiState(UiState::ESC_MENU);

    if (uiState != UiState::NONE)
        return;

    if (IsKeyPressed(KEY_O))
        ToggleOverlay(Overlay::OXYGEN);

    if (IsKeyPressed(KEY_W))
        ToggleOverlay(Overlay::WALL);

    if (IsKeyPressed(KEY_P))
        ToggleOverlay(Overlay::POWER);

    if (IsKeyPressed(KEY_B))
        ToggleBuildGameState();
}

void PlayerCam::ToggleSelectedCrew(int crewIndex)
{
    const auto selectedCrewListIter = selectedCrewList.find(crewIndex);
    if (selectedCrewListIter == selectedCrewList.end())
        selectedCrewList.insert(crewIndex);
    else
        selectedCrewList.erase(selectedCrewListIter);
}
