#pragma once
#include "tile_def.hpp"

class TileDefinitionRegistry
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

    void RegisterAllTileDefinitions()
    {
        tileDefinitions["BLUE_FLOOR"] = std::make_shared<TileDef>("BLUE_FLOOR", TileDef::Height::FLOOR, std::unordered_set<std::shared_ptr<Component>>{std::make_shared<WalkableComponent>(), std::make_shared<OxygenComponent>()});
        tileDefinitions["WALL"] = std::make_shared<TileDef>("WALL", TileDef::Height::FLOOR | TileDef::Height::WAIST | TileDef::Height::CEILING, std::unordered_set<std::shared_ptr<Component>>{std::make_shared<SolidComponent>()});
        tileDefinitions["OXYGEN_PRODUCER"] = std::make_shared<TileDef>("OXYGEN_PRODUCER", TileDef::Height::WAIST, std::unordered_set<std::shared_ptr<Component>>{std::make_shared<PowerConnectorComponent>(PowerConnectorComponent::IO::INPUT), std::make_shared<PowerConsumerComponent>(OxygenProducerComponent::POWER_CONSUMPTION), std::make_shared<OxygenProducerComponent>()});
        tileDefinitions["BATTERY"] = std::make_shared<TileDef>("BATTERY", TileDef::Height::WAIST, std::unordered_set<std::shared_ptr<Component>>{std::make_shared<PowerConnectorComponent>(PowerConnectorComponent::IO::INPUT | PowerConnectorComponent::IO::OUTPUT), std::make_shared<BatteryComponent>(2000.f)});
        tileDefinitions["SOLAR_PANEL"] = std::make_shared<TileDef>("SOLAR_PANEL", TileDef::Height::WAIST, std::unordered_set<std::shared_ptr<Component>>{std::make_shared<PowerConnectorComponent>(PowerConnectorComponent::IO::OUTPUT), std::make_shared<PowerProducerComponent>(SolarPanelComponent::SOLAR_PANEL_POWER_OUTPUT), std::make_shared<SolarPanelComponent>()});
        tileDefinitions["FRAME"] = std::make_shared<TileDef>("FRAME", TileDef::Height::FLOOR, std::unordered_set<std::shared_ptr<Component>>{std::make_shared<WalkableComponent>()});
    }
};
