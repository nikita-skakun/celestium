#pragma once
#include "tile_enums.hpp"
#include "utils.hpp"
#include <unordered_set>

struct Component;
struct SpriteDef;

struct TileDef
{
private:
    const std::string id;
    const TileHeight height;
    const TileCategory category;
    const std::unordered_set<std::shared_ptr<Component>> refComponents;
    const std::shared_ptr<SpriteDef> refSprite;
    const Vector2Int iconOffset;
    const std::unordered_map<std::string, int> buildResources;

public:
    TileDef(const std::string &id, TileHeight height, TileCategory category, const std::unordered_set<std::shared_ptr<Component>> &refComponents,
            const std::shared_ptr<SpriteDef> &refSprite, const Vector2Int &iconOffset, const std::unordered_map<std::string, int> &buildResources)
        : id(id), height(height), category(category), refComponents(refComponents), refSprite(refSprite), iconOffset(iconOffset), buildResources(buildResources) {}

    constexpr const std::string &GetId() const { return id; }
    constexpr std::string GetName() const { return MacroCaseToName(id); }
    constexpr TileHeight GetHeight() const { return height; }
    constexpr TileCategory GetCategory() const { return category; }
    constexpr const std::unordered_set<std::shared_ptr<Component>> &GetReferenceComponents() const { return refComponents; }
    constexpr const std::shared_ptr<SpriteDef> &GetReferenceSprite() const { return refSprite; }
    constexpr const Vector2Int &GetIconOffset() const { return iconOffset; }
    constexpr const std::unordered_map<std::string, int> &GetBuildResources() const { return buildResources; }
};