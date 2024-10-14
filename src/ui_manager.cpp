#include "ui_manager.hpp"

void InitializeEscapeMenu(PlayerCam &camera)
{
    constexpr int buttonCount = 3;
    constexpr double buttonWidth = 1. / 12.;
    constexpr double buttonSpacing = 1. / 72.;
    constexpr double buttonHeight = 1. / 24.;
    constexpr double totalButtonHeight = buttonCount * buttonHeight + (buttonCount - 1) * buttonSpacing;

    // Calculate menu dimensions
    constexpr Vector2 menuSize = Vector2(buttonWidth + buttonSpacing, totalButtonHeight + buttonSpacing * 2.);

    constexpr Vector2 menuPos = Vector2(.5, .5) - menuSize / 2.;

    // Darken the background to draw focus to the UI
    auto escMenu = std::make_shared<UiPanel>(Rectangle(0, 0, 1, 1), Fade(BLACK, .2));
    std::weak_ptr<UiPanel> weakEscMenu = escMenu;
    escMenu->SetOnUpdate([weakEscMenu, &camera]()
                         { if (auto escMenu = weakEscMenu.lock())
                             { escMenu->SetVisibility(camera.IsUiState(PlayerCam::UiState::ESC_MENU)); } });

    // Draw a rectangle for the menu background
    auto menuBackground = std::make_shared<UiPanel>(Vector2ToRect(menuPos, menuSize), Fade(BLACK, .5));
    escMenu->AddChild(menuBackground);

    // Dynamically position buttons in the center of the menu
    constexpr double firstButtonPosY = menuPos.y + (menuSize.y - totalButtonHeight) / 2.;
    constexpr double buttonPosX = .5 - buttonWidth / 2.;

    constexpr Rectangle resumeButtonRect = Rectangle(buttonPosX, firstButtonPosY, buttonWidth, buttonHeight);
    auto resumeButton = std::make_shared<UiButton>(resumeButtonRect, "Resume", [&camera]()
                                                   { camera.SetUiState(PlayerCam::UiState::NONE); });
    escMenu->AddChild(resumeButton);

    constexpr Rectangle settingsButtonRect = Rectangle(buttonPosX, firstButtonPosY + (buttonHeight + buttonSpacing), buttonWidth, buttonHeight);
    auto settingsButton = std::make_shared<UiButton>(settingsButtonRect, "Settings", [&camera]()
                                                     { camera.SetUiState(PlayerCam::UiState::SETTINGS_MENU); });
    escMenu->AddChild(settingsButton);

    constexpr Rectangle exitButtonRect = Rectangle(buttonPosX, firstButtonPosY + 2. * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight);
    auto exitButton = std::make_shared<UiButton>(exitButtonRect, "Exit", []()
                                                 { GameManager::SetBit(GameState::RUNNING, false); });
    escMenu->AddChild(exitButton);

    UiManager::AddElement("ESC_MENU", escMenu);
}

void InitializeSettingsMenu(PlayerCam &camera)
{
    constexpr Vector2 menuSize = Vector2(1., 1.) * 2. / 3.;
    constexpr Vector2 menuPos = Vector2(.5, .5) - menuSize / 2.;

    // Darken the background to draw focus to the UI
    auto settingsMenu = std::make_shared<UiPanel>(Rectangle(0, 0, 1, 1), Fade(BLACK, .2));
    std::weak_ptr<UiPanel> weakSettingsMenu = settingsMenu;
    settingsMenu->SetOnUpdate([weakSettingsMenu, &camera]()
                              { if (auto settingsMenu = weakSettingsMenu.lock())
                             { settingsMenu->SetVisibility(camera.IsUiState(PlayerCam::UiState::SETTINGS_MENU)); } });

    // Draw a rectangle for the menu background
    auto menuBackground = std::make_shared<UiPanel>(Vector2ToRect(menuPos, menuSize), Fade(BLACK, .5));
    settingsMenu->AddChild(menuBackground);

    constexpr double spacing = 1. / 72.;
    constexpr double settingHeight = 1. / 36.;
    constexpr double halfPanelWidth = menuSize.x / 2. - spacing * 2. / 3.;

    Rectangle monitorTextRect = Rectangle(menuPos.x + spacing / 3., menuPos.y + spacing, halfPanelWidth, settingHeight);
    auto monitorText = std::make_shared<UiStatusBar>(monitorTextRect, "Render Monitor:");
    settingsMenu->AddChild(monitorText);

    std::string monitorNames;
    for (int i = 0; i < GetMonitorCount(); i++)
    {
        if (i > 0)
            monitorNames += ";";
        monitorNames += GetMonitorName(i);
    }

    int selectedMonitor = GetCurrentMonitor();
    Rectangle monitorSelectRect = Rectangle(menuPos.x + halfPanelWidth + spacing, menuPos.y + spacing, halfPanelWidth, settingHeight);
    auto monitorSelect = std::make_shared<UiComboBox>(monitorSelectRect, monitorNames, selectedMonitor, [](int monitor)
                                                      {  SetWindowMonitor(monitor); SetTargetFPS(GetMonitorRefreshRate(monitor)); });
    settingsMenu->AddChild(monitorSelect);

    UiManager::AddElement("SETTINGS_MENU", settingsMenu);
}

void InitializeSidebar(PlayerCam &camera)
{
    constexpr Vector2 largeButtonSize = Vector2(1. / 30., 8. / 135.);
    constexpr Vector2 smallButtonSize = largeButtonSize / 2.;
    constexpr double spacing = 1. / 72.;

    constexpr Rectangle buildButtonRect = Vector2ToRect(Vector2(spacing / 2., (1. - largeButtonSize.x) / 2.), largeButtonSize);
    bool isInBuildMode = camera.IsInBuildMode();
    auto buildToggle = std::make_shared<UiToggle>(buildButtonRect, isInBuildMode, [&camera](bool state)
                                                  { camera.SetBuildModeState(state); });

    constexpr Rectangle buildIconRect = Vector2ToRect(Vector2(buildButtonRect.x, buildButtonRect.y) + largeButtonSize / 8., largeButtonSize * .75);
    buildToggle->AddChild(std::make_shared<UiIcon>(buildIconRect, "ICON", Rectangle(1, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8)));

    std::weak_ptr<UiToggle> weakBuildToggle = buildToggle;
    buildToggle->SetOnUpdate([weakBuildToggle, &camera]()
                             { if (auto buildToggle = weakBuildToggle.lock())
                             { buildToggle->SetVisibility(camera.IsUiClear()); } });

    UiManager::AddElement("BUILD_TGL", buildToggle);

    Rectangle overlayRect = Vector2ToRect(Vector2(spacing / 2., (1. + largeButtonSize.y) / 2. + spacing * 2.), smallButtonSize);

    for (auto overlay : magic_enum::enum_values<PlayerCam::Overlay>())
    {
        if (overlay == PlayerCam::Overlay::NONE)
            continue;

        bool isOverlayActive = camera.IsOverlay(overlay);
        auto overlayToggle = std::make_shared<UiToggle>(overlayRect, isOverlayActive, [&camera, overlay](bool)
                                                        { camera.ToggleOverlay(overlay); });

        Rectangle iconRect = Vector2ToRect(Vector2(overlayRect.x, overlayRect.y) + smallButtonSize / 4., smallButtonSize / 2.);
        float iconIndex = (float)(magic_enum::enum_underlying<PlayerCam::Overlay>(overlay) - 1);
        overlayToggle->AddChild(std::make_shared<UiIcon>(iconRect, "ICON", Rectangle(iconIndex, 0, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8)));

        std::weak_ptr<UiToggle> weakOverlayToggle = overlayToggle;
        overlayToggle->SetOnUpdate([weakOverlayToggle, overlay, &camera]()
                                   { if (auto overlayToggle = weakOverlayToggle.lock())
                             { 
                                overlayToggle->SetVisibility(camera.IsUiClear());
                                overlayToggle->SetToggle(camera.IsOverlay(overlay));
                              } });

        UiManager::AddElement(std::format("OVERLAY_{}_TGL", magic_enum::enum_name(overlay)), overlayToggle);

        overlayRect.y += smallButtonSize.y + spacing;
    }
}

void UiManager::InitializeElements(PlayerCam &camera)
{
    InitializeSidebar(camera);
    InitializeEscapeMenu(camera);
    InitializeSettingsMenu(camera);
}