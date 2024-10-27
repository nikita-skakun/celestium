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

void PlayerCam::ToggleSelectedCrew(int crewIndex)
{
    const auto selectedCrewListIter = selectedCrewList.find(crewIndex);
    if (selectedCrewListIter == selectedCrewList.end())
        selectedCrewList.insert(crewIndex);
    else
        selectedCrewList.erase(selectedCrewListIter);
}
