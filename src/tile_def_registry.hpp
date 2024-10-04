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

public:
    static TileDefinitionRegistry &GetInstance()
    {
        static TileDefinitionRegistry instance;
        return instance;
    }

    std::shared_ptr<TileDef> GetTileDefinition(const std::string &name) const
    {
        return tileDefinitions.at(name);
    }

    std::shared_ptr<Component> CreateComponent(Component::Type type, const ryml::ConstNodeRef &componentNode)
    {
        switch (type)
        {
        case Component::Type::WALKABLE:
        {
            return std::make_shared<WalkableComponent>();
        }

        case Component::Type::SOLID:
        {
            return std::make_shared<SolidComponent>();
        }

        case Component::Type::POWER_CONNECTOR:
        {
            std::string ioStr;
            componentNode["io"] >> ioStr;

            // Remove spaces from IO string
            ioStr.erase(std::remove_if(ioStr.begin(), ioStr.end(), ::isspace), ioStr.end());

            auto io = magic_enum::enum_flags_cast<PowerConnectorComponent::IO>(ioStr);
            if (!io.has_value())
                LogMessage(LogLevel::ERROR, std::format("Parsing of IO string failed: {}", ioStr));

            return std::make_shared<PowerConnectorComponent>(io.value());
        }

        case Component::Type::BATTERY:
        {
            float maxCharge = 0.f;
            if (componentNode.has_child("maxCharge") && !componentNode["maxCharge"].val_is_null())
                componentNode["maxCharge"] >> maxCharge;
            return std::make_shared<BatteryComponent>(maxCharge);
        }

        case Component::Type::POWER_CONSUMER:
        {
            float powerConsumption = 0.f;
            if (componentNode.has_child("powerConsumption") && !componentNode["powerConsumption"].val_is_null())
                componentNode["powerConsumption"] >> powerConsumption;
            return std::make_shared<PowerConsumerComponent>(powerConsumption);
        }

        case Component::Type::POWER_PRODUCER:
        {
            float powerProduction = 0.f;
            if (componentNode.has_child("powerProduction") && !componentNode["powerProduction"].val_is_null())
                componentNode["powerProduction"] >> powerProduction;
            return std::make_shared<PowerProducerComponent>(powerProduction);
        }

        case Component::Type::SOLAR_PANEL:
        {
            return std::make_shared<SolarPanelComponent>();
        }

        case Component::Type::OXYGEN:
        {
            float oxygenLevel = 100.f;
            if (componentNode.has_child("oxygenLevel") && !componentNode["oxygenLevel"].val_is_null())
                componentNode["oxygenLevel"] >> oxygenLevel;
            return std::make_shared<OxygenComponent>(oxygenLevel);
        }

        case Component::Type::OXYGEN_PRODUCER:
        {
            float oxygenProduction = 0.f;
            if (componentNode.has_child("oxygenProduction") && !componentNode["oxygenProduction"].val_is_null())
                componentNode["oxygenProduction"] >> oxygenProduction;
            return std::make_shared<OxygenProducerComponent>(oxygenProduction);
        }

        case Component::Type::DECORATIVE:
        {
            return std::make_shared<DecorativeComponent>();
        }

        default:
            LogMessage(LogLevel::ERROR, std::format("Parsing of component type failed: {}", magic_enum::enum_name(type)));
            return nullptr;
        }

        return nullptr;
    }

    void ParseTilesFromFile(const std::string &filename)
    {
        std::vector<char> contents = ReadFromFile<std::vector<char>>(filename);
        ryml::Tree tree = ryml::parse_in_place(ryml::to_substr(contents));

        if (tree.empty())
            LogMessage(LogLevel::ERROR, std::format("The tile definition file is empty or unreadable: {}", filename));

        for (ryml::ConstNodeRef tileNode : tree["tiles"])
        {
            // Retrieve the tile ID string
            std::string tileId;
            tileNode["id"] >> tileId;

            // Remove spaces from tile ID string
            tileId.erase(std::remove_if(tileId.begin(), tileId.end(), ::isspace), tileId.end());
            if (tileId.empty())
                LogMessage(LogLevel::ERROR, std::format("Parsing of tile ID string failed: {}", tileId));

            // Retrieve the height string
            std::string heightStr;
            tileNode["height"] >> heightStr;

            // Remove spaces from height string
            heightStr.erase(std::remove_if(heightStr.begin(), heightStr.end(), ::isspace), heightStr.end());

            // Parse Height
            auto height = magic_enum::enum_flags_cast<TileDef::Height>(heightStr);
            if (!height.has_value())
                LogMessage(LogLevel::ERROR, std::format("Parsing of height string failed: {}", heightStr));

            // Parse Components
            std::unordered_set<std::shared_ptr<Component>> refComponents;
            for (ryml::ConstNodeRef componentNode : tileNode["components"])
            {
                std::string typeStr;
                componentNode["type"] >> typeStr;

                // Remove spaces from type string
                typeStr.erase(std::remove_if(typeStr.begin(), typeStr.end(), ::isspace), typeStr.end());

                auto type = magic_enum::enum_cast<Component::Type>(typeStr);
                if (!type.has_value())
                    LogMessage(LogLevel::ERROR, std::format("Parsing of component type string failed: {}", typeStr));

                auto component = CreateComponent(type.value(), componentNode);
                refComponents.insert(component);
            }

            TileDefinitionRegistry::GetInstance().tileDefinitions[tileId] = std::make_shared<TileDef>(tileId, height.value(), refComponents);
        }
    }
};
