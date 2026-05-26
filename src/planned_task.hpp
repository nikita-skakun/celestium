#pragma once
#include "direction.hpp"
#include "utils.hpp"

struct Tile;

struct PlannedTask
{
    Vector2Int position;
    std::string tileId;
    bool isBuild;
    float progress;
    Rotation rotation;
    mutable std::shared_ptr<Tile> previewTile = nullptr;

    PlannedTask(const Vector2Int &position, const std::string &tileId, bool isBuild, Rotation rotation = Rotation::UP, float progress = 0.f)
        : position(position), tileId(tileId), isBuild(isBuild), progress(progress), rotation(rotation) {}
};