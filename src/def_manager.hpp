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
private:
    std::unordered_map<std::string, std::shared_ptr<TileDef>> tileDefinitions;
    std::unordered_map<std::string, std::shared_ptr<EffectDef>> effectDefinitions;

    DefinitionManager() = default;
    ~DefinitionManager() = default;
    DefinitionManager(const DefinitionManager &) = delete;
    DefinitionManager &operator=(const DefinitionManager &) = delete;

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

    std::optional<PowerConsumerComponent::PowerPriority> ParsePowerPriority(const ryml::ConstNodeRef &node)
    {
        std::string powerPriorityStr;
        node["powerPriority"] >> powerPriorityStr;
        StringRemoveSpaces(powerPriorityStr);

        if (powerPriorityStr.empty())
            return std::nullopt;

        auto powerPriority = magic_enum::enum_cast<PowerConsumerComponent::PowerPriority>(powerPriorityStr, magic_enum::case_insensitive);
        if (!powerPriority.has_value())
            throw std::runtime_error(std::format("Parsing of power priority string failed: {}", powerPriorityStr));

        return powerPriority;
    }

    std::shared_ptr<Component> CreateComponent(Component::Type type, const ryml::ConstNodeRef &node)
    {
        switch (type)
        {
        case Component::Type::WALKABLE:
            return std::make_shared<WalkableComponent>();

        case Component::Type::SOLID:
            return std::make_shared<SolidComponent>();

        case Component::Type::POWER_CONNECTOR:
            return std::make_shared<PowerConnectorComponent>();

        case Component::Type::BATTERY:
            return std::make_shared<BatteryComponent>(GetValue(node, "maxCharge", 0.f));

        case Component::Type::POWER_CONSUMER:
            if (auto powerPriority = ParsePowerPriority(node); powerPriority.has_value())
                return std::make_shared<PowerConsumerComponent>(GetValue(node, "powerConsumption", 0.f), powerPriority.value());
            return nullptr;

        case Component::Type::POWER_PRODUCER:
            return std::make_shared<PowerProducerComponent>(GetValue(node, "powerProduction", 0.f));

        case Component::Type::SOLAR_PANEL:
            return std::make_shared<SolarPanelComponent>();

        case Component::Type::OXYGEN:
            return std::make_shared<OxygenComponent>(GetValue(node, "oxygenLevel", 100.f));

        case Component::Type::OXYGEN_PRODUCER:
            return std::make_shared<OxygenProducerComponent>(GetValue(node, "oxygenProduction", 0.f));

        case Component::Type::DECORATIVE:
            return std::make_shared<DecorativeComponent>();

        case Component::Type::DOOR:
            return std::make_shared<DoorComponent>(GetValue(node, "movingSpeed", 0.f));

        case Component::Type::DURABILITY:
            return std::make_shared<DurabilityComponent>(GetValue(node, "hitpoints", 1.f));

        case Component::Type::ROTATABLE:
            return std::make_shared<RotatableComponent>();

        case Component::Type::STRUCTURE:
            return std::make_shared<StructureComponent>();

        default:
            throw std::runtime_error(std::format("Parsing of component type failed: {}", magic_enum::enum_name(type)));
        }
    }

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

    static void ParseEffectsFromFile(const std::string &filename)
    {
        std::vector<char> contents = ReadFromFile<std::vector<char>>(filename);
        ryml::Tree tree = ryml::parse_in_place(ryml::to_substr(contents));

        if (tree.empty())
            throw std::runtime_error(std::format("The definition file is empty or unreadable: {}", filename));

        for (ryml::ConstNodeRef effectNode : tree["env_effects"])
        {
            // Retrieve the effect ID string
            std::string effectId;
            effectNode["id"] >> effectId;
            StringRemoveSpaces(effectId);

            if (effectId.empty())
                throw std::runtime_error(std::format("Parsing of effect ID string failed: {}", effectId));

            // Retrieve the effect spritesheet string
            std::string spritesheet;
            effectNode["spritesheet"] >> spritesheet;
            StringRemoveSpaces(spritesheet);

            if (spritesheet.empty())
                throw std::runtime_error(std::format("Parsing of effect spritesheet string failed: {}", spritesheet));

            uint16_t sizeIncrements = GetValue<uint16_t>(effectNode, "sizeIncrements", 1);
            uint16_t spriteCount = GetValue<uint16_t>(effectNode, "spriteCount", 1);
            float animationSpeed = GetValue<float>(effectNode, "animationSpeed", 0);

            DefinitionManager::GetInstance().effectDefinitions[effectId] = std::make_shared<EffectDef>(effectId, spritesheet, sizeIncrements, spriteCount, animationSpeed);
        }
    }

    static void ParseTilesFromFile(const std::string &filename)
    {
        std::vector<char> contents = ReadFromFile<std::vector<char>>(filename);
        ryml::Tree tree = ryml::parse_in_place(ryml::to_substr(contents));

        if (tree.empty())
            throw std::runtime_error(std::format("The definition file is empty or unreadable: {}", filename));

        for (ryml::ConstNodeRef tileNode : tree["tiles"])
        {
            // Retrieve the tile ID string
            std::string tileId;
            tileNode["id"] >> tileId;
            StringRemoveSpaces(tileId);

            if (tileId.empty())
                throw std::runtime_error(std::format("Parsing of tile ID string failed: {}", tileId));

            // Parse Height
            std::string heightStr;
            tileNode["height"] >> heightStr;
            StringRemoveSpaces(heightStr);
            auto height = magic_enum::enum_flags_cast<TileDef::Height>(heightStr, magic_enum::case_insensitive);
            if (!height.has_value())
                throw std::runtime_error(std::format("Parsing of height string for tile ({}) failed: {}", tileId, heightStr));

            // Parse Category
            std::string categoryStr;
            tileNode["category"] >> categoryStr;
            StringRemoveSpaces(categoryStr);
            auto category = magic_enum::enum_cast<TileDef::Category>(categoryStr, magic_enum::case_insensitive);
            if (!category.has_value())
                throw std::runtime_error(std::format("Parsing of category string for tile ({}) failed: {}", tileId, categoryStr));

            // Parse Components
            std::unordered_set<std::shared_ptr<Component>> refComponents;
            for (ryml::ConstNodeRef node : tileNode["components"])
            {
                std::string typeStr;
                node["type"] >> typeStr;
                StringRemoveSpaces(typeStr);

                auto type = magic_enum::enum_cast<Component::Type>(typeStr, magic_enum::case_insensitive);
                if (!type.has_value())
                    throw std::runtime_error(std::format("Parsing of component type string for tile ({}) failed: {}", tileId, typeStr));

                auto component = DefinitionManager::GetInstance().CreateComponent(type.value(), node);
                if (!component)
                    throw std::runtime_error(std::format("Parsing of component string failed: {}", ryml::emitrs_yaml<std::string>(node)));
                refComponents.insert(component);
            }

            // Parse Sprites
            std::shared_ptr<SpriteDef> refSprite;
            if (tileNode.has_child("sprite"))
            {
                Vector2Int sprite;
                tileNode["sprite"] >> sprite;

                refSprite = std::make_shared<BasicSpriteDef>(sprite);
            }
            else if (tileNode.has_child("slicedSprite"))
            {
                std::vector<SliceWithConditions> slices;
                for (ryml::ConstNodeRef sliceNode : tileNode["slicedSprite"])
                {
                    std::string conditionsStr;
                    sliceNode["conditions"] >> conditionsStr;
                    StringRemoveSpaces(conditionsStr);

                    SpriteCondition conditions;
                    if (conditionsStr.empty() || conditionsStr == "NONE")
                        conditions = SpriteCondition::NONE;
                    else
                    {
                        auto parsedConditions = magic_enum::enum_flags_cast<SpriteCondition>(conditionsStr, magic_enum::case_insensitive);
                        if (!parsedConditions.has_value())
                            throw std::runtime_error(std::format("Parsing of conditions string for tile ({}) failed: {}", tileId, conditionsStr));
                        conditions = parsedConditions.value();
                    }

                    Rectangle sourceRect;
                    sliceNode["sourceRect"] >> sourceRect;

                    Vector2 destOffset;
                    sliceNode["destOffset"] >> destOffset;

                    slices.emplace_back(conditions, SpriteSlice(sourceRect, destOffset));
                }

                refSprite = std::make_shared<MultiSliceSpriteDef>(slices);
            }

            // Parse Icon Offset
            Vector2Int iconOffset;
            if (tileNode.has_child("icon"))
                tileNode["icon"] >> iconOffset;
            else
                iconOffset = Vector2Int(0, 0);

            DefinitionManager::GetInstance().tileDefinitions[tileId] = std::make_shared<TileDef>(tileId, height.value(), category.value(), refComponents, refSprite, iconOffset);
        }
    }
};
