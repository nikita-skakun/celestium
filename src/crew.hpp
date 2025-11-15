#pragma once
#include "utils.hpp"
#include <deque>
#include <atomic>

struct Action;
struct Tile;

struct Crew
{
protected:
    std::string name;
    Vector2 position;
    Color color;
    std::deque<std::shared_ptr<Action>> actionQueue;
    float oxygen;
    float health;
    bool isAlive;
    std::weak_ptr<Tile> currentTile;
    uint64_t instanceId;
    static std::atomic<uint64_t> nextInstanceId;

public:
    Crew(const std::string &n, const Vector2 &p, const Color &c)
        : name(n), position(p), color(c), oxygen(CREW_OXYGEN_MAX), health(CREW_HEALTH_MAX), isAlive(true)
    {
        instanceId = nextInstanceId.fetch_add(1);
    }

    const std::string &GetName() const { return name; }
    const Vector2 &GetPosition() const { return position; }
    void SetPosition(const Vector2 &newPosition) { position = newPosition; }
    Color GetColor() const { return color; }
    const std::deque<std::shared_ptr<Action>> &GetReadOnlyActionQueue() const { return actionQueue; }
    std::deque<std::shared_ptr<Action>> &GetActionQueue() { return actionQueue; }
    void RemoveFirstAction() { actionQueue.pop_front(); }
    float GetOxygen() const { return oxygen; }
    float GetHealth() const { return health; }
    bool IsAlive() const { return isAlive; }
    std::shared_ptr<Tile> GetCurrentTile() const { return currentTile.lock(); }
    void SetCurrentTile(const std::shared_ptr<Tile> &tile) { currentTile = tile; }

    void ConsumeOxygen(float deltaTime)
    {
        if (!isAlive)
            return;

        oxygen -= CREW_OXYGEN_USE * deltaTime;
        if (oxygen <= 0)
        {
            Die();
        }
    }

    void RefillOxygen(float deltaTime, float &sourceOxygen)
    {
        if (isAlive && oxygen < CREW_OXYGEN_MAX && sourceOxygen > 0.f)
        {
            float usedOxygen = std::min(std::min(sourceOxygen, CREW_OXYGEN_REFILL * deltaTime), CREW_OXYGEN_MAX - oxygen);
            oxygen += usedOxygen;
            sourceOxygen -= usedOxygen;
        }
    }

    void SetHealth(float newHealth)
    {
        health = std::clamp(newHealth, 0.f, CREW_HEALTH_MAX);

        if (health <= 0)
            Die();
    }

    std::string GetActionName() const;
    std::string GetInfo() const;
    uint64_t GetInstanceId() const { return instanceId; }

    void Die()
    {
        isAlive = false;
        actionQueue.clear();
        oxygen = 0;
        health = 0;
    }
};