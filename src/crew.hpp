#pragma once
#include "station.hpp"
#include "task.hpp"

struct Crew
{
    std::string name;
    Vector2 position;
    Color color;
    std::vector<std::shared_ptr<Task>> taskQueue;
    float oxygen;
    bool isAlive;
    std::shared_ptr<Tile> currentTile;

    constexpr Crew(const std::string &n, const Vector2 &p, const Color &c) : name(n), position(p), color(c), oxygen(CREW_OXYGEN_MAX), isAlive(true) {}

    constexpr void ConsumeOxygen(float deltaTime)
    {
        if (!isAlive)
            return;

        oxygen -= CREW_OXYGEN_USE * deltaTime;
        if (oxygen <= 0)
        {
            Die();
        }
    }

    constexpr void RefillOxygen(float deltaTime, float &sourceOxygen)
    {
        if (isAlive && oxygen < CREW_OXYGEN_MAX && sourceOxygen > 0.f)
        {
            float usedOxygen = std::min(std::min(sourceOxygen, CREW_OXYGEN_REFILL * deltaTime), CREW_OXYGEN_MAX - oxygen);
            oxygen += usedOxygen;
            sourceOxygen -= usedOxygen;
        }
    }

    constexpr bool CanPathInSpace() const
    {
        // Later could be true if they are wearing a spacesuit
        return false;
    }

    constexpr void Die()
    {
        isAlive = false;
        taskQueue.clear();
        oxygen = 0;
    }
};