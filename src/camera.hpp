#pragma once
#include "utils.hpp"

struct PlayerCam
{
    enum class Overlay : uint8_t
    {
        NONE,
        OXYGEN,
        WALL,
        POWER,
    };

    enum class DragType : uint8_t
    {
        NONE,
        SELECT,
        POWER_CONNECT,
    };

    enum class UiState : uint8_t
    {
        NONE,
        ESC_MENU,
        SETTINGS_MENU,
    };

private:
    Vector2 position = Vector2(0, 0);
    Vector2 dragStartPos = Vector2(0, 0);
    Vector2 dragEndPos = Vector2(0, 0);
    DragType dragType = DragType::NONE;
    float zoom = 1.f;
    Overlay overlay = Overlay::NONE;
    UiState uiState = UiState::NONE;
    uint16_t fpsIndex;

public:
    PlayerCam() {}

    constexpr const Vector2 &GetPosition() const { return position; }

    constexpr const Vector2 &GetDragStart() const { return dragStartPos; }
    constexpr const Vector2 &GetDragEnd() const { return dragEndPos; }
    constexpr Rectangle GetDragRect() const { return Vector2ToBoundingBox(dragStartPos, dragEndPos); }
    constexpr void SetDragStart(const Vector2 dragStart) { dragStartPos = dragStart; }
    constexpr void SetDragEnd(const Vector2 dragEnd) { dragEndPos = dragEnd; }

    constexpr DragType GetDragType() const { return dragType; }
    constexpr void SetDragType(DragType newDragType) { dragType = newDragType; }
    constexpr bool IsDragging() const { return dragType != PlayerCam::DragType::NONE; }

    constexpr float GetZoom() const { return zoom; }

    constexpr Overlay GetOverlay() const { return overlay; }
    constexpr bool IsOverlay(Overlay other) const { return overlay == other; }
    constexpr void SetOverlay(Overlay targetOverlay) { overlay = targetOverlay; }
    constexpr void ToggleOverlay(Overlay targetOverlay) { overlay = (overlay != targetOverlay) ? targetOverlay : Overlay::NONE; }
    constexpr std::string GetOverlayName() const { return EnumToName<Overlay>(overlay); }

    constexpr UiState GetUiState() const { return uiState; }
    constexpr void SetUiState(UiState newUiState) { uiState = newUiState; }
    constexpr void ToggleUiState(UiState targetUiState) { uiState = (uiState != targetUiState) ? targetUiState : UiState::NONE; }
    constexpr bool IsUiState(UiState other) const { return uiState == other; }
    constexpr bool IsUiClear() const { return uiState == UiState::NONE; }

    constexpr uint16_t GetFpsIndex() const { return fpsIndex; }
    constexpr void SetFpsIndex(uint16_t newFpsIndex)
    {
        fpsIndex = std::min(newFpsIndex, (uint16_t)(FPS_OPTIONS.size() - 1));
        SetTargetFPS(FPS_OPTIONS.at(fpsIndex));
    }
    constexpr void SetFps(uint16_t newFps) { SetFpsIndex(std::ranges::find(FPS_OPTIONS, newFps) - FPS_OPTIONS.begin()); }
    
    std::string GetFpsOptions() const;

    void HandleMovement();
};
