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
    };

private:
    const std::string id;
    const Height height;
    const std::unordered_set<std::shared_ptr<Component>> refComponents;
    const std::shared_ptr<SpriteDef> refSprite;

public:
    TileDef(const std::string &id, Height height, const std::unordered_set<std::shared_ptr<Component>> &refComponents,
            const std::shared_ptr<SpriteDef> &refSprite)
        : id(id), height(height), refComponents(refComponents), refSprite(refSprite) {}

    constexpr const std::string &GetId() const { return id; }
    constexpr Height GetHeight() const { return height; }
    constexpr const std::unordered_set<std::shared_ptr<Component>> &GetReferenceComponents() const { return refComponents; }
    constexpr const std::shared_ptr<SpriteDef> &GetReferenceSprite() const { return refSprite; }
};

template <>
struct magic_enum::customize::enum_range<TileDef::Height>
{
    static constexpr bool is_flags = true;
};