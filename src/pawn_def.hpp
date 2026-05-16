#pragma once
#include "direction.hpp"
#include <unordered_map>

enum class PawnAnimationType : uint8_t
{
    IDLE,
    WALKING,
};

struct PawnAnimation
{
    float speed; // Seconds per frame
    std::unordered_map<Direction, std::vector<Vector2Int>> framesByDirection;
};

struct PawnDef
{
private:
    const std::string id;
    const std::unordered_map<PawnAnimationType, PawnAnimation> animations;

    // Find the closest available direction to the requested direction
    Direction FindClosestDirection(const PawnAnimation &anim, Direction requestedDir) const
    {
        // If exact match exists, use it
        if (anim.framesByDirection.count(requestedDir))
            return requestedDir;

        // Convert requested direction to normalized vector
        Vector2Int dirVec = DirectionToVector2Int(requestedDir);
        Vector2 requestedVec = Vector2Normalize(Vector2(static_cast<float>(dirVec.x), static_cast<float>(dirVec.y)));

        Direction closest = anim.framesByDirection.begin()->first;
        float maxDot = -2.f;

        // Find direction with highest dot product (closest angle)
        for (const auto &[dir, frames] : anim.framesByDirection)
        {
            Vector2Int availableVec = DirectionToVector2Int(dir);
            Vector2 availableNorm = Vector2Normalize(Vector2(static_cast<float>(availableVec.x), static_cast<float>(availableVec.y)));
            float dot = Vector2Dot(requestedVec, availableNorm);

            if (dot > maxDot)
            {
                maxDot = dot;
                closest = dir;
            }
        }

        return closest;
    }

public:
    PawnDef(const std::string &id,
            const std::unordered_map<PawnAnimationType, PawnAnimation> &animations)
        : id(id), animations(animations) {}

    const std::string &GetId() const { return id; }
    const PawnAnimation &GetAnimation(PawnAnimationType type) const { return animations.at(type); }

    // Get frame for animation type and direction, automatically using closest available
    const Vector2Int &GetFrame(PawnAnimationType type, Direction dir, size_t frameIndex) const
    {
        const auto &anim = GetAnimation(type);
        Direction closestDir = FindClosestDirection(anim, dir);
        const auto &frames = anim.framesByDirection.at(closestDir);
        return frames[frameIndex % frames.size()];
    }

    float GetAnimationSpeed(PawnAnimationType type) const
    {
        return GetAnimation(type).speed;
    }
};
