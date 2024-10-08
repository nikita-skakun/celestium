#pragma once
#include "utils.hpp"

struct Hazard
{
    Vector2Int position;

    constexpr Hazard(const Vector2Int position) : position(position) {}
    virtual ~Hazard() = default;
};

struct FireHazard : public Hazard
{
    constexpr FireHazard(const Vector2Int position) : Hazard(position) {}
};