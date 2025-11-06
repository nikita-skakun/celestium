#include "component.hpp"
#include "def_manager.hpp"
#include "sprite.hpp"
#include "tile.hpp"

namespace
{
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

    static std::shared_ptr<Component> CreateComponent(ComponentType type, const ryml::ConstNodeRef &node)
    {
        switch (type)
        {
        case ComponentType::WALKABLE:
            return std::make_shared<WalkableComponent>();

        case ComponentType::SOLID:
            return std::make_shared<SolidComponent>();

        case ComponentType::POWER_CONNECTOR:
            return std::make_shared<PowerConnectorComponent>();

        case ComponentType::BATTERY:
            return std::make_shared<BatteryComponent>(DefinitionManager::GetRequiredValue<float>(node, "maxCharge"));

        case ComponentType::POWER_CONSUMER:
            if (auto powerPriority = ParsePowerPriority(node); powerPriority.has_value())
                return std::make_shared<PowerConsumerComponent>(DefinitionManager::GetRequiredValue<float>(node, "powerConsumption"), powerPriority.value());
            return nullptr;

        case ComponentType::POWER_PRODUCER:
            return std::make_shared<PowerProducerComponent>(DefinitionManager::GetRequiredValue<float>(node, "powerProduction"));

        case ComponentType::SOLAR_PANEL:
            return std::make_shared<SolarPanelComponent>();

        case ComponentType::OXYGEN:
            return std::make_shared<OxygenComponent>(DefinitionManager::GetRequiredValue<float>(node, "oxygenLevel"));

        case ComponentType::OXYGEN_PRODUCER:
            return std::make_shared<OxygenProducerComponent>(DefinitionManager::GetRequiredValue<float>(node, "oxygenProduction"));

        case ComponentType::DECORATIVE:
            return std::make_shared<DecorativeComponent>();

        case ComponentType::DOOR:
            return std::make_shared<DoorComponent>(DefinitionManager::GetRequiredValue<float>(node, "movingSpeed"));

        case ComponentType::DURABILITY:
            return std::make_shared<DurabilityComponent>(DefinitionManager::GetRequiredValue<float>(node, "hitpoints"));

        case ComponentType::ROTATABLE:
            return std::make_shared<RotatableComponent>();

        case ComponentType::STRUCTURE:
            return std::make_shared<StructureComponent>();

        default:
            throw std::runtime_error(std::format("Parsing of component type failed: {}", magic_enum::enum_name(type)));
        }
    }

    static std::vector<SpriteCondition> ExpandUtilityConditions(const std::string &condStr)
    {
        std::string s = StringToMacroCase(condStr);

        std::vector<SpriteCondition> out;

        // Support CARDINAL_N_SAME where N is 0..4: exactly N of the cardinal neighbors
        // are SAME and the rest are DIFFERENT. This generates all bit combinations
        // of NORTH/EAST/SOUTH/WEST where the count of SAME equals N.
        if (s.rfind("CARDINAL_", 0) == 0 && s.size() > 9 && s.find("_SAME", 9) != std::string::npos)
        {
            // Extract the number between CARDINAL_ and _SAME
            size_t start = 9;
            size_t end = s.find("_SAME", start);
            std::string numStr = s.substr(start, end - start);
            int n = 0;
            try
            {
                n = std::stoi(numStr);
            }
            catch (...)
            {
                return out;
            }

            if (n < 0 || n > 4)
                return out;

            // Order: NORTH(0), EAST(1), SOUTH(2), WEST(3)
            const SpriteCondition sameFlags[4] = {
                SpriteCondition::NORTH_SAME,
                SpriteCondition::EAST_SAME,
                SpriteCondition::SOUTH_SAME,
                SpriteCondition::WEST_SAME,
            };
            const SpriteCondition diffFlags[4] = {
                SpriteCondition::NORTH_DIFFERENT,
                SpriteCondition::EAST_DIFFERENT,
                SpriteCondition::SOUTH_DIFFERENT,
                SpriteCondition::WEST_DIFFERENT,
            };

            // Iterate all 16 masks and pick those with exactly n bits set
            for (uint32_t mask = 0; mask < 16; ++mask)
            {
                int bits = __builtin_popcount(mask);
                if (bits != n)
                    continue;

                SpriteCondition combined = SpriteCondition::NONE;
                for (int i = 0; i < 4; ++i)
                {
                    if (mask & (1u << i))
                        combined = combined | sameFlags[i];
                    else
                        combined = combined | diffFlags[i];
                }
                out.push_back(combined);
            }

            return out;
        }

        // Unknown utility, return empty to indicate not handled
        return out;
    }
}

void DefinitionManager::ParseTilesFromFile(const std::string &filename)
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

            auto type = magic_enum::enum_cast<ComponentType>(typeStr, magic_enum::case_insensitive);
            if (!type.has_value())
                throw std::runtime_error(std::format("Parsing of component type string for tile ({}) failed: {}", tileId, typeStr));

            auto component = CreateComponent(type.value(), node);
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

                Rectangle sourceRect;
                sliceNode["sourceRect"] >> sourceRect;

                Vector2 destOffset;
                sliceNode["destOffset"] >> destOffset;

                if (conditionsStr.empty() || conditionsStr == "NONE")
                {
                    slices.emplace_back(SpriteCondition::NONE, SpriteSlice(sourceRect, destOffset));
                    continue;
                }

                std::vector<SpriteCondition> combos;
                combos.push_back(SpriteCondition::NONE);

                size_t start = 0;
                while (start < conditionsStr.size())
                {
                    size_t sep = conditionsStr.find('|', start);
                    std::string token = (sep == std::string::npos) ? conditionsStr.substr(start) : conditionsStr.substr(start, sep - start);
                    start = (sep == std::string::npos) ? conditionsStr.size() : sep + 1;

                    if (token.empty())
                        continue;

                    // Try parse token as enum flag
                    std::vector<SpriteCondition> tokenPossibilities;
                    auto parsed = magic_enum::enum_flags_cast<SpriteCondition>(token, magic_enum::case_insensitive);
                    if (parsed.has_value())
                    {
                        tokenPossibilities.push_back(parsed.value());
                    }
                    else
                    {
                        // Try utility expansion
                        auto expandedToken = ExpandUtilityConditions(token);
                        if (!expandedToken.empty())
                            tokenPossibilities.insert(tokenPossibilities.end(), expandedToken.begin(), expandedToken.end());
                    }

                    if (tokenPossibilities.empty())
                        throw std::runtime_error(std::format("Parsing of conditions string for tile ({}) failed: {}", tileId, token));

                    // Build new combos by OR-ing existing combos with each possibility
                    std::vector<SpriteCondition> newCombos;
                    for (auto &existing : combos)
                    {
                        for (auto &poss : tokenPossibilities)
                        {
                            SpriteCondition combined = existing | poss;
                            newCombos.push_back(combined);
                        }
                    }
                    combos.swap(newCombos);
                }

                // Add a slice for each resulting combination
                for (auto &cond : combos)
                    slices.emplace_back(cond, SpriteSlice(sourceRect, destOffset));
            }

            refSprite = std::make_shared<MultiSliceSpriteDef>(slices);
        }

        // Parse Icon Offset
        Vector2Int iconOffset;
        if (tileNode.has_child("icon"))
            tileNode["icon"] >> iconOffset;
        else
            iconOffset = Vector2Int(0, 0);

        // Parse Build Resources
        std::unordered_map<std::string, int> buildResources;
        if (tileNode.has_child("buildResources"))
        {
            for (ryml::ConstNodeRef resourceNode : tileNode["buildResources"])
            {
                std::string resourceId;
                resourceNode["id"] >> resourceId;
                StringRemoveSpaces(resourceId);

                int amount = DefinitionManager::GetRequiredValue<int>(resourceNode, "amount");

                buildResources[resourceId] = amount;
            }
        }

        DefinitionManager::GetInstance().tileDefinitions[tileId] = std::make_shared<TileDef>(tileId, height.value(), category.value(), refComponents, refSprite, iconOffset, buildResources);
    }
}

void DefinitionManager::ParseEffectsFromFile(const std::string &filename)
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

        uint16_t sizeIncrements = GetValue<uint16_t>(effectNode, "sizeIncrements", 1);

        std::vector<ParticleSystemDef> particleSystems;
        if (effectNode.has_child("particle_systems"))
        {
            for (ryml::ConstNodeRef psysNode : effectNode["particle_systems"])
            {
                std::string psysId;
                psysNode["id"] >> psysId;
                StringRemoveSpaces(psysId);
                std::string onCreateLua, onUpdateLua, onDeleteLua;
                if (psysNode.has_child("on_create"))
                    psysNode["on_create"] >> onCreateLua;
                if (psysNode.has_child("on_update"))
                    psysNode["on_update"] >> onUpdateLua;
                if (psysNode.has_child("on_delete"))
                    psysNode["on_delete"] >> onDeleteLua;
                particleSystems.emplace_back(psysId, onCreateLua, onUpdateLua, onDeleteLua);
            }
        }

        DefinitionManager::GetInstance().effectDefinitions[effectId] =
            std::make_shared<EffectDef>(effectId, sizeIncrements, particleSystems);
    }
}

void DefinitionManager::ParseConstantsFromFile(const std::string &filename)
{
    std::vector<char> contents = ReadFromFile<std::vector<char>>(filename);
    ryml::Tree tree = ryml::parse_in_place(ryml::to_substr(contents));

    if (tree.empty())
        throw std::runtime_error(std::format("The constants file is empty or unreadable: {}", filename));

    ryml::ConstNodeRef root = tree.rootref();

    // Helper to read color sequences [r,g,b,a] from a slash-separated path; throws if missing
    auto ReadColorAt = [&](const std::string &path)
    {
        ryml::ConstNodeRef colNode = GetNodeByPath(root, path);
        if (!colNode.is_seq())
            throw std::runtime_error(std::format("Expected color sequence at {}", path));
        int r = 0, g = 0, b = 0, a = 255;
        size_t idx = 0;
        for (ryml::ConstNodeRef child : colNode)
        {
            int v = 0;
            child >> v;
            if (idx == 0)
                r = v;
            else if (idx == 1)
                g = v;
            else if (idx == 2)
                b = v;
            else if (idx == 3)
                a = v;
            ++idx;
        }
        return Color{(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    };

    // general (required)
    FIXED_DELTA_TIME = GetRequiredValue<double>(root, "general/fixedDeltaTime");

    // fps (required)
    {
        ryml::ConstNodeRef fpsNode = GetNodeByPath(root, "fps/options");
        std::vector<uint16_t> tmp;
        fpsNode >> tmp;
        if (tmp.empty())
            throw std::runtime_error(std::format("fps/options must contain at least one entry"));
        FPS_OPTIONS = tmp;
    }

    // camera (required)
    MIN_ZOOM = GetRequiredValue<float>(root, "camera/minZoom");
    MAX_ZOOM = GetRequiredValue<float>(root, "camera/maxZoom");
    ZOOM_SPEED = GetRequiredValue<float>(root, "camera/zoomSpeed");
    CAMERA_KEY_MOVE_SPEED = GetRequiredValue<float>(root, "camera/keyMoveSpeed");

    // ui (required)
    DEFAULT_FONT_SIZE = GetRequiredValue<int>(root, "ui/defaultFontSize");
    DEFAULT_PADDING = GetRequiredValue<float>(root, "ui/defaultPadding");
    UI_TEXT_COLOR = ReadColorAt("ui/textColor");

    // tile (required)
    TILE_SIZE = GetRequiredValue<float>(root, "tile/size");
    TILE_OXYGEN_MAX = GetRequiredValue<float>(root, "tile/oxygenMax");
    GRID_COLOR = ReadColorAt("tile/gridColor");

    // oxygen (required)
    OXYGEN_DIFFUSION_RATE = GetRequiredValue<float>(root, "oxygen/diffusionRate");

    // outline (required)
    DRAG_THRESHOLD = GetRequiredValue<float>(root, "outline/dragThreshold");
    OUTLINE_SIZE = GetRequiredValue<float>(root, "outline/outlineSize");
    OUTLINE_COLOR = ReadColorAt("outline/outlineColor");

    // crew (required)
    CREW_RADIUS = GetRequiredValue<float>(root, "crew/radius");
    CREW_MOVE_SPEED = GetRequiredValue<float>(root, "crew/moveSpeed");
    CREW_OXYGEN_MAX = GetRequiredValue<float>(root, "crew/oxygenMax");
    CREW_OXYGEN_USE = GetRequiredValue<float>(root, "crew/oxygenUse");
    CREW_OXYGEN_REFILL = GetRequiredValue<float>(root, "crew/oxygenRefill");
    CREW_HEALTH_MAX = GetRequiredValue<float>(root, "crew/healthMax");
    CREW_EXTINGUISH_SPEED = GetRequiredValue<float>(root, "crew/extinguishSpeed");
    CREW_REPAIR_SPEED = GetRequiredValue<float>(root, "crew/repairSpeed");
    CREW_BUILD_SPEED = GetRequiredValue<float>(root, "crew/buildSpeed");
    CREW_DECONSTRUCT_EFFICIENCY = GetRequiredValue<float>(root, "crew/deconstructEfficiency");
}

void DefinitionManager::ParseResourcesFromFile(const std::string &filename)
{
    std::vector<char> contents = ReadFromFile<std::vector<char>>(filename);
    ryml::Tree tree = ryml::parse_in_place(ryml::to_substr(contents));

    if (tree.empty())
        throw std::runtime_error(std::format("The resource definition file is empty or unreadable: {}", filename));

    for (ryml::ConstNodeRef resourceNode : tree["resources"])
    {
        // Retrieve the resource ID string
        std::string resourceId;
        resourceNode["id"] >> resourceId;
        StringRemoveSpaces(resourceId);

        if (resourceId.empty())
            throw std::runtime_error(std::format("Parsing of resource ID string failed: {}", resourceId));

        // Parse Price
        float price = GetRequiredValue<float>(resourceNode, "price");

        DefinitionManager::GetInstance().resourceDefinitions[resourceId] = std::make_shared<ResourceDef>(resourceId, price);
    }
}
