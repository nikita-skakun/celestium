#pragma once
#include "utils.hpp"

struct Task;
struct Tile;

struct Crew
{
protected:
    std::string name;
    Vector2 position;
    Color color;
    std::vector<std::shared_ptr<Task>> taskQueue;
    float oxygen;
    float health;
    bool isAlive;
    std::weak_ptr<Tile> currentTile;

public:
    constexpr Crew(const std::string &n, const Vector2 &p, const Color &c)
        : name(n), position(p), color(c), oxygen(CREW_OXYGEN_MAX), health(CREW_HEALTH_MAX), isAlive(true) {}

    constexpr const std::string &GetName() const { return name; }
    constexpr const Vector2 &GetPosition() const { return position; }
    constexpr void SetPosition(const Vector2 &newPosition) { position = newPosition; }
    constexpr Color GetColor() const { return color; }
    constexpr const std::vector<std::shared_ptr<Task>> &GetReadOnlyTaskQueue() const { return taskQueue; }
    constexpr std::vector<std::shared_ptr<Task>> &GetTaskQueue() { return taskQueue; }
    constexpr void RemoveFirstTask() { taskQueue.erase(taskQueue.begin()); }
    constexpr float GetOxygen() const { return oxygen; }
    constexpr float GetHealth() const { return health; }
    constexpr bool IsAlive() const { return isAlive; }
    std::shared_ptr<Tile> GetCurrentTile() const { return currentTile.lock(); }
    constexpr void SetCurrentTile(const std::shared_ptr<Tile> &tile) { currentTile = tile; }

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

    constexpr void SetHealth(float newHealth)
    {
        health = std::clamp(newHealth, 0.f, CREW_HEALTH_MAX);

        if (health <= 0)
            Die();
    }

    std::string GetActionName() const;
    std::string GetInfo() const;

    constexpr void Die()
    {
        isAlive = false;
        taskQueue.clear();
        oxygen = 0;
        health = 0;
    }
};