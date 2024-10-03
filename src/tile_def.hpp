#pragma once
#include "component.hpp"
#include <unordered_set>

struct TileDef
{
    enum class Height : u_int8_t
    {
        NONE = 0,
        FLOOR = 1 << 0,
        WAIST = 1 << 1,
        CEILING = 1 << 2,
    };

private:
    const std::string id;
    const Height height;
    const std::unordered_set<std::shared_ptr<Component>> refComponents;

public:
    TileDef(const std::string &id, Height height, std::unordered_set<std::shared_ptr<Component>> refComponents)
        : id(id), height(height), refComponents(refComponents) {}

    constexpr const std::string &GetId() const { return id; }
    constexpr Height GetHeight() const { return height; }
    constexpr const std::unordered_set<std::shared_ptr<Component>> &GetReferenceComponents() const { return refComponents; }
};

template <>
struct magic_enum::customize::enum_range<TileDef::Height>
{
    static constexpr bool is_flags = true;
};