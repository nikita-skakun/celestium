#pragma once
#include "fs_utils.hpp"
#include "tile_def.hpp"
#include <ryml_std.hpp>
#include <ryml.hpp>

struct TileDefinitionRegistry
{
private:
    std::unordered_map<std::string, std::shared_ptr<TileDef>> tileDefinitions;

    TileDefinitionRegistry() = default;
    ~TileDefinitionRegistry() = default;
    TileDefinitionRegistry(const TileDefinitionRegistry &) = delete;
    TileDefinitionRegistry &operator=(const TileDefinitionRegistry &) = delete;

    template <typename T>
    T GetValue(const ryml::ConstNodeRef &node, const ryml::csubstr &key, T defaultValue)
    {
        if (node.has_child(key) && !node[key].val_is_null())
        {
            T value;
            node[key] >> value;
            return value;
        }
        return defaultValue;
    }

    std::optional<PowerConnectorComponent::IO> ParseIO(const ryml::ConstNodeRef &node)
    {
        std::string ioStr;
        node["io"] >> ioStr;
        StringRemoveSpaces(ioStr);

        auto io = magic_enum::enum_flags_cast<PowerConnectorComponent::IO>(ioStr, magic_enum::case_insensitive);
        if (!io.has_value())
            throw std::runtime_error(std::format("Parsing of IO string failed: {}", ioStr));
        return io;
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
        {
            auto io = ParseIO(node);
            if (!io.has_value())
                return nullptr;
            return std::make_shared<PowerConnectorComponent>(io.value());
        }

        case Component::Type::BATTERY:
            return std::make_shared<BatteryComponent>(GetValue(node, "maxCharge", 0.f));

        case Component::Type::POWER_CONSUMER:
            return std::make_shared<PowerConsumerComponent>(GetValue(node, "powerConsumption", 0.f));

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

        default:
            throw std::runtime_error(std::format("Parsing of component type failed: {}", magic_enum::enum_name(type)));
            return nullptr;
        }
    }

public:
    static TileDefinitionRegistry &GetInstance()
    {
        static TileDefinitionRegistry instance;
        return instance;
    }

    std::shared_ptr<TileDef> GetTileDefinition(const std::string &tileId) const
    {
        return tileDefinitions.at(tileId);
    }

    void ParseTilesFromFile(const std::string &filename)
    {
        std::vector<char> contents = ReadFromFile<std::vector<char>>(filename);
        ryml::Tree tree = ryml::parse_in_place(ryml::to_substr(contents));

        if (tree.empty())
            throw std::runtime_error(std::format("The tile definition file is empty or unreadable: {}", filename));

        for (ryml::ConstNodeRef tileNode : tree["tiles"])
        {
            // Retrieve the tile ID string
            std::string tileId;
            tileNode["id"] >> tileId;
            StringRemoveSpaces(tileId);

            if (tileId.empty())
                throw std::runtime_error(std::format("Parsing of tile ID string failed: {}", tileId));

            // Retrieve the height string
            std::string heightStr;
            tileNode["height"] >> heightStr;
            StringRemoveSpaces(heightStr);

            // Parse Height
            auto height = magic_enum::enum_flags_cast<TileDef::Height>(heightStr, magic_enum::case_insensitive);
            if (!height.has_value())
                throw std::runtime_error(std::format("Parsing of height string failed: {}", heightStr));

            // Parse Components
            std::unordered_set<std::shared_ptr<Component>> refComponents;
            for (ryml::ConstNodeRef node : tileNode["components"])
            {
                std::string typeStr;
                node["type"] >> typeStr;
                StringRemoveSpaces(typeStr);

                auto type = magic_enum::enum_cast<Component::Type>(typeStr, magic_enum::case_insensitive);
                if (!type.has_value())
                    throw std::runtime_error(std::format("Parsing of component type string failed: {}", typeStr));

                auto component = CreateComponent(type.value(), node);
                if (!component)
                    throw std::runtime_error(std::format("Parsing of component string failed: {}", ryml::emitrs_yaml<std::string>(node)));
                refComponents.insert(component);
            }

            TileDefinitionRegistry::GetInstance().tileDefinitions[tileId] = std::make_shared<TileDef>(tileId, height.value(), refComponents);
        }
    }
};
