#include "audio_manager.hpp"
#include "def_manager.hpp"
#include "game_state.hpp"
#include "tile.hpp"
#include "ui_element.hpp"
#include "ui_manager.hpp"
#include "logging.hpp"

void InitializeEscapeMenu()
{
    constexpr int buttonCount = 4;
    constexpr double buttonWidth = 1. / 12.;
    constexpr Vector2 spacing = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    constexpr double buttonHeight = 1. / 24.;
    constexpr double totalButtonHeight = buttonCount * buttonHeight + (buttonCount - 1) * spacing.y;

    // Calculate menu dimensions
    constexpr Vector2 menuSize = Vector2(buttonWidth, totalButtonHeight) + spacing * 2;

    constexpr Vector2 menuPos = Vector2(.5, .5) - menuSize / 2.;

    // Darken the background to draw focus to the UI
    auto escMenu = std::make_shared<UiPanel>(Rectangle(0, 0, 1, 1), Fade(BLACK, .2));
    escMenu->SetVisible(false);
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

    constexpr Rectangle mainMenuButtonRect = Rectangle(buttonPosX, firstButtonPosY + 2. * (buttonHeight + spacing.y), buttonWidth, buttonHeight);
    auto mainMenuButton = std::make_shared<UiButton>(mainMenuButtonRect, "Main Menu", []()
                                                     { GameManager::RequestStateChange(GameState::MAIN_MENU); });
    menuBackground->AddChild(mainMenuButton);

    constexpr Rectangle exitButtonRect = Rectangle(buttonPosX, firstButtonPosY + 3. * (buttonHeight + spacing.y), buttonWidth, buttonHeight);
    auto exitButton = std::make_shared<UiButton>(exitButtonRect, "Exit", []()
                                                 { GameManager::RequestStateChange(GameState::NONE); });
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
    settingsMenu->SetVisible(false);
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
        if (auto fpsSelect = _fpsSelect.lock())
        {
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
    buildToggle->SetVisible(false);

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
        overlayToggle->SetVisible(false);

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

void InitializeBuildWorldUi()
{
    constexpr Vector2 SPACING = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    constexpr Vector2 BUTTON_SIZE = Vector2ScreenScale(Vector2(32, 32));
    constexpr Vector2 ICON_SIZE = BUTTON_SIZE * 3.f / 4.f;
    constexpr Vector2 ICON_OFFSET = (BUTTON_SIZE - ICON_SIZE) / 2.;
    constexpr Vector2 PANEL_SIZE = Vector2(.8, BUTTON_SIZE.y + SPACING.y * 2.);
    constexpr Vector2 PANEL_POS = Vector2(.5 - PANEL_SIZE.x / 2., 1. - PANEL_SIZE.y - SPACING.y);

    auto onHorizontalSymmetryToggle = [](bool)
    { GameManager::ToggleHorizontalSymmetry(); };
    Vector2 horizontalSymmetryPos = Vector2(PANEL_POS.x - BUTTON_SIZE.x - SPACING.x, 1.f - SPACING.y * 1.5f - BUTTON_SIZE.y);
    auto horizontalSymmetryToggle = std::make_shared<UiToggle>(Vector2ToRect(horizontalSymmetryPos, BUTTON_SIZE), GameManager::IsHorizontalSymmetry(), onHorizontalSymmetryToggle);
    horizontalSymmetryToggle->SetVisible(false);

    std::weak_ptr<UiToggle> _horizontalSymmetryToggle = horizontalSymmetryToggle;
    horizontalSymmetryToggle->SetOnUpdate([_horizontalSymmetryToggle]()
                                          { if (auto horizontalSymmetryToggle = _horizontalSymmetryToggle.lock())
                                    { 
                                        horizontalSymmetryToggle->SetVisible(GameManager::IsInBuildMode() && GameManager::GetCamera().IsUiClear());
                                        horizontalSymmetryToggle->SetToggle(GameManager::IsHorizontalSymmetry());
                                    } });

    auto horizontalSymmetryIcon = std::make_shared<UiIcon>(Vector2ToRect(horizontalSymmetryPos + ICON_OFFSET, ICON_SIZE), "ICON", Rectangle(6, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8));
    horizontalSymmetryToggle->AddChild(horizontalSymmetryIcon);
    UiManager::AddElement("BUILD_HOR_SYM_BTN", horizontalSymmetryToggle);

    auto onVerticalSymmetryToggle = [](bool)
    { GameManager::ToggleVerticalSymmetry(); };
    Vector2 verticalTogglePos = horizontalSymmetryPos - Vector2(0, BUTTON_SIZE.y + SPACING.y);
    auto verticalSymmetryToggle = std::make_shared<UiToggle>(Vector2ToRect(verticalTogglePos, BUTTON_SIZE), GameManager::IsVerticalSymmetry(), onVerticalSymmetryToggle);
    verticalSymmetryToggle->SetVisible(false);

    std::weak_ptr<UiToggle> _verticalSymmetryToggle = verticalSymmetryToggle;
    verticalSymmetryToggle->SetOnUpdate([_verticalSymmetryToggle]()
                                        { if (auto verticalSymmetryToggle = _verticalSymmetryToggle.lock())
                                    { 
                                        verticalSymmetryToggle->SetVisible(GameManager::IsInBuildMode() && GameManager::GetCamera().IsUiClear());
                                        verticalSymmetryToggle->SetToggle(GameManager::IsVerticalSymmetry());
                                    } });

    auto verticalSymIcon = std::make_shared<UiIcon>(Vector2ToRect(verticalTogglePos + ICON_OFFSET, ICON_SIZE), "ICON", Rectangle(5, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8));
    verticalSymmetryToggle->AddChild(verticalSymIcon);
    UiManager::AddElement("BUILD_VER_SYM_BTN", verticalSymmetryToggle);
}

struct TileToggleConfig
{
    std::string tileId;
    std::string iconSpritesheetName;
    Vector2Int iconOffset;
};

void AddBuildToggle(std::shared_ptr<UiPanel> buildMenuPanel, const TileToggleConfig &config, int index)
{
    constexpr Vector2 SPACING = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    constexpr Vector2 TOGGLE_SIZE = Vector2ScreenScale(Vector2(64, 64));
    constexpr Vector2 PANEL_SIZE = Vector2(.8, TOGGLE_SIZE.y + SPACING.y * 2.);
    constexpr Vector2 PANEL_POS = Vector2(.5 - PANEL_SIZE.x / 2., 1. - PANEL_SIZE.y - SPACING.y * 4. - TOGGLE_SIZE.y);

    Vector2 togglePos = PANEL_POS + Vector2(SPACING.x + index * (TOGGLE_SIZE.x + SPACING.x), SPACING.y);

    auto onToggle = [config](bool state)
    { GameManager::SetBuildTileId(state ? config.tileId : ""); };

    auto toggle = std::make_shared<UiToggle>(Vector2ToRect(togglePos, TOGGLE_SIZE),
                                             GameManager::IsBuildTileId(config.tileId),
                                             onToggle);

    std::weak_ptr<UiToggle> _toggle = toggle;
    toggle->SetOnUpdate([_toggle, config]()
                        {
        if (auto t = _toggle.lock())
            t->SetToggle(GameManager::IsBuildTileId(config.tileId)); });

    buildMenuPanel->AddChild(toggle);

    Rectangle iconRect = Vector2ToRect(togglePos + TOGGLE_SIZE / 8.f, TOGGLE_SIZE * 0.75f);
    auto icon = std::make_shared<UiIcon>(iconRect, config.iconSpritesheetName, Vector2ToRect(ToVector2(config.iconOffset), Vector2(1, 1)) * TILE_SIZE, WHITE);
    toggle->AddChild(icon);
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
    constexpr int buttonCount = 4;
    constexpr double buttonWidth = 1. / 6.; // Menu width
    constexpr Vector2 spacing = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    constexpr double buttonHeight = 1. / 18.;
    constexpr double totalButtonHeight = buttonCount * buttonHeight + (buttonCount - 1) * spacing.y;

    constexpr Vector2 menuSize = Vector2(buttonWidth, totalButtonHeight) + spacing * 2;
    constexpr Vector2 menuPos = Vector2(1. - menuSize.x - spacing.x, (1. - menuSize.y) / 2.);

    auto mainMenuPanel = std::make_shared<UiPanel>(Vector2ToRect(menuPos, menuSize), Fade(BLACK, .5));
    std::weak_ptr<UiPanel> _mainMenuPanel = mainMenuPanel;
    mainMenuPanel->SetOnUpdate([_mainMenuPanel]()
                               { if (auto mainMenuPanel = _mainMenuPanel.lock())
                             { mainMenuPanel->SetVisible(GameManager::GetCamera().IsUiClear()); } });

    constexpr double firstButtonPosY = menuPos.y + (menuSize.y - totalButtonHeight) / 2.;
    constexpr double buttonPosX = menuPos.x + (menuSize.x - buttonWidth) / 2.; // Center buttons within the panel

    constexpr Rectangle newGameButtonRect = Rectangle(buttonPosX, firstButtonPosY, buttonWidth, buttonHeight);
    auto newGameButton = std::make_shared<UiButton>(newGameButtonRect, "New Game", []()
                                                    { GameManager::RequestStateChange(GameState::GAME_SIM); });
    mainMenuPanel->AddChild(newGameButton);

    constexpr Rectangle loadGameButtonRect = Rectangle(buttonPosX, firstButtonPosY + (buttonHeight + spacing.y), buttonWidth, buttonHeight);
    auto loadGameButton = std::make_shared<UiButton>(loadGameButtonRect, "Load Game", []() { /* TODO: Implement load game functionality */ });
    mainMenuPanel->AddChild(loadGameButton);

    constexpr Rectangle settingsButtonRect = Rectangle(buttonPosX, firstButtonPosY + 2. * (buttonHeight + spacing.y), buttonWidth, buttonHeight);
    auto settingsButton = std::make_shared<UiButton>(settingsButtonRect, "Settings", []()
                                                     { GameManager::GetCamera().SetUiState(PlayerCam::UiState::SETTINGS_MENU); });
    mainMenuPanel->AddChild(settingsButton);

    constexpr Rectangle exitButtonRect = Rectangle(buttonPosX, firstButtonPosY + 3. * (buttonHeight + spacing.y), buttonWidth, buttonHeight);
    auto exitButton = std::make_shared<UiButton>(exitButtonRect, "Exit", []()
                                                 { GameManager::RequestStateChange(GameState::NONE); });
    mainMenuPanel->AddChild(exitButton);

    UiManager::AddElement("MAIN_MENU", mainMenuPanel);

    InitializeSettingsMenu();
}

void InitializeCategorySpecificMenu()
{
    TileDef::Category selectedCategory = GameManager::GetSelectedCategory();
    auto tileDefs = DefinitionManager::GetTileDefinitions();

    int index = 0;
    for (const auto &pair : tileDefs)
    {
        const auto &tileDef = pair.second;
        if (tileDef->GetCategory() == selectedCategory)
        {
            TileToggleConfig config;
            config.tileId = tileDef->GetId();
            config.iconSpritesheetName = "STATION";
            config.iconOffset = tileDef->GetIconOffset();

            AddBuildToggle(std::dynamic_pointer_cast<UiPanel>(UiManager::GetElement("BUILD_MENU")), config, index);
            index++;
        }
    }
}

void ToggleBuildCategory(TileDef::Category category)
{
    if (auto buildMenu = std::dynamic_pointer_cast<UiPanel>(UiManager::GetElement("BUILD_MENU")))
    {
        buildMenu->ClearChildren();
    }

    GameManager::ToggleSelectedCategory(category);
    InitializeCategorySpecificMenu();
}

void InitializeBuildCategory()
{
    // 3 buttons on the bottom centered of screen
    constexpr Vector2 SPACING = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    constexpr Vector2 BUTTON_SIZE = Vector2ScreenScale(Vector2(64, 64));
    constexpr Vector2 ICON_SIZE = BUTTON_SIZE * 3.f / 4.f;
    constexpr Vector2 ICON_OFFSET = (BUTTON_SIZE - ICON_SIZE) / 2.;

    constexpr Vector2 PANEL_SIZE = Vector2(.8, BUTTON_SIZE.y + SPACING.y * 2.);
    constexpr Vector2 PANEL_POS = Vector2(.5 - PANEL_SIZE.x / 2., 1. - PANEL_SIZE.y - SPACING.y);
    auto buildCategoryPanel = std::make_shared<UiPanel>(Vector2ToRect(PANEL_POS, PANEL_SIZE), Fade(BLACK, .5));
    buildCategoryPanel->SetVisible(false);
    std::weak_ptr<UiPanel> _buildCategoryPanel = buildCategoryPanel;
    buildCategoryPanel->SetOnUpdate([_buildCategoryPanel]()
                                    { if (auto buildCategoryPanel = _buildCategoryPanel.lock())
                             { buildCategoryPanel->SetVisible(GameManager::GetCamera().IsUiClear() && GameManager::IsInBuildMode()); } });
    UiManager::AddElement("BUILD_CATEGORY", buildCategoryPanel);

    // First button will be structure button
    auto structureButton = std::make_shared<UiToggle>(Vector2ToRect(Vector2(PANEL_POS.x + SPACING.x, PANEL_POS.y + SPACING.y), BUTTON_SIZE),
                                                      GameManager::GetSelectedCategory() == TileDef::Category::STRUCTURE, [](bool)
                                                      { ToggleBuildCategory(TileDef::Category::STRUCTURE); });
    std::weak_ptr<UiToggle> _structureButton = structureButton;
    structureButton->SetOnUpdate([_structureButton]()
                                 { if (auto structureButton = _structureButton.lock())
                             { structureButton->SetToggle(GameManager::GetSelectedCategory() == TileDef::Category::STRUCTURE); } });
    buildCategoryPanel->AddChild(structureButton);
    auto structureIcon = std::make_shared<UiIcon>(Vector2ToRect(Vector2(PANEL_POS.x + SPACING.x, PANEL_POS.y + SPACING.y) + ICON_OFFSET, ICON_SIZE),
                                                  "ICON", Rectangle(1, 0, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8));
    structureButton->AddChild(structureIcon);

    // Second button will be power button
    auto powerButton = std::make_shared<UiToggle>(Vector2ToRect(structureButton->GetPosition() + Vector2(BUTTON_SIZE.x + SPACING.x * 2., 0), BUTTON_SIZE),
                                                  GameManager::GetSelectedCategory() == TileDef::Category::POWER, [](bool)
                                                  { ToggleBuildCategory(TileDef::Category::POWER); });
    std::weak_ptr<UiToggle> _powerButton = powerButton;
    powerButton->SetOnUpdate([_powerButton]()
                             { if (auto powerButton = _powerButton.lock())
                             { powerButton->SetToggle(GameManager::GetSelectedCategory() == TileDef::Category::POWER); } });
    buildCategoryPanel->AddChild(powerButton);
    auto powerIcon = std::make_shared<UiIcon>(Vector2ToRect(powerButton->GetPosition() + ICON_OFFSET, ICON_SIZE),
                                              "ICON", Rectangle(2, 0, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8));
    powerButton->AddChild(powerIcon);

    // Third button will be oxygen button
    auto oxygenButton = std::make_shared<UiToggle>(Vector2ToRect(powerButton->GetPosition() + Vector2(BUTTON_SIZE.x + SPACING.x * 2., 0), BUTTON_SIZE),
                                                   GameManager::GetSelectedCategory() == TileDef::Category::OXYGEN, [](bool)
                                                   { ToggleBuildCategory(TileDef::Category::OXYGEN); });
    std::weak_ptr<UiToggle> _oxygenButton = oxygenButton;
    oxygenButton->SetOnUpdate([_oxygenButton]()
                              { if (auto oxygenButton = _oxygenButton.lock())
                             { oxygenButton->SetToggle(GameManager::GetSelectedCategory() == TileDef::Category::OXYGEN); } });
    buildCategoryPanel->AddChild(oxygenButton);
    auto oxygenIcon = std::make_shared<UiIcon>(Vector2ToRect(oxygenButton->GetPosition() + ICON_OFFSET, ICON_SIZE),
                                               "ICON", Rectangle(0, 0, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8));
    oxygenButton->AddChild(oxygenIcon);
}

void InitializeBuildMenu()
{
    // Panel right above the build category panel
    constexpr Vector2 SPACING = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));
    constexpr Vector2 BUTTON_SIZE = Vector2ScreenScale(Vector2(64, 64));
    // constexpr Vector2 ICON_SIZE = BUTTON_SIZE * 3.f / 4.f;
    // constexpr Vector2 ICON_OFFSET = (BUTTON_SIZE - ICON_SIZE) / 2.;
    constexpr Vector2 PANEL_SIZE = Vector2(.8, BUTTON_SIZE.y + SPACING.y * 2.);
    constexpr Vector2 PANEL_POS = Vector2(.5 - PANEL_SIZE.x / 2., 1. - PANEL_SIZE.y - SPACING.y * 4. - BUTTON_SIZE.y);

    auto buildMenuPanel = std::make_shared<UiPanel>(Vector2ToRect(PANEL_POS, PANEL_SIZE), Fade(BLACK, .5));
    buildMenuPanel->SetVisible(false);
    std::weak_ptr<UiPanel> _buildMenuPanel = buildMenuPanel;
    buildMenuPanel->SetOnUpdate([_buildMenuPanel]()
                                { if (auto buildMenuPanel = _buildMenuPanel.lock())
                             { buildMenuPanel->SetVisible(GameManager::GetCamera().IsUiClear() && GameManager::IsInBuildMode() && GameManager::GetSelectedCategory() != TileDef::Category::NONE); } });
    UiManager::AddElement("BUILD_MENU", buildMenuPanel);
}

void UiManager::InitializeGameSim()
{
    InitializeSidebar();
    InitializeEscapeMenu();
    InitializeSettingsMenu();
    InitializeBuildWorldUi();
    InitializeBuildCategory();
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
