#pragma once
#include "env_effect_def.hpp"
#include "fs_utils.hpp"
#include "tile_def.hpp"
#include <c4/format.hpp>
#include <ryml_std.hpp>
#include <ryml.hpp>

namespace c4
{
    inline bool from_chars(ryml::csubstr buf, Vector2 *vector)
    {
        size_t ret = ryml::unformat(buf, "({},{})", vector->x, vector->y);
        return ret != ryml::yml::npos;
    }

    inline bool from_chars(ryml::csubstr buf, Vector2Int *vector)
    {
        size_t ret = ryml::unformat(buf, "({},{})", vector->x, vector->y);
        return ret != ryml::yml::npos;
    }

    inline bool from_chars(ryml::csubstr buf, Rectangle *rect)
    {
        size_t ret = ryml::unformat(buf, "({},{},{},{})", rect->x, rect->y, rect->width, rect->height);
        return ret != ryml::yml::npos;
    }
}

struct DefinitionManager
{
protected:
    std::unordered_map<std::string, std::shared_ptr<TileDef>> tileDefinitions;
    std::unordered_map<std::string, std::shared_ptr<EffectDef>> effectDefinitions;

    DefinitionManager() = default;
    ~DefinitionManager() = default;
    DefinitionManager(const DefinitionManager &) = delete;
    DefinitionManager &operator=(const DefinitionManager &) = delete;

    static DefinitionManager &GetInstance()
    {
        static DefinitionManager instance;
        return instance;
    }

public:
    static const std::unordered_map<std::string, std::shared_ptr<TileDef>> &GetTileDefinitions()
    {
        return DefinitionManager::GetInstance().tileDefinitions;
    }

    static const std::shared_ptr<TileDef> &GetTileDefinition(const std::string &tileId)
    {
        return DefinitionManager::GetInstance().tileDefinitions.at(tileId);
    }

    static std::shared_ptr<EffectDef> GetEffectDefinition(const std::string &effectId)
    {
        return DefinitionManager::GetInstance().effectDefinitions.at(effectId);
    }

    // Resolve a slash-separated path (e.g. "ui/textColor") against a node and return
    // the child node reference. Throws if any path component is missing.
    static ryml::ConstNodeRef GetNodeByPath(const ryml::ConstNodeRef &root, const std::string &path)
    {
        ryml::ConstNodeRef cur = root;
        size_t start = 0;
        while (start < path.size())
        {
            size_t sep = path.find('/', start);
            std::string part = (sep == std::string::npos) ? path.substr(start) : path.substr(start, sep - start);
            ryml::csubstr part_cs(part.c_str(), part.size());
            if (!cur.has_child(part_cs))
                throw std::runtime_error(std::format("Missing required configuration key: {}", path));
            cur = cur[part_cs];
            start = (sep == std::string::npos) ? path.size() : sep + 1;
        }
        return cur;
    }

    template <typename T>
    static T GetRequiredValue(const ryml::ConstNodeRef &root, const std::string &key)
    {
        ryml::ConstNodeRef child = GetNodeByPath(root, key);
        if (child.val_is_null())
            throw std::runtime_error(std::format("Required configuration key is null: {}", key));
        T value;
        child >> value;
        return value;
    }

    template <typename T>
    static T GetValue(const ryml::ConstNodeRef &node, const ryml::csubstr &key, T defaultValue)
    {
        if (node.has_child(key) && !node[key].val_is_null())
        {
            T value;
            node[key] >> value;
            return value;
        }
        return defaultValue;
    }

    static void ParseTilesFromFile(const std::string &filename);
    static void ParseEffectsFromFile(const std::string &filename);
    static void ParseConstantsFromFile(const std::string &filename);
};
