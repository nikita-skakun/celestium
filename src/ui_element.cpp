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
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, textAttrs.horizontalAlignment);
    GuiSetState(IsEnabled() ? STATE_NORMAL : STATE_DISABLED);
    GuiSetStyle(DEFAULT, TEXT_SIZE, textAttrs.fontSize);
    GuiSetFont(textAttrs.font);

    if (GuiButton(rect, text.c_str()))
        onPress();

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_FONT_SIZE);
    GuiSetFont(GetFontDefault());
    GuiEnable();
}

void UiPanel::Render()
{
    DrawRectangleRec(rect, backgroundColor);
}

void UiStatusBar::Render()
{
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, textAttrs.horizontalAlignment);
    GuiSetStyle(DEFAULT, TEXT_SIZE, textAttrs.fontSize);
    GuiSetFont(textAttrs.font);

    GuiStatusBar(rect, text.c_str());

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_FONT_SIZE);
    GuiSetFont(GetFontDefault());
}

void UiComboBox::Render()
{
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, textAttrs.horizontalAlignment);
    GuiSetState(IsEnabled() ? STATE_NORMAL : STATE_DISABLED);
    GuiSetStyle(DEFAULT, TEXT_SIZE, textAttrs.fontSize);
    GuiSetFont(textAttrs.font);

    int oldState = state;
    GuiComboBox(rect, text.c_str(), &state);
    if (oldState != state)
        onPress(state);

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_FONT_SIZE);
    GuiSetFont(GetFontDefault());
    GuiEnable();
}
