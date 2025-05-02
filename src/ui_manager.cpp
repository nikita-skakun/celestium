#include "audio_manager.hpp"
#include "game_state.hpp"
#include "tile.hpp"
#include "ui_element.hpp"
#include "ui_manager.hpp"

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
    std::weak_ptr<UiPanel> _escMenu = escMenu;
    escMenu->SetOnUpdate([_escMenu]()
                         { if (auto escMenu = _escMenu.lock())
                            escMenu->SetVisible(GameManager::GetCamera().IsUiState(PlayerCam::UiState::ESC_MENU)); });

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
    std::weak_ptr<UiPanel> _settingsMenu = settingsMenu;
    settingsMenu->SetOnUpdate([_settingsMenu]()
                              { if (auto settingsMenu = _settingsMenu.lock())
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
    auto monitorSelect = std::make_shared<UiComboBox>(monitorSelectRect, monitorNames, selectedMonitor);
    menuBackground->AddChild(monitorSelect);

    Rectangle fpsSelectTextRect = Rectangle(menuPos.x + spacing.x, monitorTextRect.y + settingHeight + spacing.y, halfPanelWidth, settingHeight);
    auto fpsSelectText = std::make_shared<UiStatusBar>(fpsSelectTextRect, "Monitor FPS:");
    menuBackground->AddChild(fpsSelectText);

    const std::string availableFpsOptions = GameManager::GetCamera().GetFpsOptions();

    Rectangle fpsSelectRect = Rectangle(fpsSelectTextRect.x + halfPanelWidth + spacing.x, fpsSelectTextRect.y, halfPanelWidth, settingHeight);
    auto fpsSelect = std::make_shared<UiComboBox>(fpsSelectRect, availableFpsOptions, GameManager::GetCamera().GetFpsIndex(), [](int fpsIndex)
                                                  { GameManager::GetCamera().SetFpsIndex(fpsIndex); });
    menuBackground->AddChild(fpsSelect);

    std::weak_ptr<UiComboBox> _fpsSelect = fpsSelect;
    auto monitorSelectOnPress = [_fpsSelect](int monitor)
    {
        SetWindowMonitor(monitor);
        GameManager::GetCamera().SetFps(GetMonitorRefreshRate(monitor));
        if (auto fpsSelect = _fpsSelect.lock()) {
            std::string availableFpsOptions = GameManager::GetCamera().GetFpsOptions();
            fpsSelect->SetText(availableFpsOptions);
            fpsSelect->SetState(GameManager::GetCamera().GetFpsIndex());
        }
    };
    monitorSelect->SetOnPress(monitorSelectOnPress);

    Rectangle masterVolumeTextRect = Rectangle(menuPos.x + spacing.x, fpsSelectRect.y + settingHeight + spacing.y, halfPanelWidth, settingHeight);
    auto masterVolumeText = std::make_shared<UiStatusBar>(masterVolumeTextRect, "Master Volume:");
    menuBackground->AddChild(masterVolumeText);

    Rectangle masterVolumeSliderRect = Rectangle(masterVolumeTextRect.x + halfPanelWidth + spacing.x, masterVolumeTextRect.y, halfPanelWidth, settingHeight);
    auto masterVolumeSlider = std::make_shared<UiSlider>(masterVolumeSliderRect, AudioManager::GetMasterVolume(), 0, 1, [](float volume)
                                                         { AudioManager::SetMasterVolume(volume); });
    std::weak_ptr<UiSlider> _masterVolumeSlider = masterVolumeSlider;
    masterVolumeSlider->SetOnUpdate([_masterVolumeSlider]()
                                    { if (auto masterVolumeSlider = _masterVolumeSlider.lock())
                             { masterVolumeSlider->SetValue(AudioManager::GetMasterVolume()); } });
    menuBackground->AddChild(masterVolumeSlider);

    Rectangle musicVolumeTextRect = Rectangle(menuPos.x + spacing.x, masterVolumeTextRect.y + settingHeight + spacing.y, halfPanelWidth, settingHeight);
    auto musicVolumeText = std::make_shared<UiStatusBar>(musicVolumeTextRect, "Music Volume:");
    menuBackground->AddChild(musicVolumeText);

    Rectangle musicVolumeSliderRect = Rectangle(musicVolumeTextRect.x + halfPanelWidth + spacing.x, musicVolumeTextRect.y, halfPanelWidth, settingHeight);
    auto musicVolumeSlider = std::make_shared<UiSlider>(musicVolumeSliderRect, AudioManager::GetMusicVolume(), 0, 1, [](float volume)
                                                        { AudioManager::SetMusicVolume(volume); });
    std::weak_ptr<UiSlider> _musicVolumeSlider = musicVolumeSlider;
    musicVolumeSlider->SetOnUpdate([_musicVolumeSlider]()
                                   { if (auto musicVolumeSlider = _musicVolumeSlider.lock())
                             { musicVolumeSlider->SetValue(AudioManager::GetMusicVolume()); } });
    menuBackground->AddChild(musicVolumeSlider);

    Rectangle effectsVolumeTextRect = Rectangle(menuPos.x + spacing.x, musicVolumeTextRect.y + settingHeight + spacing.y, halfPanelWidth, settingHeight);
    auto effectsVolumeText = std::make_shared<UiStatusBar>(effectsVolumeTextRect, "Effects Volume:");
    menuBackground->AddChild(effectsVolumeText);

    Rectangle effectsVolumeSliderRect = Rectangle(effectsVolumeTextRect.x + halfPanelWidth + spacing.x, effectsVolumeTextRect.y, halfPanelWidth, settingHeight);
    auto effectsVolumeSlider = std::make_shared<UiSlider>(effectsVolumeSliderRect, AudioManager::GetEffectsVolume(), 0, 1, [](float volume)
                                                          { AudioManager::SetEffectsVolume(volume); });
    std::weak_ptr<UiSlider> _effectsVolumeSlider = effectsVolumeSlider;
    effectsVolumeSlider->SetOnUpdate([_effectsVolumeSlider]()
                                     { if (auto effectsVolumeSlider = _effectsVolumeSlider.lock())
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

    std::weak_ptr<UiToggle> _buildToggle = buildToggle;
    buildToggle->SetOnUpdate([_buildToggle]()
                             { if (auto buildToggle = _buildToggle.lock())
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

        std::weak_ptr<UiToggle> _overlayToggle = overlayToggle;
        overlayToggle->SetOnUpdate([_overlayToggle, overlay]()
                                   { if (auto overlayToggle = _overlayToggle.lock())
                             { 
                                overlayToggle->SetVisible(GameManager::GetCamera().IsUiClear());
                                overlayToggle->SetToggle(GameManager::GetCamera().IsOverlay(overlay));
                              } });

        UiManager::AddElement(std::format("OVERLAY_{}_TGL", magic_enum::enum_name(overlay)), overlayToggle);

        overlayRect.y += smallButtonSize.y + spacing.y;
    }
}

// void InitializeBuildWorldUi()
// {
//     constexpr Vector2 SPACING = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
//     constexpr Vector2 BUTTON_SIZE = Vector2ScreenScale(Vector2(32, 32));
//     constexpr Vector2 ICON_SIZE = BUTTON_SIZE * 3.f / 4.f;
//     constexpr Vector2 ICON_OFFSET = (BUTTON_SIZE - ICON_SIZE) / 2.;

//     auto moveOnPress = []()
//     { GameManager::SetMoveTile(); };
//     Vector2 buildMovePos = Vector2(SPACING.x, .5 - SPACING.y / 2. - BUTTON_SIZE.y);
//     auto buildMoveButton = std::make_shared<UiButton>(Vector2ToRect(buildMovePos, BUTTON_SIZE), "", moveOnPress);

//     std::weak_ptr<UiButton> _buildMoveButton = buildMoveButton;
//     buildMoveButton->SetOnUpdate([_buildMoveButton]()
//                                  { if (auto buildMoveButton = _buildMoveButton.lock())
//                             { 
//                                 auto selectedTile = GameManager::GetSelectedTile();
//                                 buildMoveButton->SetVisible(GameManager::IsInBuildMode() && GameManager::GetCamera().IsUiClear());
//                                 buildMoveButton->SetEnabled(selectedTile != nullptr);
//                             } });

//     auto buildMoveIcon = std::make_shared<UiIcon>(Vector2ToRect(buildMovePos + ICON_OFFSET, ICON_SIZE), "ICON", Rectangle(2, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8));
//     buildMoveButton->AddChild(buildMoveIcon);
//     UiManager::AddElement("BUILD_MOVE_BTN", buildMoveButton);

//     auto deleteOnPress = []()
//     {   GameManager::GetSelectedTile()->DeleteTile();
//         GameManager::SetSelectedTile(nullptr); };
//     Vector2 buildDeletePos = Vector2(SPACING.x * 2. + BUTTON_SIZE.x, .5 - SPACING.y / 2. - BUTTON_SIZE.y);
//     auto buildDeleteButton = std::make_shared<UiButton>(Vector2ToRect(buildDeletePos, BUTTON_SIZE), "", deleteOnPress);

//     std::weak_ptr<UiButton> _buildDeleteButton = buildDeleteButton;
//     buildDeleteButton->SetOnUpdate([_buildDeleteButton]()
//                                    { if (auto buildDeleteButton = _buildDeleteButton.lock())
//                             {
//                                 auto selectedTile = GameManager::GetSelectedTile();
//                                 buildDeleteButton->SetVisible(GameManager::IsInBuildMode() && GameManager::GetCamera().IsUiClear());
//                                 buildDeleteButton->SetEnabled(selectedTile != nullptr);
//                             } });

//     auto buildDeleteIcon = std::make_shared<UiIcon>(Vector2ToRect(buildDeletePos + ICON_OFFSET, ICON_SIZE), "ICON", Rectangle(3, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8));
//     buildDeleteButton->AddChild(buildDeleteIcon);
//     UiManager::AddElement("BUILD_DELETE_BTN", buildDeleteButton);

//     auto rotateOnPress = []()
//     { GameManager::GetSelectedTile()->RotateTile(); };
//     Vector2 buildRotatePos = Vector2(SPACING.x * 3. + BUTTON_SIZE.x * 2., .5 - SPACING.y / 2. - BUTTON_SIZE.y);
//     auto buildRotateButton = std::make_shared<UiButton>(Vector2ToRect(buildRotatePos, BUTTON_SIZE), "", rotateOnPress);

//     std::weak_ptr<UiButton> _buildRotateButton = buildRotateButton;
//     buildRotateButton->SetOnUpdate([_buildRotateButton]()
//                                    { if (auto buildRotateButton = _buildRotateButton.lock())
//                             {
//                                 auto selectedTile = GameManager::GetSelectedTile();
//                                 buildRotateButton->SetVisible(GameManager::IsInBuildMode() && GameManager::GetCamera().IsUiClear());
//                                 buildRotateButton->SetEnabled(selectedTile != nullptr && selectedTile->HasComponent<RotatableComponent>());
//                             } });

//     auto buildRotateIcon = std::make_shared<UiIcon>(Vector2ToRect(buildRotatePos + ICON_OFFSET, ICON_SIZE), "ICON", Rectangle(4, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8));
//     buildRotateButton->AddChild(buildRotateIcon);
//     UiManager::AddElement("BUILD_ROTATE_BTN", buildRotateButton);
// }

struct TileToggleConfig
{
    std::string tileId;
    std::string iconSpritesheetName;
    Rectangle iconSourceRect;
};

void AddBuildToggle(const std::shared_ptr<UiPanel> &buildMenu, const TileToggleConfig &config, int index)
{
    constexpr Vector2 SPACING = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    constexpr Vector2 MENU_POS = Vector2(SPACING.x, .5 + SPACING.y);
    constexpr Vector2 MENU_SIZE = Vector2(1. / 6., .5);
    constexpr Vector2 TOGGLE_SIZE = Vector2ScreenScale(Vector2(64, 64));
    constexpr int MAX_ITEMS_PER_ROW = Floor((MENU_SIZE.x - 2 * SPACING.x) / (TOGGLE_SIZE.x + SPACING.x));
    constexpr float SPACING_FOR_CENTER = (MENU_SIZE.x - (MAX_ITEMS_PER_ROW * TOGGLE_SIZE.x + (MAX_ITEMS_PER_ROW + 1) * SPACING.x)) / (MAX_ITEMS_PER_ROW + 1);

    // Calculate row and column based on index and maxItemsPerRow
    int row = index / MAX_ITEMS_PER_ROW;
    int column = index % MAX_ITEMS_PER_ROW;

    Vector2 togglePos = Vector2(
        MENU_POS.x + SPACING.x + SPACING_FOR_CENTER + column * (TOGGLE_SIZE.x + SPACING.x + SPACING_FOR_CENTER),
        MENU_POS.y + SPACING.y + row * (TOGGLE_SIZE.y + SPACING.y));

    auto onToggle = [config](bool state)
    {
        GameManager::SetBuildTileId(state ? config.tileId : "");
    };

    auto toggle = std::make_shared<UiToggle>(Vector2ToRect(togglePos, TOGGLE_SIZE), GameManager::IsBuildTileId(config.tileId), onToggle);

    std::weak_ptr<UiToggle> _toggle = toggle;
    auto onUpdate = [_toggle, config]()
    {
        if (auto toggle = _toggle.lock())
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

    auto buildMenu = std::make_shared<UiPanel>(Rectangle(SPACING.x, .5 + SPACING.y / 2., 1. / 6., .5), Fade(BLACK, .5));
    std::weak_ptr<UiPanel> _buildMenu = buildMenu;
    buildMenu->SetOnUpdate([_buildMenu]()
                           { if (auto buildMenu = _buildMenu.lock())
                             { buildMenu->SetVisible(GameManager::IsInBuildMode() && GameManager::GetCamera().IsUiClear()); } });

    std::vector<TileToggleConfig> tileConfigs = {
        {"WALL", "STATION", Rectangle(4, 1, 1, 1) * TILE_SIZE},
        {"BLUE_FLOOR", "STATION", Rectangle(0, 1, 1, 1) * TILE_SIZE},
        {"DOOR", "STATION", Rectangle(0, 5, 1, 1) * TILE_SIZE},
        {"OXYGEN_PRODUCER", "STATION", Rectangle(6, 7, 1, 1) * TILE_SIZE},
        {"BATTERY", "STATION", Rectangle(5, 7, 1, 1) * TILE_SIZE},
        {"FRAME", "STATION", Rectangle(1, 0, 1, 1) * TILE_SIZE},
        {"SOLAR_PANEL", "STATION", Rectangle(7, 7, 1, 1) * TILE_SIZE},
    };

    for (size_t i = 0; i < tileConfigs.size(); ++i)
    {
        AddBuildToggle(buildMenu, tileConfigs.at(i), (int)i);
    }

    UiManager::AddElement("BUILD_MENU", buildMenu);
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

void UiManager::InitializeElements()
{
    InitializeSidebar();
    InitializeEscapeMenu();
    InitializeSettingsMenu();
    // InitializeBuildWorldUi();
    InitializeBuildMenu();
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
