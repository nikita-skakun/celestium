#pragma once
#include "def_manager.hpp"

struct Room;
struct Station;

struct Tile : public std::enable_shared_from_this<Tile>
{
private:
    std::shared_ptr<TileDef> tileDef;
    Vector2Int position;
    Vector2Int spriteOffset;
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

    constexpr const Vector2Int &GetSpriteOffset() const { return spriteOffset; }
    constexpr void SetSpriteOffset(const Vector2Int &newOffset) { spriteOffset = newOffset; }

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