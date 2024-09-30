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

    enum DragType : u_int8_t
    {
        SELECT,
        POWER_CONNECT,
    };

    Vector2 position;
    bool isDragging;
    Vector2 dragStartPos;
    Vector2 dragEndPos;
    DragType dragType;
    std::unordered_set<int> selectedCrewList;
    int crewHoverIndex;
    float zoom;
    Overlay overlay;

    PlayerCam() : isDragging(false), dragType(DragType::SELECT), crewHoverIndex(-1), zoom(1.f), overlay(Overlay::NONE) {}


    // Utility function for Screen to World space transformations
    Vector2 GetWorldMousePos() const;
    Vector2 ScreenToWorld(const Vector2 &screenPos) const;
    Vector2Int ScreenToTile(const Vector2 &screenPos) const;
    Vector2 WorldToScreen(const Vector2 &worldPos) const;
};

void HandleCameraMovement(PlayerCam &camera);
void HandleCameraOverlays(PlayerCam &camera);