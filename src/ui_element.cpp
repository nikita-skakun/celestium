#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "ui_element.hpp"

void UiToggle::Render()
{
    GuiSetState(IsEnabled() ? STATE_NORMAL : STATE_DISABLED);

    bool oldState = state;
    GuiToggle(rect, "", &state);
    if (oldState != state)
        onToggle(state);

    GuiEnable();
}

void UiIcon::Render()
{
    DrawTexturePro(spritesheet, spriteOutline, rect, Vector2(), 0, tint);
}

void UiButton::Render()
{
    GuiSetState(IsEnabled() ? STATE_NORMAL : STATE_DISABLED);
    GuiSetStyle(DEFAULT, TEXT_SIZE, fontSize);
    GuiSetFont(font);

    if (GuiButton(rect, text.c_str()))
        onPress();

    GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_FONT_SIZE);
    GuiSetFont(GetFontDefault());
    GuiEnable();
}

void UiPanel::Render()
{
    DrawRectangleRec(rect, backgroundColor);
}
