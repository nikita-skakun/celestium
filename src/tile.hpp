#pragma once
#include "def_manager.hpp"

struct Station;

struct Tile : public std::enable_shared_from_this<Tile>
{
private:
    std::shared_ptr<TileDef> tileDef;
    Vector2Int position;
    std::shared_ptr<Sprite> sprite;
    std::unordered_set<std::shared_ptr<Component>> components;
    std::shared_ptr<Station> station;

    Tile(const std::string &defName, const Vector2Int &position, std::shared_ptr<Station> station);

public:
    static std::shared_ptr<Tile> CreateTile(const std::string &defName, const Vector2Int &position, std::shared_ptr<Station> station);
    void MoveTile(const Vector2Int &newPosition);
    void DeleteTile();

    constexpr const Vector2Int &GetPosition() const { return position; }

    constexpr TileDef::Height GetHeight() const { return tileDef->GetHeight(); }

    constexpr const std::shared_ptr<Sprite> &GetSprite() const { return sprite; }
    void SetSprite(const std::shared_ptr<Sprite> &newSprite) { sprite = newSprite; }

    std::shared_ptr<TileDef> GetTileDefinition() const { return tileDef; }
    std::shared_ptr<Station> GetStation() const { return station; }

    constexpr const std::string &GetId() const { return tileDef->GetId(); }
    constexpr std::string GetName() const { return MacroCaseToName(GetId()); }
    std::string GetInfo() const;

    template <typename T>
    constexpr std::shared_ptr<T> GetComponent() const
    {
        for (const auto &component : components)
        {
            if (auto castedComponent = std::dynamic_pointer_cast<T>(component))
                return castedComponent;
        }
        return nullptr;
    }

    template <typename T>
    constexpr bool HasComponent() const
    {
        for (const auto &component : components)
        {
            if (std::dynamic_pointer_cast<T>(component))
                return true;
        }
        return false;
    }

    template <typename T, typename... Args>
    constexpr std::shared_ptr<T> AddComponent(Args &&...args)
    {
        if (auto existingComponent = GetComponent<T>())
            return existingComponent;

        auto newComponent = std::make_shared<T>(std::forward<Args>(args)...);
        components.insert(newComponent);
        return newComponent;
    }

    template <typename T>
    constexpr bool RemoveComponent()
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

    static constexpr bool CompareByHeight(const std::shared_ptr<Tile> &a, const std::shared_ptr<Tile> &b)
    {
        return magic_enum::enum_underlying(a->GetHeight()) < magic_enum::enum_underlying(b->GetHeight());
    }
};