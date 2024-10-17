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

    UiManager::AddElement("SETTINGS_MENU", settingsMenu);
}

void InitializeSidebar()
{
    constexpr Vector2 largeButtonSize = Vector2ScreenScale(Vector2(64, 64));
    constexpr Vector2 smallButtonSize = largeButtonSize / 2.;
    constexpr Vector2 spacing = Vector2ScreenScale(Vector2(DEFAULT_PADDING, DEFAULT_PADDING));

    constexpr Rectangle buildButtonRect = Vector2ToRect(Vector2(spacing.x, (1. - largeButtonSize.y) / 2.), largeButtonSize);
    bool isInBuildMode = GameManager::GetCamera().IsInBuildMode();
    auto buildToggle = std::make_shared<UiToggle>(buildButtonRect, isInBuildMode, [](bool state)
                                                  { GameManager::GetCamera().SetBuildModeState(state); });

    constexpr Rectangle buildIconRect = Vector2ToRect(Vector2(buildButtonRect.x, buildButtonRect.y) + largeButtonSize / 8., largeButtonSize * .75);
    buildToggle->AddChild(std::make_shared<UiIcon>(buildIconRect, "ICON", Rectangle(1, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8)));

    std::weak_ptr<UiToggle> weakBuildToggle = buildToggle;
    buildToggle->SetOnUpdate([weakBuildToggle]()
                             { if (auto buildToggle = weakBuildToggle.lock())
                             { buildToggle->SetVisible(GameManager::GetCamera().IsUiClear()); } });

    UiManager::AddElement("BUILD_TGL", buildToggle);

    Rectangle overlayRect = Vector2ToRect(Vector2(spacing.x, (1. + largeButtonSize.y) / 2. + spacing.y), smallButtonSize);

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
                                auto &camera = GameManager::GetCamera();
                                overlayToggle->SetVisible(camera.IsUiClear());
                                overlayToggle->SetToggle(camera.IsOverlay(overlay));
                              } });

        UiManager::AddElement(std::format("OVERLAY_{}_TGL", magic_enum::enum_name(overlay)), overlayToggle);

        overlayRect.y += smallButtonSize.y + spacing.y;
    }
}

void InitializeBuildUi()
{
    auto buildMoveButton = std::make_shared<UiButton>(Rectangle(0, 0, 1, 1), "", nullptr, TextAttrs(), nullptr, true);
    buildMoveButton->SetVisible(false);

    std::weak_ptr<UiButton> weakBuildMoveButton = buildMoveButton;
    buildMoveButton->SetOnUpdate([weakBuildMoveButton]()
                                 { if (auto buildMoveButton = weakBuildMoveButton.lock())
                            { 
                                constexpr Vector2 buttonSize = Vector2(1. / 3., 1. / 3.);
                                auto selectedTile = GameManager::GetSelectedTile();
                                bool visible = GameManager::GetCamera().IsInBuildMode() && selectedTile;
                                if (visible && ToVector2Int(RectToPos(buildMoveButton->GetRect())) != selectedTile->GetPosition()) {
                                    buildMoveButton->SetRect(Vector2ToRect(ToVector2(selectedTile->GetPosition()) + Vector2(1 + buttonSize.x / 2., (1. - buttonSize.y) / 2.), buttonSize));
                                }
                                buildMoveButton->SetVisible(visible);
                            } });

    auto buildMoveIcon = std::make_shared<UiIcon>(Rectangle(0, 0, 1, 1), "ICON", Rectangle(2, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8), nullptr, true);

    std::weak_ptr<UiIcon> weakBuildMoveIcon = buildMoveIcon;
    buildMoveIcon->SetOnUpdate([weakBuildMoveIcon]()
                               { if (auto buildMoveIcon = weakBuildMoveIcon.lock())
                            { 
                                constexpr Vector2 buttonSize = Vector2(1. / 3., 1. / 3.);
                                constexpr double iconSize = 2. / 3.;
                                constexpr Vector2 offset = buttonSize * (1. - iconSize) / 2.;
                                auto selectedTile = GameManager::GetSelectedTile();
                                bool visible = GameManager::GetCamera().IsInBuildMode() && selectedTile;
                                if (visible && ToVector2Int(RectToPos(buildMoveIcon->GetRect())) != selectedTile->GetPosition()) {
                                    buildMoveIcon->SetRect(Vector2ToRect(ToVector2(selectedTile->GetPosition()) + Vector2(1 + buttonSize.x / 2., (1. - buttonSize.y) / 2.) + offset, buttonSize * iconSize));
                                }
                            } });

    buildMoveButton->AddChild(buildMoveIcon);
    UiManager::AddElement("BUILD_MOVE_BTN", buildMoveButton);
}

void UiManager::InitializeElements()
{
    InitializeSidebar();
    InitializeEscapeMenu();
    InitializeSettingsMenu();
    InitializeBuildUi();
}