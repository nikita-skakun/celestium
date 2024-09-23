#pragma once
#include "utils.h"
#include <magic_enum.hpp>
#include <memory>
#include <vector>

using namespace magic_enum::bitwise_operators;

struct VerticalTile;
struct Component;
struct Room;
struct Station;

struct IOxygenProducer
{
    virtual ~IOxygenProducer() = default;
};

struct Tile
{
    enum class Type : u_int8_t
    {
        DEFAULT,
        FLOOR,
        WALL,
        OXYGEN_PRODUCER,
    };

    enum class ID : u_int16_t
    {
        BLUE_FLOOR,
        WALL,
        OXYGEN_PRODUCER,
    };

    enum class Height : u_int8_t
    {
        NONE = 0,
        FLOOR = 1 << 0,
        WAIST = 1 << 1,
        CEILING = 1 << 2,
    };

    ID id;
    Vector2Int position;
    Vector2Int spriteOffset;
    std::shared_ptr<Component> component;
    std::shared_ptr<Room> room;
    std::shared_ptr<Station> station;
    std::vector<std::shared_ptr<VerticalTile>> verticalTiles;

    Tile() {}
    Tile(ID i, const Vector2Int &p, std::shared_ptr<Station> s, std::shared_ptr<Room> r = nullptr, std::shared_ptr<Component> c = nullptr)
        : id(i), position(p), component(c), room(r), station(s) {}

    std::string GetName() const;
    virtual bool IsWalkable() const = 0;
    virtual Height GetHeight() const = 0;

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
    FloorTile(ID i, const Vector2Int &p, std::shared_ptr<Station> s, std::shared_ptr<Room> r = nullptr, std::shared_ptr<Component> c = nullptr)
        : Tile(i, p, s, r, c), oxygen(TILE_OXYGEN_MAX) {}

    bool IsWalkable() const
    {
        return true;
    }

    Height GetHeight() const
    {
        return Height::FLOOR;
    }

    Type GetType() const
    {
        return Type::FLOOR;
    }
};

struct WallTile : Tile
{
    WallTile() : Tile() {}
    WallTile(ID i, const Vector2Int &p, std::shared_ptr<Station> s, std::shared_ptr<Room> r = nullptr, std::shared_ptr<Component> c = nullptr)
        : Tile(i, p, s, r, c) {}

    bool IsWalkable() const
    {
        return false;
    }

    Height GetHeight() const
    {
        return Height::FLOOR | Height::WAIST | Height::CEILING;
    }

    Type GetType() const
    {
        return Type::WALL;
    };
};

struct OxygenProducingTile : Tile, IOxygenProducer
{
    std::shared_ptr<FloorTile> floorTile;

    OxygenProducingTile(ID i, const Vector2Int &p, std::shared_ptr<FloorTile> f, std::shared_ptr<Station> s, std::shared_ptr<Room> r = nullptr, std::shared_ptr<Component> c = nullptr)
        : Tile(i, p, s, r, c), floorTile(f) {}

    bool IsWalkable() const
    {
        return false;
    }

    Height GetHeight() const
    {
        return Height::WAIST;
    }

    Type GetType() const
    {
        return Type::OXYGEN_PRODUCER;
    };
};

struct VerticalTile
{
    Vector2Int offset;
    Vector2Int spriteOffset;

    VerticalTile(const Vector2Int &o, const Vector2Int &sO) : offset(o), spriteOffset(sO) {}
};