#include "astar.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>

// Funnel algorithm for a sequence of shared edges
std::deque<Vector2> Funnel(Vector2 start, Vector2 end, const std::vector<std::pair<Vector2, Vector2>> &portals)
{
    std::deque<Vector2> waypoints;
    if (portals.empty())
    {
        waypoints.push_back(end);
        return waypoints;
    }

    std::vector<std::pair<Vector2, Vector2>> segments = portals;
    segments.push_back({end, end});

    Vector2 apex = start;
    Vector2 funnelLeft = segments[0].first;
    Vector2 funnelRight = segments[0].second;
    int leftIdx = 0, rightIdx = 0;

    auto CrossProduct = [](Vector2 a, Vector2 b, Vector2 c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    };

    for (int i = 1; i < (int)segments.size(); ++i)
    {
        Vector2 left = segments[i].first;
        Vector2 right = segments[i].second;

        // Update right leg
        if (CrossProduct(apex, funnelRight, right) <= 0.0f)
        {
            if (apex == funnelRight || CrossProduct(apex, funnelLeft, right) > 0.0f)
            {
                funnelRight = right;
                rightIdx = i;
            }
            else
            {
                apex = funnelLeft;
                waypoints.push_back(apex);
                funnelLeft = apex;
                funnelRight = apex;
                i = leftIdx;
                leftIdx = i;
                rightIdx = i;
                continue;
            }
        }

        // Update left leg
        if (CrossProduct(apex, funnelLeft, left) >= 0.0f)
        {
            if (apex == funnelLeft || CrossProduct(apex, funnelRight, left) < 0.0f)
            {
                funnelLeft = left;
                leftIdx = i;
            }
            else
            {
                apex = funnelRight;
                waypoints.push_back(apex);
                funnelLeft = apex;
                funnelRight = apex;
                i = rightIdx;
                leftIdx = i;
                rightIdx = i;
                continue;
            }
        }
    }

    waypoints.push_back(end);
    return waypoints;
}

std::deque<Vector2> FindPath(
    const Vector2 &start, 
    const Vector2 &end, 
    const std::vector<ConvexPolygon> &polygons, 
    std::function<bool(const ConvexPolygon::Link&)> isLinkTraversable
)
{
    int startPoly = -1, endPoly = -1;
    for (int i = 0; i < (int)polygons.size(); ++i)
    {
        if (startPoly == -1 && polygons[i].Contains(start)) startPoly = i;
        if (endPoly == -1 && polygons[i].Contains(end)) endPoly = i;
    }

    if (startPoly == -1 || endPoly == -1) return {};
    if (startPoly == endPoly) return {end};

    // A* on polygons
    struct Node {
        int polyIdx;
        float gCost, fCost;
        bool operator>(const Node &o) const { return fCost > o.fCost; }
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    std::unordered_map<int, float> minGCost;
    std::unordered_map<int, int> cameFrom;

    open.push({startPoly, 0.0f, Vector2Distance(start, end)});
    minGCost[startPoly] = 0.0f;

    while (!open.empty())
    {
        Node cur = open.top();
        open.pop();

        if (cur.gCost > minGCost[cur.polyIdx]) continue;
        if (cur.polyIdx == endPoly) break;

        const auto &poly = polygons[cur.polyIdx];
        Vector2 curCenter = poly.GetCenter();

        for (const auto &link : poly.links)
        {
            if (isLinkTraversable && !isLinkTraversable(link)) continue;

            int nbIdx = link.targetPolyIdx;
            const auto &nbPoly = polygons[nbIdx];
            
            Vector2 fromPos = (cur.polyIdx == startPoly) ? start : curCenter;
            float dist;
            if (nbIdx == endPoly) dist = Vector2Distance(fromPos, end);
            else dist = Vector2Distance(fromPos, nbPoly.GetCenter());

            // Add door penalty (e.g. 5 tiles distance equivalent)
            if (link.door.lock()) dist += 5.0f;

            float newG = cur.gCost + dist;

            if (!minGCost.contains(nbIdx) || newG < minGCost[nbIdx])
            {
                minGCost[nbIdx] = newG;
                cameFrom[nbIdx] = cur.polyIdx;
                float h = (nbIdx == endPoly) ? 0.0f : Vector2Distance(nbPoly.GetCenter(), end);
                open.push({nbIdx, newG, newG + h});
            }
        }
    }

    if (!minGCost.contains(endPoly)) 
    {
        TraceLog(LOG_INFO, "FindPath: No path found from %d to %d", startPoly, endPoly);
        return {};
    }

    // Reconstruct path and segments
    std::vector<int> path;
    for (int curr = endPoly; curr != startPoly; curr = cameFrom[curr]) path.push_back(curr);
    path.push_back(startPoly);
    std::reverse(path.begin(), path.end());

    std::string pathStr = "";
    for (int p : path) pathStr += std::to_string(p) + " ";
    TraceLog(LOG_INFO, "FindPath: Start (%f, %f) -> End (%f, %f)", start.x, start.y, end.x, end.y);
    TraceLog(LOG_INFO, "FindPath: Path: %s", pathStr.c_str());
    for (int p : path) {
        const auto& poly = polygons[p];
        TraceLog(LOG_INFO, "  Poly %d: (%.1f, %.1f) to (%.1f, %.1f)", p, poly.vertices[0].x, poly.vertices[0].y, poly.vertices[2].x, poly.vertices[2].y);
    }

    std::vector<std::pair<Vector2, Vector2>> segments;
    for (size_t i = 0; i < path.size() - 1; ++i)
    {
        const auto &p1 = polygons[path[i]];
        int nextPolyIdx = path[i + 1];
        
        for (const auto &link : p1.links)
        {
            if (link.targetPolyIdx == nextPolyIdx)
            {
                Vector2 left = link.portalA;
                Vector2 right = link.portalB;
                
                Vector2 center = p1.GetCenter();
                auto cp = (left.x - center.x) * (right.y - center.y) - (left.y - center.y) * (right.x - center.x);
                if (cp < 0) std::swap(left, right);

                float padding = 0.5f;
                Vector2 dir = {right.x - left.x, right.y - left.y};
                float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                
                if (dist > 0.001f) {
                    if (dist <= padding * 2.0f) {
                        Vector2 mid = {(left.x + right.x) * 0.5f, (left.y + right.y) * 0.5f};
                        left = mid;
                        right = mid;
                    } else {
                        left.x += (dir.x / dist) * padding;
                        left.y += (dir.y / dist) * padding;
                        right.x -= (dir.x / dist) * padding;
                        right.y -= (dir.y / dist) * padding;
                    }
                }

                // Double-portal padding: one offset into current, one into next
                Vector2 normal = {0, 0};
                if (link.edgeIdx == 0) normal.y = 1;       // North edge of p1
                else if (link.edgeIdx == 1) normal.x = -1; // East
                else if (link.edgeIdx == 2) normal.y = -1; // South
                else if (link.edgeIdx == 3) normal.x = 1;  // West

                segments.push_back({
                    {left.x + normal.x * padding, left.y + normal.y * padding},
                    {right.x + normal.x * padding, right.y + normal.y * padding}
                });

                segments.push_back({
                    {left.x - normal.x * padding, left.y - normal.y * padding},
                    {right.x - normal.x * padding, right.y - normal.y * padding}
                });

                break;
            }
        }
    }

    for (int i = 0; i < (int)segments.size(); ++i) {
        TraceLog(LOG_INFO, "  Portal %d: (%f, %f) - (%f, %f)", i, segments[i].first.x, segments[i].first.y, segments[i].second.x, segments[i].second.y);
    }

    auto finalPath = Funnel(start, end, segments);
    TraceLog(LOG_INFO, "FindPath: Waypoints (%d):", (int)finalPath.size());
    for (int i = 0; i < (int)finalPath.size(); ++i) {
        TraceLog(LOG_INFO, "  WP %d: (%f, %f)", i, finalPath[i].x, finalPath[i].y);
    }

    return finalPath;
}
