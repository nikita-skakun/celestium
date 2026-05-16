#include "audio_manager.hpp"
#include "camera.hpp"
#include "def_manager.hpp"
#include "game_state.hpp"
#include "tile_def.hpp"
#include "ui_element.hpp"
#include "ui_manager.hpp"
#include <raygui.h>

namespace
{
    void ToggleBuildCategory(TileCategory category);
    void InitializeCategorySpecificMenu();

    void LinkToggle(std::shared_ptr<UiToggle> toggle, std::function<bool()> stateFunc)
    {
        std::weak_ptr<UiToggle> weak = toggle;
        toggle->SetOnUpdate([weak, stateFunc]()
                            { if (auto t = weak.lock()) t->SetToggle(stateFunc()); });
    }

    void LinkVisibility(std::shared_ptr<UiElement> element, std::function<bool()> visibilityFunc)
    {
        std::weak_ptr<UiElement> weak = element;
        element->SetOnUpdate([weak, visibilityFunc]()
                             { if (auto e = weak.lock()) e->SetVisible(visibilityFunc()); });
    }

    void LinkToggleWithVisibility(std::shared_ptr<UiToggle> toggle, std::function<bool()> stateFunc, std::function<bool()> visibilityFunc)
    {
        std::weak_ptr<UiToggle> weak = toggle;
        toggle->SetOnUpdate([weak, stateFunc, visibilityFunc]()
                            { if (auto t = weak.lock()) { t->SetVisible(visibilityFunc()); t->SetToggle(stateFunc()); } });
    }

    std::shared_ptr<UiToggle> AddIconButton(std::shared_ptr<UiPanel> parent, Rectangle rect, const std::string &id,
                                            std::function<bool()> stateFunc, std::function<void(bool)> onToggle,
                                            const std::string &iconSheet = "", Rectangle iconSource = {0, 0, 0, 0}, Color iconTint = WHITE,
                                            std::function<bool()> visibilityFunc = nullptr)
    {
        auto toggle = std::make_shared<UiToggle>(rect, stateFunc(), onToggle);
        if (visibilityFunc)
            LinkToggleWithVisibility(toggle, stateFunc, visibilityFunc);
        else
            LinkToggle(toggle, stateFunc);

        if (!iconSheet.empty())
        {
            Vector2 size = RectToSize(rect);
            Vector2 iconSize = size * 0.75f;
            Vector2 iconPos = RectToPos(rect) + (size - iconSize) / 2.0f;
            toggle->AddChild(std::make_shared<UiIcon>(Vector2ToRect(iconPos, iconSize), iconSheet, iconSource, iconTint));
        }

        if (parent)
            parent->AddChild(toggle);
        if (!id.empty())
            UiManager::AddElement(id, toggle);
        return toggle;
    }

    void AddButtonList(std::shared_ptr<UiPanel> panel, Vector2 pos, float width, float height, float spacing,
                       const std::vector<std::pair<std::string, std::function<void()>>> &buttons)
    {
        for (size_t i = 0; i < buttons.size(); ++i)
        {
            Rectangle rect = {pos.x, pos.y + i * (height + spacing), width, height};
            panel->AddChild(std::make_shared<UiButton>(rect, buttons[i].first, buttons[i].second));
        }
    }

    void AddVolumeSlider(std::shared_ptr<UiPanel> parent, Rectangle textRect, Rectangle sliderRect, const std::string &label, std::function<float()> getVolume, std::function<void(float)> setVolume)
    {
        parent->AddChild(std::make_shared<UiStatusBar>(textRect, label));
        auto slider = std::make_shared<UiSlider>(sliderRect, getVolume(), 0, 1, setVolume);
        std::weak_ptr<UiSlider> weak = slider;
        slider->SetOnUpdate([weak, getVolume]()
                            { if (auto s = weak.lock()) s->SetValue(getVolume()); });
        parent->AddChild(slider);
    }

    void InitializeSettingsMenu()
    {
        const Vector2 size = Vector2(1, 1) * 2. / 3.;
        const Vector2 pos = Vector2(.5, .5) - size / 2.;
        const Vector2 spacing = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
        const float height = 1. / 36.;
        const float halfWidth = size.x / 2. - spacing.x * 1.5;

        auto settingsMenu = std::make_shared<UiPanel>(Rectangle(0, 0, 1, 1), Fade(BLACK, .2));
        LinkVisibility(settingsMenu, []()
                       { return GameManager::GetCamera().IsUiState(PlayerCam::UiState::SETTINGS_MENU); });
        UiManager::AddElement("SETTINGS_MENU", settingsMenu);

        auto menuBg = std::make_shared<UiPanel>(Vector2ToRect(pos, size), Fade(BLACK, .5));
        settingsMenu->AddChild(menuBg);

        float currentY = pos.y + spacing.y;
        auto getRect = [&](bool right)
        { return Rectangle(pos.x + spacing.x + (right ? halfWidth + spacing.x : 0), currentY, halfWidth, height); };

        // Monitor & FPS
        menuBg->AddChild(std::make_shared<UiStatusBar>(getRect(false), "Render Monitor:"));
        std::string monitorNames;
        for (int i = 0; i < GetMonitorCount(); i++)
            monitorNames += (i > 0 ? ";" : "") + std::string(GetMonitorName(i));
        auto monitorSelect = std::make_shared<UiComboBox>(getRect(true), monitorNames, GetCurrentMonitor());
        menuBg->AddChild(monitorSelect);
        currentY += height + spacing.y;

        menuBg->AddChild(std::make_shared<UiStatusBar>(getRect(false), "Monitor FPS:"));
        auto fpsSelect = std::make_shared<UiComboBox>(getRect(true), GameManager::GetCamera().GetFpsOptions(), GameManager::GetCamera().GetFpsIndex(),
                                                      [](int idx)
                                                      { GameManager::GetCamera().SetFpsIndex(idx); });
        menuBg->AddChild(fpsSelect);

        std::weak_ptr<UiComboBox> weakFps = fpsSelect;
        monitorSelect->SetOnPress([weakFps](int monitor)
                                  {
            SetWindowMonitor(monitor);
            GameManager::GetCamera().SetFps(GetMonitorRefreshRate(monitor));
            if (auto f = weakFps.lock()) { f->SetText(GameManager::GetCamera().GetFpsOptions()); f->SetState(GameManager::GetCamera().GetFpsIndex()); } });
        currentY += height + spacing.y;

        // Volume Sliders
        auto addSlider = [&](const std::string &label, auto get, auto set)
        {
            AddVolumeSlider(menuBg, getRect(false), getRect(true), label, get, set);
            currentY += height + spacing.y;
        };
        addSlider("Master Volume:", &AudioManager::GetMasterVolume, &AudioManager::SetMasterVolume);
        addSlider("Music Volume:", &AudioManager::GetMusicVolume, &AudioManager::SetMusicVolume);
        addSlider("Effects Volume:", &AudioManager::GetEffectsVolume, &AudioManager::SetEffectsVolume);
    }

    struct TileToggleConfig
    {
        std::string tileId;
        std::string iconSheet;
        Vector2Int iconOffset;
    };

    void AddBuildToggle(std::shared_ptr<UiPanel> panel, const TileToggleConfig &cfg, int idx)
    {
        const Vector2 SPACING = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
        const Vector2 SIZE = Vector2ScreenScale(Vector2(64, 64));
        const Vector2 PANEL_POS = Vector2(.5 - .8 / 2., 1. - (SIZE.y + SPACING.y * 2.) - SPACING.y * 4. - SIZE.y);
        Vector2 pos = PANEL_POS + Vector2(SPACING.x + idx * (SIZE.x + SPACING.x), SPACING.y);
        const std::string tileId = cfg.tileId;
        const std::string iconSheet = cfg.iconSheet;
        const Vector2Int iconOffset = cfg.iconOffset;

        AddIconButton(panel, Vector2ToRect(pos, SIZE), "", [tileId]()
                      { return GameManager::GetBuildTileId() == tileId; }, [tileId](bool s)
                      { GameManager::SetBuildTileId(s ? tileId : ""); if (s) GameManager::SetCancelMode(false); }, iconSheet, Vector2ToRect(ToVector2(iconOffset), {1, 1}) * TILE_SIZE);
    }

    void InitializeCategorySpecificMenu()
    {
        TileCategory selectedCategory = GameManager::GetSelectedCategory();
        const auto &tileDefs = DefinitionManager::GetTileDefinitions();

        int index = 0;
        for (const auto &pair : tileDefs)
        {
            const auto &tileDef = pair.second;
            if (tileDef->GetCategory() == selectedCategory)
            {
                TileToggleConfig config;
                config.tileId = tileDef->GetId();
                config.iconSheet = "STATION";
                config.iconOffset = tileDef->GetIconOffset();

                AddBuildToggle(std::dynamic_pointer_cast<UiPanel>(UiManager::GetElement("BUILD_MENU")), config, index);
                index++;
            }
        }
    }

    void ToggleBuildCategory(TileCategory category)
    {
        if (auto buildMenu = std::dynamic_pointer_cast<UiPanel>(UiManager::GetElement("BUILD_MENU")))
        {
            buildMenu->ClearChildren();
        }

        GameManager::ToggleSelectedCategory(category);
        InitializeCategorySpecificMenu();
    }
}

void UiManager::Update()
{
    for (const auto &pair : UiManager::GetInstance().uiElements)
    {
        if (pair.second)
            pair.second->Update();
    }
}

void UiManager::Render()
{
    for (const auto &pair : UiManager::GetInstance().uiElements)
    {
        const auto &element = pair.second;
        if (!element || !element->IsVisible())
            continue;

        element->Render();
        element->RenderChildren();
    }
}

void UiManager::InitializeMainMenu()
{
    const Vector2 spacing = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    const float btnW = 1. / 6., btnH = 1. / 18.;
    const Vector2 menuSize = Vector2(btnW, 4 * btnH + 3 * spacing.y) + spacing * 2;
    const Vector2 menuPos = Vector2(1. - menuSize.x - spacing.x, (1. - menuSize.y) / 2.);

    auto panel = std::make_shared<UiPanel>(Vector2ToRect(menuPos, menuSize), Fade(BLACK, .5));
    LinkVisibility(panel, []()
                   { return GameManager::GetCamera().IsUiClear(); });

    AddButtonList(panel, menuPos + spacing, btnW, btnH, spacing.y, {{"New Game", []()
                                                                     { GameManager::RequestStateChange(GameState::GAME_SIM); }},
                                                                    {"Load Game", []() {}},
                                                                    {"Settings", []()
                                                                     { GameManager::GetCamera().SetUiState(PlayerCam::UiState::SETTINGS_MENU); }},
                                                                    {"Exit", []()
                                                                     { GameManager::RequestStateChange(GameState::NONE); }}});

    UiManager::AddElement("MAIN_MENU", panel);
    InitializeSettingsMenu();
}

void UiManager::InitializeGameSim()
{
    const Vector2 largeSize = Vector2ScreenScale(Vector2(64, 64)), smallSize = largeSize / 2.;
    const Vector2 spacing = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    const Color buttonTextColor = GetColor(GuiGetStyle(BUTTON, TEXT_COLOR_NORMAL));

    // Sidebar & Overlays
    AddIconButton(nullptr, Vector2ToRect({1.f - spacing.x - largeSize.x, (1.f - largeSize.y) / 2.f}, largeSize), "BUILD_TGL", []()
                  { return GameManager::IsInBuildMode(); }, [](bool s)
                  { GameManager::SetBuildModeState(s); }, "ICON", {TILE_SIZE, TILE_SIZE, TILE_SIZE, TILE_SIZE}, buttonTextColor, []()
                  { return GameManager::GetCamera().IsUiClear(); });

    Vector2 overlayPos = {1.f - spacing.x - smallSize.x, (1.f + largeSize.y) / 2.f + spacing.y};
    auto addOverlayButton = [&](PlayerCam::Overlay overlay, const char *id, Vector2Int iconOffset)
    {
        AddIconButton(nullptr, Vector2ToRect(overlayPos, smallSize), id, [overlay]()
                      { return GameManager::GetCamera().IsOverlay(overlay); }, [overlay](bool)
                      { GameManager::GetCamera().ToggleOverlay(overlay); }, "ICON", Rectangle((float)iconOffset.x, (float)iconOffset.y, 1, 1) * TILE_SIZE, buttonTextColor, []()
                      { return GameManager::GetCamera().IsUiClear(); });
        overlayPos.y += smallSize.y + spacing.y;
    };
    addOverlayButton(PlayerCam::Overlay::OXYGEN, "OVERLAY_OXYGEN_TGL", {0, 0});
    addOverlayButton(PlayerCam::Overlay::WALL, "OVERLAY_WALL_TGL", {1, 0});
    addOverlayButton(PlayerCam::Overlay::POWER, "OVERLAY_POWER_TGL", {2, 0});

    // Escape Menu
    const float escW = 1. / 12., escH = 1. / 24.;
    const Vector2 escSize = Vector2(escW, 4 * escH + 3 * spacing.y) + spacing * 2;
    const Vector2 escPos = Vector2(.5, .5) - escSize / 2.;
    auto escMenu = std::make_shared<UiPanel>(Rectangle(0, 0, 1, 1), Fade(BLACK, .2));
    LinkVisibility(escMenu, []()
                   { return GameManager::GetCamera().IsUiState(PlayerCam::UiState::ESC_MENU); });
    auto escBg = std::make_shared<UiPanel>(Vector2ToRect(escPos, escSize), Fade(BLACK, .5));
    escMenu->AddChild(escBg);
    AddButtonList(escBg, escPos + spacing, escW, escH, spacing.y, {{"Resume", []()
                                                                    { GameManager::GetCamera().SetUiState(PlayerCam::UiState::NONE); }},
                                                                   {"Settings", []()
                                                                    { GameManager::GetCamera().SetUiState(PlayerCam::UiState::SETTINGS_MENU); }},
                                                                   {"Main Menu", []()
                                                                    { GameManager::RequestStateChange(GameState::MAIN_MENU); }},
                                                                   {"Exit", []()
                                                                    { GameManager::RequestStateChange(GameState::NONE); }}});
    UiManager::AddElement("ESC_MENU", escMenu);
    InitializeSettingsMenu();

    // Symmetry Buttons
    Vector2 panelPos = {.5f - .8f / 2.f, 1.f - (smallSize.y * 2 + spacing.y * 2) - spacing.y};
    Vector2 symPos = {panelPos.x - smallSize.x - spacing.x, 1.f - spacing.y * 1.5f - smallSize.y};
    auto addSym = [&](const std::string &id, auto get, auto tgl, Rectangle offset)
    {
        AddIconButton(nullptr, Vector2ToRect(symPos, smallSize), id, get, [tgl](bool)
                      { tgl(); }, "ICON", offset * TILE_SIZE, Fade(DARKGRAY, .8f), []()
                      { return GameManager::IsInBuildMode() && GameManager::GetCamera().IsUiClear(); });
        symPos.y -= smallSize.y + spacing.y;
    };
    addSym("BUILD_HOR_SYM_BTN", GameManager::IsHorizontalSymmetry, GameManager::ToggleHorizontalSymmetry, {6, 1, 1, 1});
    addSym("BUILD_VER_SYM_BTN", GameManager::IsVerticalSymmetry, GameManager::ToggleVerticalSymmetry, {5, 1, 1, 1});

    // Build Category Panel
    auto buildCatPanel = std::make_shared<UiPanel>(Vector2ToRect(panelPos, {.8f, largeSize.y + spacing.y * 2}), Fade(BLACK, .5f));
    LinkVisibility(buildCatPanel, []()
                   { return GameManager::GetCamera().IsUiClear() && GameManager::IsInBuildMode(); });
    UiManager::AddElement("BUILD_CATEGORY", buildCatPanel);

    Vector2 catPos = panelPos + spacing;
    auto addCategoryButton = [&](TileCategory category, Vector2Int iconOffset)
    {
        AddIconButton(buildCatPanel, Vector2ToRect(catPos, largeSize), "", [category]()
                      { return GameManager::GetSelectedCategory() == category; }, [category](bool)
                      { ToggleBuildCategory(category); }, "ICON", Rectangle((float)iconOffset.x, (float)iconOffset.y, 1, 1) * TILE_SIZE, buttonTextColor);
        catPos.x += largeSize.x + spacing.x * 2;
    };
    addCategoryButton(TileCategory::STRUCTURE, {1, 0});
    addCategoryButton(TileCategory::POWER, {2, 0});
    addCategoryButton(TileCategory::OXYGEN, {0, 0});
    AddIconButton(buildCatPanel, Vector2ToRect(catPos, largeSize), "", GameManager::IsInCancelMode, [](bool s)
                  { GameManager::SetCancelMode(s); if (s) GameManager::SetBuildTileId(""); }, "ICON", Rectangle{7, 1, 1, 1} * TILE_SIZE, buttonTextColor, []()
                  { return GameManager::IsInBuildMode() && GameManager::GetCamera().IsUiClear(); });

    // Build Menu Panel
    auto buildMenuPanel = std::make_shared<UiPanel>(Vector2ToRect({.5f - .8f / 2.f, 1.f - (largeSize.y + spacing.y * 2) - spacing.y * 4 - largeSize.y}, {.8f, largeSize.y + spacing.y * 2}), Fade(BLACK, .5f));
    LinkVisibility(buildMenuPanel, []()
                   { return GameManager::GetCamera().IsUiClear() && GameManager::IsInBuildMode() && GameManager::GetSelectedCategory() != TileCategory::NONE; });
    UiManager::AddElement("BUILD_MENU", buildMenuPanel);
}

std::shared_ptr<UiElement> UiManager::FindUiElementAtPos(const Vector2 &pos)
{
    for (const auto &pair : UiManager::GetInstance().uiElements)
    {
        const auto &element = pair.second;
        if (auto found = UiElement::FindChildAtPos(element, pos))
            return found;
    }

    return nullptr;
}
