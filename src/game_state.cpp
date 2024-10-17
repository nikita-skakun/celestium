#include "game_state.hpp"

/**
 * Converts the current mouse position from screen coordinates to world coordinates.
 *
 * @return The world position of the mouse as a Vector2.
 */
Vector2 GameManager::GetWorldMousePos()
{
    return ScreenToWorld(GetMousePosition());
}

/**
 * Converts screen coordinates to world coordinates based on the camera's position and zoom level.
 *
 * @param screenPos The position in screen coordinates.
 * @return The corresponding position in world coordinates as a Vector2.
 */
Vector2 GameManager::ScreenToWorld(const Vector2 &screenPos)
{
    auto &camera = GetInstance().camera;
    return (screenPos - (GetScreenSize() / 2.)) / camera.GetZoom() / TILE_SIZE + camera.GetPosition();
}

/**
 * Converts screen coordinates to the corresponding tile position in the world.
 *
 * @param screenPos The position in screen coordinates.
 * @return The tile position in world coordinates as a Vector2, rounded down to the nearest tile.
 */
Vector2Int GameManager::ScreenToTile(const Vector2 &screenPos)
{
    return ToVector2Int(ScreenToWorld(screenPos));
}

/**
 * Converts world coordinates to screen coordinates based on the camera's position and zoom level.
 *
 * @param worldPos The position in world coordinates.
 * @return The position in screen coordinates as a Vector2.
 */
Vector2 GameManager::WorldToScreen(const Vector2 &worldPos)
{
    auto &camera = GetInstance().camera;
    return (worldPos - camera.GetPosition()) * TILE_SIZE * camera.GetZoom() + (GetScreenSize() / 2.);
}

/**
 * Converts world coordinates to screen coordinates based on the camera's position and zoom level.
 *
 * @param worldPos The position in world coordinates.
 * @return The position in screen coordinates as a Vector2.
 */
Vector2 GameManager::WorldToScreen(const Vector2Int &worldPos)
{
    auto &camera = GetInstance().camera;
    return (ToVector2(worldPos) - camera.GetPosition()) * TILE_SIZE * camera.GetZoom() + (GetScreenSize() / 2.);
}