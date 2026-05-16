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

    for (int i = 1; i < (int)segments.size(); ++i)
    {
        Vector2 left = segments[i].first;
        Vector2 right = segments[i].second;

        // Update right leg
        if (Vector2Cross(apex, funnelRight, right) <= 0.f)
        {
            if (apex == funnelRight || Vector2Cross(apex, funnelLeft, right) > 0.f)
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
        if (Vector2Cross(apex, funnelLeft, left) >= 0.f)
        {
            if (apex == funnelLeft || Vector2Cross(apex, funnelRight, left) < 0.f)
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
    std::function<bool(const ConvexPolygon::Link &)> isLinkTraversable)
{
    int startPoly = -1, endPoly = -1;
    for (int i = 0; i < (int)polygons.size(); ++i)
    {
        if (startPoly == -1 && IsVector2WithinRect(polygons[i].bounds, start))
            startPoly = i;
        if (endPoly == -1 && IsVector2WithinRect(polygons[i].bounds, end))
            endPoly = i;
    }

    if (startPoly == -1 || endPoly == -1)
        return {};
    if (startPoly == endPoly)
        return {end};

    // A* on polygons
    struct Node
    {
        int polyIdx;
        float gCost, fCost;
        bool operator>(const Node &o) const { return fCost > o.fCost; }
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    std::vector<float> minGCost(polygons.size(), std::numeric_limits<float>::max());
    std::vector<int> cameFrom(polygons.size(), -1);

    open.push({startPoly, 0.f, Vector2Distance(start, end)});
    minGCost[startPoly] = 0.f;

    while (!open.empty())
    {
        Node cur = open.top();
        open.pop();

        if (cur.gCost > minGCost[cur.polyIdx])
            continue;
        if (cur.polyIdx == endPoly)
            break;

        const auto &poly = polygons[cur.polyIdx];
        Vector2 curCenter = poly.GetCenter();

        for (const auto &link : poly.links)
        {
            if (isLinkTraversable && !isLinkTraversable(link))
                continue;

            int nbIdx = link.targetPolyIdx;
            const auto &nbPoly = polygons[nbIdx];

            Vector2 fromPos = (cur.polyIdx == startPoly) ? start : curCenter;
            float dist;
            if (nbIdx == endPoly)
                dist = Vector2Distance(fromPos, end);
            else
                dist = Vector2Distance(fromPos, nbPoly.GetCenter());

            // Add door penalty (e.g. 5 tiles distance equivalent)
            if (link.door.lock())
                dist += 5.f;

            float newG = cur.gCost + dist;

            if (newG < minGCost[nbIdx])
            {
                minGCost[nbIdx] = newG;
                cameFrom[nbIdx] = cur.polyIdx;
                float h = (nbIdx == endPoly) ? 0.0f : Vector2Distance(nbPoly.GetCenter(), end);
                open.push({nbIdx, newG, newG + h});
            }
        }
    }

    if (minGCost[endPoly] == std::numeric_limits<float>::max())
        return {};

    // Reconstruct path and segments
    std::vector<int> path;
    for (int curr = endPoly; curr != startPoly; curr = cameFrom[curr])
        path.push_back(curr);
    path.push_back(startPoly);
    std::reverse(path.begin(), path.end());

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
                if (cp < 0)
                    std::swap(left, right);

                float padding = 0.5f;
                Vector2 dir = {right.x - left.x, right.y - left.y};
                float distSq = Vector2LengthSq(dir);

                const float minDist = .001f;
                if (distSq > minDist * minDist)
                {
                    const float maxPad = padding * 2.f;
                    if (distSq <= maxPad * maxPad)
                    {
                        Vector2 mid = {(left.x + right.x) * .5f, (left.y + right.y) * .5f};
                        left = mid;
                        right = mid;
                    }
                    else
                    {
                        float dist = std::sqrt(distSq);
                        float inv = padding / dist;
                        left.x += dir.x * inv;
                        left.y += dir.y * inv;
                        right.x -= dir.x * inv;
                        right.y -= dir.y * inv;
                    }
                }

                // Double-portal padding: one offset into current, one into next
                Vector2 normal = {0, 0};
                if (link.edgeIdx == 0)
                    normal.y = 1; // North edge of p1
                else if (link.edgeIdx == 1)
                    normal.x = -1; // East
                else if (link.edgeIdx == 2)
                    normal.y = -1; // South
                else if (link.edgeIdx == 3)
                    normal.x = 1; // West

                segments.push_back({{left.x + normal.x * padding, left.y + normal.y * padding},
                                    {right.x + normal.x * padding, right.y + normal.y * padding}});

                segments.push_back({{left.x - normal.x * padding, left.y - normal.y * padding},
                                    {right.x - normal.x * padding, right.y - normal.y * padding}});

                break;
            }
        }
    }

    return Funnel(start, end, segments);
}
