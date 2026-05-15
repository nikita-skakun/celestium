#pragma once
#include "utils.hpp"

struct Tile;

/**
 * @brief A sub-area within the station ensuring safe straight-line movement.
 * These are the nodes in the navigation graph.
 */
struct ConvexPolygon {
    Vector2 vertices[4];                    // Vertices in world space
    
    struct Link {
        int targetPolyIdx;
        int edgeIdx; // which edge of THIS polygon connects to targetPolyIdx
        Vector2 portalA, portalB; // The specific segment that is passable
        std::weak_ptr<Tile> door;
    };
    std::vector<Link> links;
    int roomId = -1;                        // The room this polygon belongs to
    Rectangle bounds = {0, 0, 0, 0};        // Bounding box for fast Contains check

    void RecalculateBounds() {
        float minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
        for (int i = 0; i < 4; ++i) {
            minX = std::min(minX, vertices[i].x);
            maxX = std::max(maxX, vertices[i].x);
            minY = std::min(minY, vertices[i].y);
            maxY = std::max(maxY, vertices[i].y);
        }
        bounds = {minX, minY, maxX - minX, maxY - minY};
    }

    /**
     * @brief Gets the center point of the polygon.
     */
    Vector2 GetCenter() const {
        Vector2 center = {0, 0};
        for (int i = 0; i < 4; ++i) center.x += vertices[i].x, center.y += vertices[i].y;
        center.x /= 4.f;
        center.y /= 4.f;
        return center;
    }
};

/**
 * @brief Represents a metadata container for a walkable area.
 * Rooms are collections of convex polygons.
 */
struct Room {
    int id = -1;
    std::vector<int> polygonIds;
};
