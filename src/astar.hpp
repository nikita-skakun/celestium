#pragma once
#include "navigation.hpp"
#include <deque>
#include <functional>

std::deque<Vector2> FindPath(
    const Vector2 &start, 
    const Vector2 &end, 
    const std::vector<ConvexPolygon> &polygons, 
    std::function<bool(const ConvexPolygon::Link&)> isLinkTraversable = nullptr
);