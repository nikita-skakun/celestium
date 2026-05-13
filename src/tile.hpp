#pragma once
#include "tile_def.hpp"
#include <memory>
#include <unordered_map>
#include <string>

struct Station;
struct Sprite;
struct Component;

enum class ComponentType : uint8_t;

struct Tile : public std::enable_shared_from_this<Tile>
{
private:
    std::shared_ptr<TileDef> tileDef;
    Vector2Int position;
    std::shared_ptr<Sprite> sprite;
    std::unordered_map<ComponentType, std::shared_ptr<Component>> components;
    std::shared_ptr<Station> station;

    Tile(const std::string &tileId, const Vector2Int &position, const std::shared_ptr<Station> &station);

public:
    static std::shared_ptr<Tile> CreateTile(const std::string &tileId, const Vector2Int &position, const std::shared_ptr<Station> &station, bool overwriteExisting, bool useResources);
    void MoveTile(const Vector2Int &newPosition);
    void RotateTile();
    void DeleteTile(bool returnResources);

    constexpr const Vector2Int &GetPosition() const { return position; }

    TileDef::Height GetHeight() const { return tileDef->GetHeight(); }
    TileDef::Category GetCategory() const { return tileDef->GetCategory(); }

    const std::shared_ptr<Sprite> &GetSprite() const { return sprite; }
    void SetSprite(const std::shared_ptr<Sprite> &newSprite) { sprite = newSprite; }

    std::shared_ptr<TileDef> GetTileDefinition() const { return tileDef; }
    std::shared_ptr<Station> GetStation() const { return station; }

    bool IsActive() const;

    const std::string &GetId() const { return tileDef->GetId(); }
    std::string GetName() const { return tileDef->GetName(); }
    std::string GetInfo() const;

    template <typename T>
    std::shared_ptr<T> GetComponent() const
    {
        for (const auto &[type, component] : components)
        {
            if (auto castedComponent = std::dynamic_pointer_cast<T>(component))
                return castedComponent;
        }
        return nullptr;
    }

    bool HasComponent(ComponentType type) const;

    template <typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args &&...args)
    {
        ComponentType type = T::GetStaticType();
        auto it = components.find(type);
        if (it != components.end())
        {
            return std::dynamic_pointer_cast<T>(it->second);
        }
        auto newComponent = std::make_shared<T>(std::forward<Args>(args)...);
        components[type] = newComponent;
        return newComponent;
    }

    template <typename T>
    bool RemoveComponent()
    {
        return components.erase(T::GetStaticType()) > 0;
    }

    static bool CompareByHeight(const std::shared_ptr<Tile> &a, const std::shared_ptr<Tile> &b);
};