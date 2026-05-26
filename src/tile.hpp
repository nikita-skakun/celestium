#pragma once
#include "tile_def.hpp"
#include <unordered_map>
#include <string>

struct Station;
struct Sprite;
struct Component;

struct Tile : public std::enable_shared_from_this<Tile>
{
private:
    std::shared_ptr<TileDef> tileDef;
    Vector2Int position;
    std::vector<std::shared_ptr<Sprite>> sprites;
    std::unordered_map<ComponentType, std::shared_ptr<Component>> components;
    std::shared_ptr<Station> station;

    Tile(const std::string &tileId, const Vector2Int &position, const std::shared_ptr<Station> &station);

public:
    static std::shared_ptr<Tile> CreateTile(const std::string &tileId, const Vector2Int &position, const std::shared_ptr<Station> &station, bool overwriteExisting, bool useResources, Rotation rotation = Rotation::UP);
    static std::shared_ptr<Tile> CreatePreviewTile(const std::string &tileId, const Vector2Int &position, const std::shared_ptr<Station> &station);
    void MoveTile(const Vector2Int &newPosition);
    void RotateTile();
    void DeleteTile(bool returnResources);

    constexpr const Vector2Int &GetPosition() const { return position; }
    void SetPosition(const Vector2Int &newPos) { position = newPos; }
    std::vector<Vector2Int> GetOccupiedPositions() const;

    TileHeight GetHeight() const { return tileDef->GetHeight(); }
    TileCategory GetCategory() const { return tileDef->GetCategory(); }

    const std::shared_ptr<Sprite> &GetSprite() const
    {
        static const std::shared_ptr<Sprite> nullSprite = nullptr;
        return sprites.empty() ? nullSprite : sprites.front();
    }
    void SetSprite(const std::shared_ptr<Sprite> &newSprite)
    {
        sprites.clear();
        if (newSprite)
            sprites.push_back(newSprite);
    }
    const std::vector<std::shared_ptr<Sprite>> &GetSprites() const { return sprites; }
    void ClearSprites() { sprites.clear(); }
    void AddSprite(const std::shared_ptr<Sprite> &newSprite) { sprites.push_back(newSprite); }

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