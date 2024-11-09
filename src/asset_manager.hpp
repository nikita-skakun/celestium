#pragma once
#include "utils.hpp"
#include <unordered_map>

struct AssetManager
{
private:
    std::unordered_map<std::string, Texture2D> textures;
    std::unordered_map<std::string, Font> fonts;

    AssetManager() = default;
    ~AssetManager() = default;
    AssetManager(const AssetManager &) = delete;
    AssetManager &operator=(const AssetManager &) = delete;

    static AssetManager &GetInstance()
    {
        static AssetManager instance;
        return instance;
    }

public:
    static void Initialize()
    {
        auto &assetManager = AssetManager::GetInstance();

        assetManager.textures.emplace("STATION", LoadTexture("../assets/tilesets/station.png"));
        assetManager.textures.emplace("ICON", LoadTexture("../assets/tilesets/icons.png"));
        assetManager.textures.emplace("FIRE", LoadTexture("../assets/tilesets/fire.png"));
        assetManager.textures.emplace("FOAM", LoadTexture("../assets/tilesets/foam.png"));
        assetManager.fonts.emplace("DEFAULT", LoadFontEx("../assets/fonts/Jersey25.ttf", DEFAULT_FONT_SIZE, NULL, 0));
    }

    static const Texture2D &GetTexture(const std::string &key)
    {
        auto &assetManager = AssetManager::GetInstance();

        if (!assetManager.textures.contains(key))
            throw std::runtime_error(std::format("Texture [{}] is not loaded!", key));

        return assetManager.textures.at(key);
    }

    static const Font &GetFont(const std::string &key)
    {
        auto &assetManager = AssetManager::GetInstance();

        if (!assetManager.fonts.contains(key))
            throw std::runtime_error(std::format("Font [{}] is not loaded!", key));

        return assetManager.fonts.at(key);
    }

    static void CleanUp()
    {
        auto &assetManager = AssetManager::GetInstance();

        for (auto &texture : assetManager.textures)
        {
            UnloadTexture(texture.second);
        }

        for (auto &font : assetManager.fonts)
        {
            UnloadFont(font.second);
        }
    }
};