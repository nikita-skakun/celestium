#pragma once
#include "utils.hpp"

struct UiElement
{
protected:
    Rectangle rect;
    bool enabled;
    bool visible;
    std::vector<std::shared_ptr<UiElement>> children;

public:
    constexpr UiElement(const Rectangle &rect) : rect(rect), enabled(true), visible(true) {};
    constexpr const Rectangle &GetRect() const { return rect; }
    constexpr void SetRect(const Rectangle &newRect) { rect = newRect; }
    constexpr bool IsEnabled() const { return enabled; }
    constexpr void SetEnabled(bool state) { enabled = state; }
    constexpr bool IsVisible() const { return visible; }
    constexpr void SetVisibility(bool state) { visible = state; }

    virtual void Render() = 0;
    virtual ~UiElement() = default;

    void AddChild(const std::shared_ptr<UiElement> &child)
    {
        children.push_back(child);
    }

    constexpr void RenderChildren() const
    {
        for (const auto &child : children)
        {
            if (child->IsVisible())
                child->Render();
        }
    }
};

struct UiToggle : UiElement
{
protected:
    bool state;
    std::function<void(bool)> onToggle;

public:
    UiToggle(const Rectangle &rect, bool startState, std::function<void(bool)> callback = nullptr)
        : UiElement(rect), state(startState), onToggle(callback) {}

    void Render() override;
};

struct UiIcon : UiElement
{
protected:
    const Texture2D &spritesheet;
    Rectangle spriteOutline;
    Color tint;

public:
    UiIcon(const Rectangle &rect, const Texture2D &spritesheet, const Rectangle &spriteOutline, const Color &tint)
        : UiElement(rect), spritesheet(spritesheet), spriteOutline(spriteOutline), tint(tint) {}

    void Render() override;
};