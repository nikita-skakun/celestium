#include "ui_manager.hpp"
#include "tile.hpp"

void InitializeEscapeMenu()
{
    constexpr int buttonCount = 3;
    constexpr double buttonWidth = 1. / 12.;
    constexpr Vector2 spacing = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    constexpr double buttonHeight = 1. / 24.;
    constexpr double totalButtonHeight = buttonCount * buttonHeight + (buttonCount - 1) * spacing.y;

    // Calculate menu dimensions
    constexpr Vector2 menuSize = Vector2(buttonWidth, totalButtonHeight) + spacing * 2;

    constexpr Vector2 menuPos = Vector2(.5, .5) - menuSize / 2.;

    // Darken the background to draw focus to the UI
    auto escMenu = std::make_shared<UiPanel>(Rectangle(0, 0, 1, 1), Fade(BLACK, .2));
    std::weak_ptr<UiPanel> weakEscMenu = escMenu;
    escMenu->SetOnUpdate([weakEscMenu]()
                         { if (auto escMenu = weakEscMenu.lock())
                             { escMenu->SetVisible(GameManager::GetCamera().IsUiState(PlayerCam::UiState::ESC_MENU)); } });

    // Draw a rectangle for the menu background
    auto menuBackground = std::make_shared<UiPanel>(Vector2ToRect(menuPos, menuSize), Fade(BLACK, .5));
    escMenu->AddChild(menuBackground);

    // Dynamically position buttons in the center of the menu
    constexpr double firstButtonPosY = menuPos.y + (menuSize.y - totalButtonHeight) / 2.;
    constexpr double buttonPosX = .5 - buttonWidth / 2.;

    constexpr Rectangle resumeButtonRect = Rectangle(buttonPosX, firstButtonPosY, buttonWidth, buttonHeight);
    auto resumeButton = std::make_shared<UiButton>(resumeButtonRect, "Resume", []()
                                                   { GameManager::GetCamera().SetUiState(PlayerCam::UiState::NONE); });
    menuBackground->AddChild(resumeButton);

    constexpr Rectangle settingsButtonRect = Rectangle(buttonPosX, firstButtonPosY + (buttonHeight + spacing.y), buttonWidth, buttonHeight);
    auto settingsButton = std::make_shared<UiButton>(settingsButtonRect, "Settings", []()
                                                     { GameManager::GetCamera().SetUiState(PlayerCam::UiState::SETTINGS_MENU); });
    menuBackground->AddChild(settingsButton);

    constexpr Rectangle exitButtonRect = Rectangle(buttonPosX, firstButtonPosY + 2. * (buttonHeight + spacing.y), buttonWidth, buttonHeight);
    auto exitButton = std::make_shared<UiButton>(exitButtonRect, "Exit", []()
                                                 { GameManager::SetGameState(GameState::RUNNING, false); });
    menuBackground->AddChild(exitButton);

    UiManager::AddElement("ESC_MENU", escMenu);
}

void InitializeSettingsMenu()
{
    constexpr Vector2 menuSize = Vector2(1, 1) * 2. / 3.;
    constexpr Vector2 menuPos = Vector2(.5, .5) - menuSize / 2.;
    constexpr Vector2 spacing = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    constexpr double settingHeight = 1. / 36.;
    constexpr double halfPanelWidth = menuSize.x / 2. - spacing.x * 1.5;

    // Darken the background to draw focus to the UI
    auto settingsMenu = std::make_shared<UiPanel>(Rectangle(0, 0, 1, 1), Fade(BLACK, .2));
    std::weak_ptr<UiPanel> weakSettingsMenu = settingsMenu;
    settingsMenu->SetOnUpdate([weakSettingsMenu]()
                              { if (auto settingsMenu = weakSettingsMenu.lock())
                             { settingsMenu->SetVisible(GameManager::GetCamera().IsUiState(PlayerCam::UiState::SETTINGS_MENU)); } });

    // Draw a rectangle for the menu background
    auto menuBackground = std::make_shared<UiPanel>(Vector2ToRect(menuPos, menuSize), Fade(BLACK, .5));
    settingsMenu->AddChild(menuBackground);

    Rectangle monitorTextRect = Rectangle(menuPos.x + spacing.x, menuPos.y + spacing.y, halfPanelWidth, settingHeight);
    auto monitorText = std::make_shared<UiStatusBar>(monitorTextRect, "Render Monitor:");
    menuBackground->AddChild(monitorText);

    std::string monitorNames;
    for (int i = 0; i < GetMonitorCount(); i++)
    {
        if (i > 0)
            monitorNames += ";";
        monitorNames += GetMonitorName(i);
    }

    int selectedMonitor = GetCurrentMonitor();
    Rectangle monitorSelectRect = Rectangle(monitorTextRect.x + halfPanelWidth + spacing.x, menuPos.y + spacing.y, halfPanelWidth, settingHeight);
    auto monitorSelect = std::make_shared<UiComboBox>(monitorSelectRect, monitorNames, selectedMonitor, [](int monitor)
                                                      {  SetWindowMonitor(monitor); SetTargetFPS(GetMonitorRefreshRate(monitor)); });
    menuBackground->AddChild(monitorSelect);

    Rectangle masterVolumeTextRect = Rectangle(menuPos.x + spacing.x, monitorTextRect.y + settingHeight + spacing.y, halfPanelWidth, settingHeight);
    auto masterVolumeText = std::make_shared<UiStatusBar>(masterVolumeTextRect, "Master Volume:");
    menuBackground->AddChild(masterVolumeText);

    Rectangle masterVolumeSliderRect = Rectangle(masterVolumeTextRect.x + halfPanelWidth + spacing.x, masterVolumeTextRect.y, halfPanelWidth, settingHeight);
    auto masterVolumeSlider = std::make_shared<UiSlider>(masterVolumeSliderRect, AudioManager::GetMasterVolume(), 0, 1, [](float volume)
                                                         { AudioManager::SetMasterVolume(volume); });
    std::weak_ptr<UiSlider> weakMasterVolumeSlider = masterVolumeSlider;
    masterVolumeSlider->SetOnUpdate([weakMasterVolumeSlider]()
                                    { if (auto masterVolumeSlider = weakMasterVolumeSlider.lock())
                             { masterVolumeSlider->SetValue(AudioManager::GetMasterVolume()); } });
    menuBackground->AddChild(masterVolumeSlider);

    Rectangle musicVolumeTextRect = Rectangle(menuPos.x + spacing.x, masterVolumeTextRect.y + settingHeight + spacing.y, halfPanelWidth, settingHeight);
    auto musicVolumeText = std::make_shared<UiStatusBar>(musicVolumeTextRect, "Music Volume:");
    menuBackground->AddChild(musicVolumeText);

    Rectangle musicVolumeSliderRect = Rectangle(musicVolumeTextRect.x + halfPanelWidth + spacing.x, musicVolumeTextRect.y, halfPanelWidth, settingHeight);
    auto musicVolumeSlider = std::make_shared<UiSlider>(musicVolumeSliderRect, AudioManager::GetMusicVolume(), 0, 1, [](float volume)
                                                        { AudioManager::SetMusicVolume(volume); });
    std::weak_ptr<UiSlider> weakMusicVolumeSlider = musicVolumeSlider;
    musicVolumeSlider->SetOnUpdate([weakMusicVolumeSlider]()
                                   { if (auto musicVolumeSlider = weakMusicVolumeSlider.lock())
                             { musicVolumeSlider->SetValue(AudioManager::GetMusicVolume()); } });
    menuBackground->AddChild(musicVolumeSlider);

    Rectangle effectsVolumeTextRect = Rectangle(menuPos.x + spacing.x, musicVolumeTextRect.y + settingHeight + spacing.y, halfPanelWidth, settingHeight);
    auto effectsVolumeText = std::make_shared<UiStatusBar>(effectsVolumeTextRect, "Effects Volume:");
    menuBackground->AddChild(effectsVolumeText);

    Rectangle effectsVolumeSliderRect = Rectangle(effectsVolumeTextRect.x + halfPanelWidth + spacing.x, effectsVolumeTextRect.y, halfPanelWidth, settingHeight);
    auto effectsVolumeSlider = std::make_shared<UiSlider>(effectsVolumeSliderRect, AudioManager::GetEffectsVolume(), 0, 1, [](float volume)
                                                          { AudioManager::SetEffectsVolume(volume); });
    std::weak_ptr<UiSlider> weakEffectsVolumeSlider = effectsVolumeSlider;
    effectsVolumeSlider->SetOnUpdate([weakEffectsVolumeSlider]()
                                     { if (auto effectsVolumeSlider = weakEffectsVolumeSlider.lock())
                             { effectsVolumeSlider->SetValue(AudioManager::GetEffectsVolume()); } });
    menuBackground->AddChild(effectsVolumeSlider);

    UiManager::AddElement("SETTINGS_MENU", settingsMenu);
}

void InitializeSidebar()
{
    constexpr Vector2 largeButtonSize = Vector2ScreenScale(Vector2(64, 64));
    constexpr Vector2 smallButtonSize = largeButtonSize / 2.;
    constexpr Vector2 spacing = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));

    constexpr Rectangle buildButtonRect = Vector2ToRect(Vector2(1. - spacing.x - largeButtonSize.x, (1. - largeButtonSize.y) / 2.), largeButtonSize);
    bool isInBuildMode = GameManager::IsInBuildMode();
    auto buildToggle = std::make_shared<UiToggle>(buildButtonRect, isInBuildMode, [](bool state)
                                                  { GameManager::SetBuildModeState(state); });

    constexpr Rectangle buildIconRect = Vector2ToRect(Vector2(buildButtonRect.x, buildButtonRect.y) + largeButtonSize / 8., largeButtonSize * .75);
    buildToggle->AddChild(std::make_shared<UiIcon>(buildIconRect, "ICON", Rectangle(1, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8)));

    std::weak_ptr<UiToggle> weakBuildToggle = buildToggle;
    buildToggle->SetOnUpdate([weakBuildToggle]()
                             { if (auto buildToggle = weakBuildToggle.lock())
        { 
            buildToggle->SetVisible(GameManager::GetCamera().IsUiClear());
            buildToggle->SetToggle(GameManager::IsInBuildMode());
        } });

    UiManager::AddElement("BUILD_TGL", buildToggle);

    Rectangle overlayRect = Vector2ToRect(Vector2(1. - spacing.x - smallButtonSize.x, (1. + largeButtonSize.y) / 2. + spacing.y), smallButtonSize);

    for (auto overlay : magic_enum::enum_values<PlayerCam::Overlay>())
    {
        if (overlay == PlayerCam::Overlay::NONE)
            continue;

        bool isOverlayActive = GameManager::GetCamera().IsOverlay(overlay);
        auto overlayToggle = std::make_shared<UiToggle>(overlayRect, isOverlayActive, [overlay](bool)
                                                        { GameManager::GetCamera().ToggleOverlay(overlay); });

        Rectangle iconRect = Vector2ToRect(Vector2(overlayRect.x, overlayRect.y) + smallButtonSize / 4., smallButtonSize / 2.);
        float iconIndex = (float)(magic_enum::enum_underlying<PlayerCam::Overlay>(overlay) - 1);
        overlayToggle->AddChild(std::make_shared<UiIcon>(iconRect, "ICON", Rectangle(iconIndex, 0, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8)));

        std::weak_ptr<UiToggle> weakOverlayToggle = overlayToggle;
        overlayToggle->SetOnUpdate([weakOverlayToggle, overlay]()
                                   { if (auto overlayToggle = weakOverlayToggle.lock())
                             { 
                                overlayToggle->SetVisible(GameManager::GetCamera().IsUiClear());
                                overlayToggle->SetToggle(GameManager::GetCamera().IsOverlay(overlay));
                              } });

        UiManager::AddElement(std::format("OVERLAY_{}_TGL", magic_enum::enum_name(overlay)), overlayToggle);

        overlayRect.y += smallButtonSize.y + spacing.y;
    }
}

void InitializeBuildWorldUi()
{
    constexpr Vector2 BUTTON_SIZE = Vector2(1. / 3., 1. / 3.);
    constexpr double ICON_SIZE = 2. / 3.;
    constexpr Vector2 ICON_OFFSET = BUTTON_SIZE * (1. - ICON_SIZE) / 2.;

    auto buildMoveButton = std::make_shared<UiButton>(Rectangle(0, 0, 1, 1), "", nullptr, TextAttrs(), nullptr, true);

    std::weak_ptr<UiButton> weakBuildMoveButton = buildMoveButton;
    buildMoveButton->SetOnUpdate([weakBuildMoveButton, BUTTON_SIZE]()
                                 { if (auto buildMoveButton = weakBuildMoveButton.lock())
                            { 
                                auto selectedTile = GameManager::GetSelectedTile();
                                bool visible = GameManager::IsInBuildMode() && selectedTile;
                                buildMoveButton->SetVisible(visible);
                                if (visible)
                                    buildMoveButton->SetRect(Vector2ToRect(ToVector2(selectedTile->GetPosition()) + Vector2(1 + BUTTON_SIZE.x / 2., (.5 - BUTTON_SIZE.y) / 2.), BUTTON_SIZE));
                            } });

    auto buildMoveIcon = std::make_shared<UiIcon>(Rectangle(0, 0, 1, 1), "ICON", Rectangle(2, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8), nullptr, true);

    std::weak_ptr<UiIcon> weakBuildMoveIcon = buildMoveIcon;
    buildMoveIcon->SetOnUpdate([weakBuildMoveIcon, ICON_OFFSET, BUTTON_SIZE]()
                               { if (auto buildMoveIcon = weakBuildMoveIcon.lock())
                            { 
                                auto selectedTile = GameManager::GetSelectedTile();
                                if (GameManager::IsInBuildMode() && selectedTile)
                                    buildMoveIcon->SetRect(Vector2ToRect(ToVector2(selectedTile->GetPosition()) + Vector2(1 + BUTTON_SIZE.x / 2., (.5 - BUTTON_SIZE.y) / 2.) + ICON_OFFSET, BUTTON_SIZE * ICON_SIZE));
                            } });

    buildMoveButton->AddChild(buildMoveIcon);
    UiManager::AddElement("BUILD_MOVE_BTN", buildMoveButton);

    auto buildDeleteButton = std::make_shared<UiButton>(Rectangle(0, 0, 1, 1), "", []()
                                                        {
        GameManager::GetSelectedTile()->DeleteTile();
        GameManager::SetSelectedTile(nullptr); }, TextAttrs(), nullptr, true);

    std::weak_ptr<UiButton> weakBuildDeleteButton = buildDeleteButton;
    buildDeleteButton->SetOnUpdate([weakBuildDeleteButton, BUTTON_SIZE]()
                                   { if (auto buildDeleteButton = weakBuildDeleteButton.lock())
                            {
                                auto selectedTile = GameManager::GetSelectedTile();
                                bool visible = GameManager::IsInBuildMode() && selectedTile;
                                buildDeleteButton->SetVisible(visible);
                                if (visible)
                                    buildDeleteButton->SetRect(Vector2ToRect(ToVector2(selectedTile->GetPosition()) + Vector2(1 + BUTTON_SIZE.x / 2., 1. - ((.5 + BUTTON_SIZE.y) / 2.)), BUTTON_SIZE));
                            } });

    auto buildDeleteIcon = std::make_shared<UiIcon>(Rectangle(0, 0, 1, 1), "ICON", Rectangle(3, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8), nullptr, true);

    std::weak_ptr<UiIcon> weakBuildDeleteIcon = buildDeleteIcon;
    buildDeleteIcon->SetOnUpdate([weakBuildDeleteIcon, ICON_OFFSET, BUTTON_SIZE]()
                                 { if (auto buildDeleteIcon = weakBuildDeleteIcon.lock())
                            { 
                                auto selectedTile = GameManager::GetSelectedTile();
                                if (GameManager::IsInBuildMode() && selectedTile)
                                    buildDeleteIcon->SetRect(Vector2ToRect(ToVector2(selectedTile->GetPosition()) + Vector2(1 + BUTTON_SIZE.x / 2., 1. - ((.5 + BUTTON_SIZE.y) / 2.)) + ICON_OFFSET, BUTTON_SIZE * ICON_SIZE));
                            } });

    buildDeleteButton->AddChild(buildDeleteIcon);
    UiManager::AddElement("BUILD_DELETE_BTN", buildDeleteButton);
}

struct TileToggleConfig
{
    std::string tileId;
    std::string iconSpritesheetName;
    Rectangle iconSourceRect;
};

void AddBuildToggle(std::shared_ptr<UiPanel> &buildMenu, const TileToggleConfig &config, int index)
{
    constexpr Vector2 SPACING = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    constexpr Vector2 MENU_POS = Vector2(SPACING.x, .5 - SPACING.y);
    constexpr Vector2 TOGGLE_SIZE = Vector2ScreenScale(Vector2(64, 64));

    Vector2 togglePos = Vector2(MENU_POS.x + SPACING.x + index * (TOGGLE_SIZE.x + SPACING.x), MENU_POS.y + SPACING.y);

    auto onToggle = [config](bool state)
    {
        GameManager::SetBuildTileId(state ? config.tileId : "");
    };

    auto toggle = std::make_shared<UiToggle>(Vector2ToRect(togglePos, TOGGLE_SIZE), GameManager::IsBuildTileId(config.tileId), onToggle);

    std::weak_ptr<UiToggle> weakToggle = toggle;
    auto onUpdate = [weakToggle, config]()
    {
        if (auto toggle = weakToggle.lock())
            toggle->SetToggle(GameManager::IsBuildTileId(config.tileId));
    };
    toggle->SetOnUpdate(onUpdate);

    buildMenu->AddChild(toggle);

    Rectangle iconRect = Vector2ToRect(togglePos + TOGGLE_SIZE / 8., TOGGLE_SIZE * .75);
    auto icon = std::make_shared<UiIcon>(iconRect, config.iconSpritesheetName, config.iconSourceRect, WHITE);
    toggle->AddChild(icon);
}

void InitializeBuildMenu()
{
    constexpr Vector2 SPACING = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));

    auto buildMenu = std::make_shared<UiPanel>(Rectangle(SPACING.x, .5 - SPACING.y, 1. / 6., .5), Fade(BLACK, .5));
    std::weak_ptr<UiPanel> weakBuildMenu = buildMenu;
    buildMenu->SetOnUpdate([weakBuildMenu]()
                           { if (auto buildMenu = weakBuildMenu.lock())
                             { buildMenu->SetVisible(GameManager::IsInBuildMode() && GameManager::GetCamera().IsUiClear()); } });

    std::vector<TileToggleConfig> tileConfigs = {
        {"WALL", "STATION", Rectangle(4, 1, 1, 1) * TILE_SIZE},
        {"BLUE_FLOOR", "STATION", Rectangle(0, 1, 1, 1) * TILE_SIZE},
    };

    for (size_t i = 0; i < tileConfigs.size(); ++i)
    {
        AddBuildToggle(buildMenu, tileConfigs[i], (int)i);
    }

    UiManager::AddElement("BUILD_MENU", buildMenu);
}

void UiManager::InitializeElements()
{
    InitializeSidebar();
    InitializeEscapeMenu();
    InitializeSettingsMenu();
    InitializeBuildWorldUi();
    InitializeBuildMenu();
}
