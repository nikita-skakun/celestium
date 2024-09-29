#include "crew.hpp"

Crew::Crew(const std::string &n, const Vector2 &p, const Color &c)
    : name(n), position(p), color(c), oxygen(100.f), isAlive(true)
{
}

void Crew::ConsumeOxygen(float deltaTime)
{
    if (isAlive)
    {
        oxygen = std::max(oxygen - CREW_OXYGEN_USE * deltaTime, 0.0f);
        if (oxygen <= 0)
        {
            Die();
        }
    }
}

void Crew::RefillOxygen(float deltaTime, float &sourceOxygen)
{
    if (isAlive && oxygen < CREW_OXYGEN_MAX && sourceOxygen > 0.f)
    {
        float usedOxygen = std::min(std::min(sourceOxygen, CREW_OXYGEN_REFILL * deltaTime), CREW_OXYGEN_MAX - oxygen);
        oxygen += usedOxygen;
        sourceOxygen -= usedOxygen;
    }
}

bool Crew::CanPathInSpace() const
{
    // Later could be true if they are wearing a spacesuit
    return false;
}

void Crew::Die()
{
    isAlive = false;
    taskQueue.clear();
    oxygen = 0;
}
