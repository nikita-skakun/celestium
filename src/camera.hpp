#pragma once
#include "utils.hpp"
#include <unordered_set>

struct PlayerCam
{
    enum class Overlay : u_int8_t
    {
        NONE,
        OXYGEN,
        WALL,
        POWER,
    };

    enum class DragType : u_int8_t
    {
        NONE,
        SELECT,
        POWER_CONNECT,
    };

    enum class UiState : u_int8_t
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
    std::unordered_set<int> selectedCrewList;
    int crewHoverIndex = -1;
    float zoom = 1.f;
    Overlay overlay = Overlay::NONE;
    UiState uiState = UiState::NONE;

public:
    PlayerCam() {}

    constexpr const Vector2 &GetPosition() const { return position; }

    constexpr const Vector2 &GetDragStart() const { return dragStartPos; }
    constexpr const Vector2 &GetDragEnd() const { return dragEndPos; }
    constexpr void SetDragStart(const Vector2 dragStart) { dragStartPos = dragStart; }
    constexpr void SetDragEnd(const Vector2 dragEnd) { dragEndPos = dragEnd; }

    constexpr DragType GetDragType() const { return dragType; }
    constexpr void SetDragType(DragType newDragType) { dragType = newDragType; }
    constexpr bool IsDragging() const { return dragType != PlayerCam::DragType::NONE; }

    constexpr const std::unordered_set<int> &GetSelectedCrew() const { return selectedCrewList; }
    constexpr void ClearSelectedCrew() { selectedCrewList.clear(); }
    constexpr void AddSelectedCrew(int crewIndex) { selectedCrewList.insert(crewIndex); }
    void ToggleSelectedCrew(int crewIndex);

    constexpr int GetCrewHoverIndex() const { return crewHoverIndex; }
    constexpr void SetCrewHoverIndex(int newIndex) { crewHoverIndex = newIndex; }

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

    void HandleMovement();
};
