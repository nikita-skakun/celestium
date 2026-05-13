#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#undef RAYGUI_IMPLEMENTATION

#include "game_state.hpp"
#include "ui_element.hpp"

namespace
{
    void ApplyTextAttributes(const TextAttrs &attrs, bool enabled)
    {
        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, attrs.horizontalAlignment);
        GuiSetState(enabled ? STATE_NORMAL : STATE_DISABLED);
        GuiSetStyle(DEFAULT, TEXT_SIZE, attrs.fontSize);
        GuiSetFont(attrs.font);
    }

    void ResetGuiStyle()
    {
        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_FONT_SIZE);
        GuiSetFont(AssetManager::GetFont("DEFAULT"));
        GuiEnable();
    }
}

Rectangle UiElement::GetRect() const
{
    return inWorldSpace ? GameManager::WorldToScreen(rect) : rect * GetScreenSize();
}

void UiToggle::Render()
{
    GuiSetState(IsEnabled() ? STATE_NORMAL : STATE_DISABLED);

    bool oldState = state;
    GuiToggle(GetRect(), "", &state);
    if (oldState != state && onToggle)
        onToggle(state);

    GuiEnable();
}

void UiIcon::Render()
{
    DrawTexturePro(AssetManager::GetTexture(spritesheetName), spriteOutline, GetRect(), Vector2(), 0, tint);
}

void UiButton::Render()
{
    ApplyTextAttributes(textAttrs, IsEnabled());

    if (GuiButton(GetRect(), text.c_str()) && onPress)
        onPress();

    ResetGuiStyle();
}

void UiPanel::Render()
{
    DrawRectangleRec(GetRect(), backgroundColor);
}

void UiStatusBar::Render()
{
    ApplyTextAttributes(textAttrs, IsEnabled());
    GuiStatusBar(GetRect(), text.c_str());
    ResetGuiStyle();
}

void UiComboBox::Render()
{
    ApplyTextAttributes(textAttrs, IsEnabled());

    int oldState = state;
    GuiComboBox(GetRect(), text.c_str(), &state);
    if (oldState != state && onPress)
        onPress(state);

    ResetGuiStyle();
}

void UiSlider::Render()
{
    GuiSetState(IsEnabled() ? STATE_NORMAL : STATE_DISABLED);

    float oldValue = value;
    GuiSlider(GetRect(), "", nullptr, &value, minValue, maxValue);
    if (oldValue != value && onSlide)
        onSlide(value);

    GuiEnable();
}
