#pragma once
#include "component.h"
#include <unordered_set>

struct Room;
struct Station;

struct Tile
{
    enum class ID : u_int16_t
    {
        BLUE_FLOOR,
        WALL,
        OXYGEN_PRODUCER,
        BATTERY,
    };

    enum class Height : u_int8_t
    {
        NONE = 0,
        FLOOR = 1 << 0,
        WAIST = 1 << 1,
        CEILING = 1 << 2,
    };

    ID id;
    Height height;
    Vector2Int position;
    Vector2Int spriteOffset;
    std::unordered_set<std::shared_ptr<Component>> components;
    std::shared_ptr<Room> room;
    std::shared_ptr<Station> station;

    Tile() {}
    Tile(ID i, Height h, const Vector2Int &p, std::shared_ptr<Station> s, std::shared_ptr<Room> r = nullptr)
        : id(i), height(h), position(p), room(r), station(s) {}

    std::string GetName() const;
    bool IsWalkable() const;
    Height GetHeight() const;

    template <typename T>
    bool HasComponent() const
    {
        for (const auto &component : components)
        {
            if (std::dynamic_pointer_cast<T>(component))
            {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    std::shared_ptr<T> GetComponent() const
    {
        for (const auto &component : components)
        {
            if (auto trueComponent = std::dynamic_pointer_cast<T>(component))
            {
                return trueComponent;
            }
        }
        return nullptr;
    }

    template <typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args &&...args)
    {
        if (auto existingComponent = GetComponent<T>())
        {
            return existingComponent;
        }

        auto newComponent = std::make_shared<T>(std::forward<Args>(args)...);

        components.insert(newComponent);

        return newComponent;
    }

    template <typename T>
    bool RemoveComponent()
    {
        for (auto it = components.begin(); it != components.end(); ++it)
        {
            if (std::dynamic_pointer_cast<T>(*it))
            {
                components.erase(it);
                return true;
            }
        }
        return false;
    }
};