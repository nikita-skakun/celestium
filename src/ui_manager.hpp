#pragma once
#include "ui_element.hpp"
#include <unordered_map>

struct UiManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<UiElement>> uiElements;

    UiManager() = default;
    ~UiManager() = default;
    UiManager(const UiManager &) = delete;
    UiManager &operator=(const UiManager &) = delete;

public:
    static UiManager &GetInstance()
    {
        static UiManager instance;
        return instance;
    }

    void AddElement(const std::string &key, std::shared_ptr<UiElement> element)
    {
        uiElements.emplace(key, element);
    }

    std::shared_ptr<UiElement> GetElement(const std::string &key) const
    {
        return uiElements.at(key);
    }

    void Render() const
    {
        for (const auto &pair : uiElements)
        {
            const auto &element = pair.second;
            if (element->IsVisible())
            {
                element->Render();
                element->RenderChildren();
            }
        }
    }

    constexpr void ClearAllElements()
    {
        uiElements.clear();
    }
};