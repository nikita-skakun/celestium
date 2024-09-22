#pragma once
#include "utils.h"
#include <magic_enum.hpp>
#include <memory>
#include <vector>

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
        WALL,
    };

    enum class ID : u_int16_t
    {
        BLUE_FLOOR,
        WALL,
    };

    Vector2 position;
    ID id;
    Vector2Int spriteOffset;
    std::shared_ptr<Component> component;
    std::shared_ptr<Room> room;
    std::shared_ptr<Station> station;
    std::vector<std::shared_ptr<VerticalTile>> verticalTiles;

    Tile() {}
    Tile(const Vector2 &p, ID i, std::shared_ptr<Station> s, std::shared_ptr<Room> r = nullptr, std::shared_ptr<Component> c = nullptr)
        : position(p), id(i), component(c), room(r), station(s) {}

    std::string GetName() const;

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
    FloorTile(const Vector2 &p, ID i, std::shared_ptr<Station> s, std::shared_ptr<Room> r = nullptr, std::shared_ptr<Component> c = nullptr)
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

struct WallTile : Tile
{
    WallTile() : Tile() {}
    WallTile(const Vector2 &p, ID i, std::shared_ptr<Station> s, std::shared_ptr<Room> r = nullptr, std::shared_ptr<Component> c = nullptr)
        : Tile(p, i, s, r, c) {}

    bool IsWalkable() const
    {
        return false;
    }

    Type GetType() const
    {
        return Type::WALL;
    };
};

struct VerticalTile
{
    Vector2 offset;
    Vector2Int spriteOffset;

    VerticalTile(const Vector2 &o, const Vector2Int &sO) : offset(o), spriteOffset(sO) {}
};