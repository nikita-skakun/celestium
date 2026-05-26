#pragma once
#include <cstdint>
#include <magic_enum/magic_enum_flags.hpp>

enum class TileHeight : uint8_t
{
    NONE = 0,
    FLOOR = 1 << 0,
    KNEE = 1 << 1,
    WAIST = 1 << 2,
    CHEST = 1 << 3,
    HEAD = 1 << 4,
    POWER = 1 << 5,
};

enum class TileCategory : uint8_t
{
    NONE,
    STRUCTURE,
    POWER,
    OXYGEN,
};

enum class ComponentType : uint8_t
{
    WALKABLE,
    SOLID,
    POWER_CONNECTOR,
    BATTERY,
    POWER_CONSUMER,
    POWER_PRODUCER,
    SOLAR_PANEL,
    OXYGEN,
    OXYGEN_PRODUCER,
    DOOR,
    DURABILITY,
    ROTATABLE,
    STRUCTURE,
};

enum class PowerPriority : uint8_t
{
    CRITICAL = 0, // Life support, etc.
    HIGH = 1,     // Main systems
    MEDIUM = 2,   // General equipment
    LOW = 3,      // Convenience, decoration
    OFFLINE = 255 // Manually turned off
};

template <>
struct magic_enum::customize::enum_range<TileHeight>
{
    static constexpr bool is_flags = true;
};
