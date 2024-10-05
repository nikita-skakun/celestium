#include "astar.hpp"

/**
 * Implements the A* pathfinding algorithm to find the shortest path
 * from a start position to an end position in a grid-based environment.
 *
 * @param start    The starting position as a Vector2Int.
 * @param end      The target position as a Vector2Int.
 * @param station  A shared pointer to the Station.
 * @param heuristic A function that estimates the cost between two points.
 * @return         A queue of Vector2Int positions representing the path.
 */
std::deque<Vector2Int> AStar(const Vector2Int &start, const Vector2Int &end,
                             std::shared_ptr<Station> station, const HeuristicFunction &heuristic)
{
    if (start == end || !station)
        return {};

    // Combined cost map for tracking both g and f costs
    std::unordered_map<Vector2Int, Vector2, Vector2Int::Hash> costMap;
    // Map to reconstruct the path
    std::unordered_map<Vector2Int, Vector2Int, Vector2Int::Hash> cameFrom;
    // Map of nodes already evaluated
    std::unordered_set<Vector2Int, Vector2Int::Hash> closedSet;

    // Comparator for the priority queue
    auto compare = [&costMap](const Vector2Int &a, const Vector2Int &b)
    {
        return costMap[a].y > costMap[b].y;
    };
    // Sorted queue of open nodes
    std::priority_queue<Vector2Int, std::vector<Vector2Int>, decltype(compare)> openQueue(compare);

    // Initialize costs for the start node
    costMap[start] = Vector2(0.f, heuristic(start, end));
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
            for (auto [step, prev] = std::tuple{end, cameFrom[end]}; step != start; std::tie(step, prev) = std::tuple{prev, cameFrom[prev]})
            {
                path.push_front(step);
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
            bool isDiagonal = (offset.x != 0 && offset.y != 0);
            if (isDiagonal)
            {
                // Check adjacent tiles for diagonal movement
                Vector2Int side1Pos = {current.x + offset.x, current.y};
                Vector2Int side2Pos = {current.x, current.y + offset.y};
                std::shared_ptr<Tile> side1Tile = station->GetTileAtPosition(side1Pos);
                std::shared_ptr<Tile> side2Tile = station->GetTileAtPosition(side2Pos);

                // Ensure both adjacent sides are walkable for diagonals
                if (!side1Tile || !side2Tile || !side1Tile->HasComponent<WalkableComponent>() || !side2Tile->HasComponent<WalkableComponent>())
                    continue; // Skip to the next neighbor
            }

            // Check if neighbor is not solid, walkable and not in the closed set
            if (!station->GetTileWithComponentAtPosition<SolidComponent>(neighborPos) &&
                station->GetTileWithComponentAtPosition<WalkableComponent>(neighborPos) &&
                !Contains(closedSet, neighborPos))
            {
                // Calculate tentative G cost
                float tentativeGCost = costMap[current].x + heuristic(current, neighborPos);
                // Insert or get existing cost
                auto [iter, inserted] = costMap.try_emplace(neighborPos, Vector2(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()));

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
                    openQueue.emplace(neighborPos);
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
 * @param station         Shared pointer to the Station containing the tiles.
 *
 * @return                True if an obstacle is found or station is null, false otherwise.
 */
bool DoesPathHaveObstacles(const std::deque<Vector2Int> &path, std::shared_ptr<Station> station)
{
    // Return that obstacle is found if station is null
    if (!station)
        return true;

    for (const Vector2Int &step : path)
    {
        // Return that an obstacle is found if the tile is solid or not walkable
        if (station->GetTileWithComponentAtPosition<SolidComponent>(step) ||
            !station->GetTileWithComponentAtPosition<WalkableComponent>(step))
            return true;
    }

    // No obstacles found
    return false;
}
