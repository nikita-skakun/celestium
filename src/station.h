#pragma once
#include "utils.h"
#include <memory>
#include <vector>
#include <unordered_map>

struct Tile;
struct VerticalTile;
struct Component;
struct Room;
struct Station;

struct Tile
{
    enum class Type : u_int8_t
    {
        DEFAULT,
        FLOOR,
    };

    enum class ID : u_int16_t
    {
        BLUE_FLOOR,
        WALL,
        ID_LENGTH
    };

    static const std::string names[int(ID::ID_LENGTH)];

    Vector2 position;
    ID id;
    Vector2Int spriteOffset;
    std::shared_ptr<Component> component;
    std::shared_ptr<Room> room;
    std::shared_ptr<Station> station;
    std::vector<std::shared_ptr<VerticalTile>> verticalTiles;

    Tile() {}
    Tile(const Vector2 &p, ID i, const std::shared_ptr<Station> &s, const std::shared_ptr<Room> &r = nullptr, const std::shared_ptr<Component> &c = nullptr)
        : position(p), id(i), component(c), room(r), station(s) {}

    std::string GetName() const
    {
        return id == ID::ID_LENGTH ? "INVALID" : names[int(id)];
    }

    virtual bool IsWalkable() const
    {
        return false;
    }

    virtual Type GetType() const
    {
        return Type::DEFAULT;
    };
    
    virtual ~Tile() = default;
};

struct FloorTile : Tile
{
    float oxygen;

    FloorTile() : Tile(), oxygen(TILE_OXYGEN_MAX) {}
    FloorTile(const Vector2 &p, ID i, const std::shared_ptr<Station> &s, const std::shared_ptr<Room> &r = nullptr, const std::shared_ptr<Component> &c = nullptr)
        : Tile(p, i, s, r, c), oxygen(TILE_OXYGEN_MAX) {}

    bool IsWalkable() const
    {
        return true;
    }

    Type GetType() const
    {
        return Type::FLOOR;
    }
};

struct VerticalTile
{
    Vector2 offset;
    Vector2Int spriteOffset;

    VerticalTile(const Vector2 &o, const Vector2Int &sO) : offset(o), spriteOffset(sO) {}
};

struct Component
{
    std::vector<std::shared_ptr<Tile>> tiles;
    std::shared_ptr<Room> room;
    std::shared_ptr<Station> station;

    bool isInternal;
};

struct Room
{
    std::vector<std::shared_ptr<Tile>> tiles;
    std::vector<std::shared_ptr<Component>> components;
    std::shared_ptr<Station> station;

    Room(const std::shared_ptr<Station> &s)
        : station(s)
    {
    }
};

struct Station
{
    std::vector<std::shared_ptr<Tile>> tiles;
    std::vector<std::shared_ptr<Component>> components;
    std::vector<std::shared_ptr<Room>> rooms;
    std::unordered_map<Vector2, std::shared_ptr<Tile>, Vector2Hash, Vector2Equal> tileMap;

    std::shared_ptr<Tile> GetTileAtPosition(const Vector2 &pos) const;
    void UpdateSpriteOffsets();
};

std::shared_ptr<Tile> CreateTile(Tile::Type type, const Vector2 &position, Tile::ID id, const std::shared_ptr<Station> &station, const std::shared_ptr<Room> &room, const std::shared_ptr<Component> &component);
std::shared_ptr<Room> CreateEmptyRoom(const std::shared_ptr<Station> &station);
std::shared_ptr<Room> CreateRectRoom(const Vector2 &pos, const Vector2Int &size, const std::shared_ptr<Station> &station);
std::shared_ptr<Room> CreateHorizontalCorridor(const Vector2 &startPos, int length, const std::shared_ptr<Station> &station);
std::shared_ptr<Station> CreateStation();