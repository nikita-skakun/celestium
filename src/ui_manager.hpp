#pragma once
#include "utils.hpp"
#include <unordered_map>

struct UiElement;

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
    static void AddElement(const std::string &key, const std::shared_ptr<UiElement> &element)
    {
        UiManager::GetInstance().uiElements.emplace(key, element);
    }

    static std::shared_ptr<UiElement> GetElement(const std::string &key)
    {
        return UiManager::GetInstance().uiElements.at(key);
    }

    static void Update();
    static void Render();

    static void ClearAllElements()
    {
        UiManager::GetInstance().uiElements.clear();
    }

    static void InitializeGameSim();

    static std::shared_ptr<UiElement> FindUiElementAtPos(const Vector2 &pos);

    static bool IsMouseOverUiElement()
    {
        return FindUiElementAtPos(GetMousePosition()) != nullptr;
    }
};