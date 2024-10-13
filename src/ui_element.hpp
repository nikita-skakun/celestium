#pragma once
#include "utils.hpp"

struct UiElement
{
protected:
    Rectangle rect;
    bool enabled;

public:
    constexpr UiElement(const Rectangle &rect) : rect(rect), enabled(true) {};
    constexpr const Rectangle &GetRect() const { return rect; }
    constexpr void SetRect(const Rectangle &newRect) { rect = newRect; }
    constexpr bool IsEnabled() const { return enabled; }
    constexpr void SetEnabled(bool state) { enabled = state; }

    virtual void Render() = 0;
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