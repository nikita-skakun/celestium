#pragma once
#include "def_manager.hpp"

struct Room;
struct Station;

struct Sprite
{
    virtual ~Sprite() = default;
    virtual void Draw(const Vector2Int &position, const Color &tint) const = 0;
};

struct BasicSprite : public Sprite
{
    Vector2Int spriteOffset;

    BasicSprite(const Vector2Int &offset) : spriteOffset(offset) {}

    void Draw(const Vector2Int &position, const Color &tint) const override;
};

struct SpriteSlice
{
    Rectangle sourceRect;
    Vector2 destOffset;

    SpriteSlice() : sourceRect(Rectangle()), destOffset(Vector2()) {}
    SpriteSlice(const Rectangle &sourceRect, const Vector2 &destOffset) : sourceRect(sourceRect), destOffset(destOffset) {}
};

struct MultiSliceSprite : public Sprite
{
    std::vector<SpriteSlice> slices;

    MultiSliceSprite(const std::vector<SpriteSlice> &slices) : slices(slices) {}

    void Draw(const Vector2Int &position, const Color &tint) const override;
};

struct Tile : public std::enable_shared_from_this<Tile>
{
private:
    std::shared_ptr<TileDef> tileDef;
    Vector2Int position;
    std::shared_ptr<Sprite> sprite;
    std::unordered_set<std::shared_ptr<Component>> components;
    std::shared_ptr<Room> room;
    std::shared_ptr<Station> station;

    Tile(const std::string &defName, const Vector2Int &position, std::shared_ptr<Station> station, std::shared_ptr<Room> room = nullptr);

public:
    static std::shared_ptr<Tile> CreateTile(const std::string &defName, const Vector2Int &position, std::shared_ptr<Station> station, std::shared_ptr<Room> room = nullptr);
    void DeleteTile();

    constexpr const Vector2Int &GetPosition() const { return position; }
    constexpr void SetPosition(const Vector2Int &newPos) { position = newPos; }

    constexpr TileDef::Height GetHeight() const { return tileDef->GetHeight(); }

    constexpr const std::shared_ptr<Sprite> &GetSprite() const { return sprite; }
    void SetSprite(const std::shared_ptr<Sprite> &newSprite) { sprite = newSprite; }

    std::shared_ptr<TileDef> GetTileDefinition() const { return tileDef; }
    std::shared_ptr<Room> GetRoom() const { return room; }
    std::shared_ptr<Station> GetStation() const { return station; }

    constexpr const std::string &GetId() const { return tileDef->GetId(); }
    constexpr std::string GetName() const { return MacroCaseToName(GetId()); }

    std::string GetInfo() const
    {
        std::string tileInfo = " - " + GetName();

        if (auto durability = GetComponent<DurabilityComponent>())
        {
            tileInfo += std::format("\n   + HP: {:.1f} / {:.1f}", durability->GetHitpoints(), durability->GetMaxHitpoints());
        }
        if (auto door = GetComponent<DoorComponent>())
        {
            tileInfo += std::format("\n   + {}", door->IsOpen() ? "Open" : "Closed");
            tileInfo += std::format("\n   + State: {} ({:.0f}%)", door->GetMovementName(), door->GetProgress() * 100.);
        }
        if (auto oxygen = GetComponent<OxygenComponent>())
        {
            tileInfo += std::format("\n   + Oxygen: {:.0f}", oxygen->GetOxygenLevel());
        }
        if (auto battery = GetComponent<BatteryComponent>())
        {
            tileInfo += std::format("\n   + Energy: {:.0f} / {:.0f}", battery->GetChargeLevel(), battery->GetMaxChargeLevel());
        }
        if (auto powerConnector = GetComponent<PowerConnectorComponent>())
        {
            tileInfo += std::format("\n   + Power Connector: {} ({})", magic_enum::enum_flags_name(powerConnector->GetIo()), powerConnector->GetConnections().size());
        }

        return tileInfo;
    }

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