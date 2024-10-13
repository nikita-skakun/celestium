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
