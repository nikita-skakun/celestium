#include "astar.hpp"
#include "game_state.hpp"
#include "station.hpp"
#include <queue>
#include <unordered_set>

/**
 * Implements the A* pathfinding algorithm to find the shortest path
 * from a start position to an end position in a grid-based environment.
 *
 * @param start     The starting position as a Vector2Int.
 * @param end       The target position as a Vector2Int.
 * @param heuristic A function that estimates the cost between two points.
 * @return          A deque of Vector2Int positions representing the path.
 */
std::deque<Vector2Int> AStar(const Vector2Int &start, const Vector2Int &end, const HeuristicFunction &heuristic)
{
    auto station = GameManager::GetStation();
    if (start == end || !station)
        return {};

    // Combined cost map for tracking both g and f costs
    std::unordered_map<Vector2Int, Vector2> costMap;
    // Map to reconstruct the path
    std::unordered_map<Vector2Int, Vector2Int> cameFrom;
    // Map of nodes already evaluated
    std::unordered_set<Vector2Int> closedSet;

    // Comparator for the priority queue
    auto compare = [&costMap](const Vector2Int &a, const Vector2Int &b)
    {
        return costMap[a].y > costMap[b].y;
    };
    // Sorted queue of open nodes
    std::priority_queue<Vector2Int, std::vector<Vector2Int>, decltype(compare)> openQueue(compare);

    // Initialize costs for the start node
    costMap[start] = Vector2(0, heuristic(start, end));
    // Add start to the queue of open nodes
    openQueue.emplace(start);

    const Vector2Int neighborOffsets[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};

    // Main loop of the A* algorithm
    while (!openQueue.empty())
    {
        // Get the node with the lowest fCost
        Vector2Int current = openQueue.top();
        openQueue.pop();

        // If we reach the end node, reconstruct the path
        if (current == end)
        {
            std::deque<Vector2Int> path;
            Vector2Int step = end;
            Vector2Int prev = cameFrom[end];
            while (step != start)
            {
                path.push_front(step);
                step = prev;
                prev = cameFrom[step];
            }
            return path;
        }

        // Mark the current node as evaluated
        closedSet.insert(current);

        // Check neighboring nodes
        for (const Vector2Int &offset : neighborOffsets)
        {
            // Calculate neighbor position
            Vector2Int neighborPos = current + offset;

            // Check if the move is diagonal
            if (offset.x != 0 && offset.y != 0)
            {
                // Ensure both adjacent sides are walkable for diagonals
                if (!station->IsPositionPathable(Vector2Int(current.x + offset.x, current.y)) ||
                    !station->IsPositionPathable(Vector2Int(current.x, current.y + offset.y)))
                    continue; // Skip to the next neighbor
            }

            // Check if neighbor is pathable and not in the closed set
            if (station->IsPositionPathable(neighborPos) && !Contains(closedSet, neighborPos))
            {
                // Calculate tentative G cost
                float tentativeGCost = costMap[current].x + heuristic(current, neighborPos);
                // Insert or get existing cost
                float inf = std::numeric_limits<float>::infinity();
                auto [iter, inserted] = costMap.try_emplace(neighborPos, Vector2(inf, inf));

                // If the new cost is lower, update costs and path tracking
                if (tentativeGCost < iter->second.x)
                {
                    // Update G cost
                    iter->second.x = tentativeGCost;
                    // Update F cost
                    iter->second.y = tentativeGCost + heuristic(neighborPos, end);
                    // Track the path
                    cameFrom[neighborPos] = current;

                    // Add neighbor to the open list
                    openQueue.push(neighborPos);
                }
            }
        }
    }

    // Return an empty queue if no path found
    return {};
}

/**
 * Checks if there are any obstacles along the given path.
 *
 * @param path            The queue of Vector2Int positions representing the path.
 *
 * @return                True if an obstacle is found or station is null, false otherwise.
 */
bool DoesPathHaveObstacles(const std::deque<Vector2Int> &path)
{
    auto station = GameManager::GetStation();
    // Return that obstacle is found if station is null
    if (!station)
        return true;

    for (const Vector2Int &step : path)
    {
        // Return that an obstacle is found if the tile is solid or not walkable
        if (!station->IsPositionPathable(step))
            return true;
    }

    // No obstacles found
    return false;
}
