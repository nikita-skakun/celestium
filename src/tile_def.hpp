#pragma once
#include "component.hpp"
#include "sprite.hpp"
#include <unordered_set>

struct TileDef
{
    enum class Height : uint8_t
    {
        NONE = 0,
        FLOOR = 1 << 0,
        KNEE = 1 << 1,
        WAIST = 1 << 2,
        CHEST = 1 << 3,
        HEAD = 1 << 4,
        POWER = 1 << 5,
    };

    enum class Category : uint8_t
    {
        NONE,
        STRUCTURE,
        POWER,
        OXYGEN,
    };

private:
    const std::string id;
    const Height height;
    const Category category;
    const std::unordered_set<std::shared_ptr<Component>> refComponents;
    const std::shared_ptr<SpriteDef> refSprite;
    const Vector2Int iconOffset;

public:
    TileDef(const std::string &id, Height height, Category category, const std::unordered_set<std::shared_ptr<Component>> &refComponents,
            const std::shared_ptr<SpriteDef> &refSprite, const Vector2Int &iconOffset)
        : id(id), height(height), category(category), refComponents(refComponents), refSprite(refSprite), iconOffset(iconOffset) {}

    constexpr const std::string &GetId() const { return id; }
    constexpr std::string GetName() const { return MacroCaseToName(id); }
    constexpr Height GetHeight() const { return height; }
    constexpr Category GetCategory() const { return category; }
    constexpr const std::unordered_set<std::shared_ptr<Component>> &GetReferenceComponents() const { return refComponents; }
    constexpr const std::shared_ptr<SpriteDef> &GetReferenceSprite() const { return refSprite; }
    constexpr const Vector2Int &GetIconOffset() const { return iconOffset; }
};

template <>
struct magic_enum::customize::enum_range<TileDef::Height>
{
    static constexpr bool is_flags = true;
};