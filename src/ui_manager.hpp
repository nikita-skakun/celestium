#pragma once
#include "ui_element.hpp"
#include "game_state.hpp"
#include "camera.hpp"
#include <unordered_map>

struct UiManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<UiElement>> uiElements;

    UiManager() = default;
    ~UiManager() = default;
    UiManager(const UiManager &) = delete;
    UiManager &operator=(const UiManager &) = delete;

    static UiManager &GetInstance()
    {
        static UiManager instance;
        return instance;
    }

public:
    static void AddElement(const std::string &key, std::shared_ptr<UiElement> element)
    {
        UiManager::GetInstance().uiElements.emplace(key, element);
    }

    static std::shared_ptr<UiElement> GetElement(const std::string &key)
    {
        return UiManager::GetInstance().uiElements.at(key);
    }

    static void Update()
    {
        for (const auto &pair : UiManager::GetInstance().uiElements)
        {
            pair.second->Update();
        }
    }

    static void Render()
    {
        for (const auto &pair : UiManager::GetInstance().uiElements)
        {
            const auto &element = pair.second;
            if (element->IsVisible())
            {
                element->Render();
                element->RenderChildren();
            }
        }
    }

    static void ClearAllElements()
    {
        UiManager::GetInstance().uiElements.clear();
    }

    static void InitializeElements(PlayerCam &camera);
};