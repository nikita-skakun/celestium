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

    enum class UiGameState : u_int8_t
    {
        SIM_MODE,
        BUILD_MODE,
    };

private:
    Vector2 position;
    Vector2 dragStartPos;
    Vector2 dragEndPos;
    DragType dragType;
    std::unordered_set<int> selectedCrewList;
    int crewHoverIndex;
    float zoom;
    Overlay overlay;
    UiState uiState;
    UiGameState gameState;

    void HandleMovement();
    void HandleOverlays();
    void HandleUiStates();

public:
    PlayerCam()
        : position(Vector2(0, 0)), dragStartPos(Vector2(0, 0)), dragEndPos(Vector2(0, 0)), dragType(DragType::NONE),
          crewHoverIndex(-1), zoom(1.f), overlay(Overlay::NONE), uiState(UiState::NONE), gameState(UiGameState::SIM_MODE) {}

    // Utility function for Screen to World space transformations
    Vector2 GetWorldMousePos() const;
    Vector2 ScreenToWorld(const Vector2 &screenPos) const;
    Vector2Int ScreenToTile(const Vector2 &screenPos) const;
    Vector2 WorldToScreen(const Vector2 &worldPos) const;

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
    void ToggleSelectedCrew(int crewIndex)
    {
        const auto selectedCrewListIter = selectedCrewList.find(crewIndex);
        if (selectedCrewListIter == selectedCrewList.end())
            selectedCrewList.insert(crewIndex);
        else
            selectedCrewList.erase(selectedCrewListIter);
    }

    constexpr int GetCrewHoverIndex() const { return crewHoverIndex; }
    constexpr void SetCrewHoverIndex(int newIndex) { crewHoverIndex = newIndex; }

    constexpr float GetZoom() const { return zoom; }

    constexpr Overlay GetOverlay() const { return overlay; }
    constexpr void ToggleOverlay(Overlay targetOverlay) { overlay = (overlay != targetOverlay) ? targetOverlay : Overlay::NONE; }
    constexpr std::string GetOverlayName() const { return EnumToName<Overlay>(overlay); }

    constexpr UiState GetUiState() const { return uiState; }
    constexpr void SetUiState(UiState newUiState) { uiState = newUiState; }
    constexpr bool IsUiClear() const { return uiState == UiState::NONE; }

    constexpr UiGameState GetUiGameState() const { return gameState; }
    constexpr void SetUiGameState(UiGameState newGameState) { gameState = newGameState; }
    constexpr void ToggleUiGameState(UiGameState targetState) { gameState = (gameState != targetState) ? targetState : UiGameState::SIM_MODE; }

    constexpr void HandleCamera()
    {
        HandleMovement();
        HandleOverlays();
        HandleUiStates();
    }
};
