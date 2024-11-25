#pragma once
#include "utils.hpp"

struct EffectDef
{
private:
    std::string id;
    std::string spritesheet;
    uint16_t sizeIncrements;
    uint16_t spriteCount;
    float animationSpeed;

public:
    EffectDef(const std::string &id, const std::string &spritesheet, uint16_t sizeIncrements = 1, uint16_t spriteCount = 1, float animationSpeed = 0)
        : id(id), spritesheet(spritesheet), sizeIncrements(sizeIncrements), spriteCount(spriteCount), animationSpeed(animationSpeed) {}

    constexpr const std::string &GetId() const { return id; }
    constexpr const std::string &GetSpriteSheet() const { return spritesheet; }
    constexpr uint16_t GetSpriteCount() const { return spriteCount; }
    constexpr uint16_t GetSizeIncrements() const { return sizeIncrements; }
    constexpr float GetAnimationSpeed() const { return animationSpeed; }
};