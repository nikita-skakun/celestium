#include "astar.h"

/**
 * Implements the A* pathfinding algorithm to find the shortest path
 * from a start position to an end position in a grid-based environment.
 *
 * @param startRaw The starting position as a Vector2.
 * @param endRaw   The target position as a Vector2.
 * @param station  A shared pointer to the Station.
 * @return         A queue of Vector2 positions representing the path,
 *                 or an empty queue if no path is found.
 */
std::queue<Vector2> AStar(const Vector2 &startRaw, const Vector2 &endRaw, std::shared_ptr<Station> station)
{
    // Cost maps for tracking the cost to reach each node
    std::unordered_map<Vector2, float, Vector2Hash, Vector2Equal> gCost, fCost;
    // Map to reconstruct the path
    std::unordered_map<Vector2, Vector2, Vector2Hash, Vector2Equal> cameFrom;
    // Map of nodes already evaluated
    std::unordered_set<Vector2, Vector2Hash, Vector2Equal> closedSet;

    // Comparator for the priority queue
    auto compare = [&fCost](const Vector2 &a, const Vector2 &b)
    {
        // Min-heap based on fCost
        return fCost[a] > fCost[b];
    };
    // Sorted queue of open nodes
    std::priority_queue<Vector2, std::vector<Vector2>, decltype(compare)> openQueue(compare);

    // Positions floored to target the center of the tile
    Vector2 start = Vector2Floor(startRaw);
    Vector2 end = Vector2Floor(endRaw);

    // Initialize costs for the start node
    gCost[start] = 0;
    fCost[start] = Vector2DistanceSq(start, end);
    // Add start to the queue of open nodes
    openQueue.push(start);

    const Vector2 neighborOffsets[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};

    // Main loop of the A* algorithm
    while (!openQueue.empty())
    {
        // Get the node with the lowest fCost
        Vector2 current = openQueue.top();
        openQueue.pop();

        // If we reach the end node, reconstruct the path
        if (current == end)
        {
            // The path backwards
            std::vector<Vector2> backPath;
            for (Vector2 step = end; step != start; step = cameFrom[step])
            {
                // Backtrack to get the path
                backPath.push_back(step);
            }
            // Reverse to get the correct order
            std::reverse(backPath.begin(), backPath.end());

            // Prepare the final path as a queue
            std::queue<Vector2> path;
            for (const auto &step : backPath)
            {
                // Push each step to the path queue
                path.push(step);
            }

            // Return the complete path
            return path;
        }

        // Mark the current node as evaluated
        closedSet.insert(current);

        // Check neighboring nodes
        for (const Vector2 &offset : neighborOffsets)
        {
            // Calculate neighbor position
            Vector2 neighborPos = current + offset;
            // Get the neighbor tile
            std::shared_ptr<Tile> neighborTile = station->GetTileAtPosition(neighborPos);

            // Check if the move is diagonal
            bool isDiagonal = (offset.x != 0 && offset.y != 0);
            if (isDiagonal)
            {
                // Check adjacent tiles for diagonal movement
                Vector2 side1Pos = {current.x + offset.x, current.y};
                Vector2 side2Pos = {current.x, current.y + offset.y};
                std::shared_ptr<Tile> side1Tile = station->GetTileAtPosition(side1Pos);
                std::shared_ptr<Tile> side2Tile = station->GetTileAtPosition(side2Pos);

                // Ensure both adjacent sides are walkable for diagonals
                if (!side1Tile || !side2Tile || !side1Tile->IsWalkable() || !side2Tile->IsWalkable())
                    continue; // Skip to the next neighbor
            }

            // Evaluate the neighbor if it is walkable and not in the closed set
            if (neighborTile && neighborTile->IsWalkable() && closedSet.find(neighborPos) == closedSet.end())
            {
                // Calculate tentative G cost
                float tentativeGCost = gCost[current] + Vector2DistanceSq(current, neighborPos);
                // Insert or get existing cost
                auto [iter, inserted] = gCost.try_emplace(neighborPos, std::numeric_limits<float>::infinity());

                // If the new cost is lower, update costs and path tracking
                if (tentativeGCost < iter->second)
                {
                    // Update G cost
                    iter->second = tentativeGCost;
                    // Update F cost
                    fCost[neighborPos] = tentativeGCost + Vector2DistanceSq(neighborPos, end);
                    // Track the path
                    cameFrom[neighborPos] = current;

                    // Add neighbor to the open list
                    openQueue.push(neighborPos);
                }
            }
        }
    }

    // Return an empty queue if no path found
    return std::queue<Vector2>();
}

/**
 * Checks if there are any obstacles along the given path.
 *
 * @param path            The queue of Vector2 positions representing the path.
 * @param station         Shared pointer to the Station containing the tiles.
 * @param canPathInSpace  If true, allows pathing through empty space (no tiles).
 *
 * @return                True if an obstacle is found or station is null, false otherwise.
 */
bool DoesPathHaveObstacles(const std::queue<Vector2> &path, std::shared_ptr<Station> station, bool canPathInSpace)
{
    // Return that obstacle is found if station is null
    if (!station)
        return true;

    // Copy the path queue locally
    std::queue<Vector2> pathCopy = path;

    // Traverse the path
    while (!pathCopy.empty())
    {
        // Advance to the next step
        Vector2 step = pathCopy.front();
        pathCopy.pop();

        // Get tile at current step
        std::shared_ptr<Tile> tile = station->GetTileAtPosition(step);

        // Return that an obstacle is found if the tile is missing and pathing in space is not allowed
        if (!tile && !canPathInSpace)
            return true;

        // Return that an obstacle is found if the tile is not walkable
        if (tile && !tile->IsWalkable())
            return true;
    }

    // No obstacles found
    return false;
}
