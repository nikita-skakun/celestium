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

    Crew(const std::string &n, const Vector2 &p, const Color &c);

    void ConsumeOxygen(float deltaTime);
    void RefillOxygen(float deltaTime, float &sourceOxygen);
    bool CanPathInSpace() const;
    void Die();
};