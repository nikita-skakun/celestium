#pragma once
#include "utils.hpp"

struct UiElement
{
protected:
    Rectangle rect;
    bool enabled;
    bool visible;
    std::vector<std::shared_ptr<UiElement>> children;
    std::function<void()> onUpdate;

public:
    UiElement(const Rectangle &rect, std::function<void()> onUpdate = nullptr)
        : rect(rect), enabled(true), visible(true), onUpdate(onUpdate) {};

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

    void SetOnUpdate(std::function<void()> callback)
    {
        onUpdate = callback;
    }

    constexpr void Update()
    {
        if (onUpdate)
            onUpdate();

        for (const auto &child : children)
        {
            child->Update();
        }
    }
};

struct TextAttrs
{
    const Font &font;
    int fontSize;
    int horizontalAlignment;

    TextAttrs(const Font &font = GetFontDefault(), int fontSize = DEFAULT_FONT_SIZE, int horizontalAlignment = 1)
        : font(font), fontSize(fontSize), horizontalAlignment(horizontalAlignment) {}
};

struct UiToggle : UiElement
{
protected:
    bool state;
    std::function<void(bool)> onToggle;

public:
    UiToggle(const Rectangle &rect, bool startState, std::function<void(bool)> onToggle = nullptr, std::function<void()> onUpdate = nullptr)
        : UiElement(rect, onUpdate), state(startState), onToggle(onToggle) {}

    constexpr void SetToggle(bool newState) { state = newState; }

    void Render() override;
};

struct UiIcon : UiElement
{
protected:
    const Texture2D &spritesheet;
    Rectangle spriteOutline;
    Color tint;

public:
    UiIcon(const Rectangle &rect, const Texture2D &spritesheet, const Rectangle &spriteOutline, const Color &tint, std::function<void()> onUpdate = nullptr)
        : UiElement(rect, onUpdate), spritesheet(spritesheet), spriteOutline(spriteOutline), tint(tint) {}

    void Render() override;
};

struct UiButton : UiElement
{
protected:
    std::string text;
    std::function<void()> onPress;
    TextAttrs textAttrs;

public:
    UiButton(const Rectangle &rect, std::string text, std::function<void()> onPress = nullptr, const TextAttrs &textAttrs = TextAttrs(),
             std::function<void()> onUpdate = nullptr)
        : UiElement(rect, onUpdate), text(text), onPress(onPress), textAttrs(textAttrs) {}

    void Render() override;
};

struct UiPanel : UiElement
{
protected:
    Color backgroundColor;

public:
    UiPanel(const Rectangle &rect, Color backgroundColor, std::function<void()> onUpdate = nullptr)
        : UiElement(rect, onUpdate), backgroundColor(backgroundColor) {}

    void Render() override;
};

struct UiStatusBar : UiElement
{
protected:
    std::string text;
    TextAttrs textAttrs;

public:
    UiStatusBar(const Rectangle &rect, const std::string &text, const TextAttrs &textAttrs = TextAttrs(), std::function<void()> onUpdate = nullptr)
        : UiElement(rect, onUpdate), text(text), textAttrs(textAttrs) {}

    void Render() override;
};

struct UiComboBox : UiElement
{
protected:
    std::string text;
    int state;
    std::function<void(int)> onPress;
    TextAttrs textAttrs;

public:
    UiComboBox(const Rectangle &rect, std::string text, int startingState, std::function<void(int)> onPress = nullptr, const TextAttrs &textAttrs = TextAttrs(),
               std::function<void()> onUpdate = nullptr)
        : UiElement(rect, onUpdate), text(text), state(startingState), onPress(onPress), textAttrs(textAttrs) {}

    void Render() override;
};
