#pragma once
#include "utils.h"
#include <unordered_set>

struct PlayerCam
{
    enum Overlay : u_int8_t
    {
        NONE,
        OXYGEN,
        WALL,
    };

    Vector2 position;
    Vector2 dragStartPos;
    Vector2 dragEndPos;
    std::unordered_set<int> selectedCrewList;
    int crewHoverIndex;
    float zoom;
    bool isDragging;
    Overlay overlay;

    PlayerCam() : crewHoverIndex(-1), zoom(1.f), isDragging(false), overlay(Overlay::NONE)
    {
    }

    Vector2 GetWorldMousePos();
};

void HandleCameraMovement(PlayerCam &camera);
void HandleCameraOverlays(PlayerCam &camera);

// Utility function for Screen to World space transformations
Vector2 ScreenToWorld(const Vector2 &screenPos, const PlayerCam &camera);
Vector2Int ScreenToTile(const Vector2 &screenPos, const PlayerCam &camera);
Vector2 WorldToScreen(const Vector2 &worldPos, const PlayerCam &camera);