#include "game_state.hpp"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "ui_element.hpp"

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
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, textAttrs.horizontalAlignment);
    GuiSetState(IsEnabled() ? STATE_NORMAL : STATE_DISABLED);
    GuiSetStyle(DEFAULT, TEXT_SIZE, textAttrs.fontSize);
    GuiSetFont(textAttrs.font);

    if (GuiButton(GetRect(), text.c_str()) && onPress)
        onPress();

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_FONT_SIZE);
    GuiSetFont(AssetManager::GetFont("DEFAULT"));
    GuiEnable();
}

void UiPanel::Render()
{
    DrawRectangleRec(GetRect(), backgroundColor);
}

void UiStatusBar::Render()
{
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, textAttrs.horizontalAlignment);
    GuiSetStyle(DEFAULT, TEXT_SIZE, textAttrs.fontSize);
    GuiSetFont(textAttrs.font);

    GuiStatusBar(GetRect(), text.c_str());

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_FONT_SIZE);
    GuiSetFont(AssetManager::GetFont("DEFAULT"));
}

void UiComboBox::Render()
{
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, textAttrs.horizontalAlignment);
    GuiSetState(IsEnabled() ? STATE_NORMAL : STATE_DISABLED);
    GuiSetStyle(DEFAULT, TEXT_SIZE, textAttrs.fontSize);
    GuiSetFont(textAttrs.font);

    int oldState = state;
    GuiComboBox(GetRect(), text.c_str(), &state);
    if (oldState != state && onPress)
        onPress(state);

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_FONT_SIZE);
    GuiSetFont(AssetManager::GetFont("DEFAULT"));
    GuiEnable();
}
