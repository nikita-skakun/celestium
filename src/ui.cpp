#include "ui.hpp"
#include "ui_manager.hpp"

/**
 * Draws a grid of tiles on the screen based on the current camera position and zoom level.
 *
 * @param camera The current player camera, used to determine the position and zoom level.
 */
void DrawTileGrid(const PlayerCam &camera)
{
    Vector2 screenSize = GetScreenSize();

    // Calculate the top-left corner in world coordinates
    Vector2 topLeft = camera.GetPosition() * TILE_SIZE - screenSize / 2. / camera.GetZoom();

    // Draw vertical lines
    for (int x = (int)(floor(topLeft.x / TILE_SIZE) * TILE_SIZE); x <= (int)(ceil((topLeft.x + screenSize.x / camera.GetZoom()) / TILE_SIZE) * TILE_SIZE); x += (int)TILE_SIZE)
    {
        float screenX = (x - camera.GetPosition().x * TILE_SIZE) * camera.GetZoom() + screenSize.x / 2.;
        DrawLineV(Vector2(screenX, 0), Vector2(screenX, screenSize.y), GRID_COLOR);
    }

    // Draw horizontal lines
    for (int y = (int)(floor(topLeft.y / TILE_SIZE) * TILE_SIZE); y <= (int)(ceil((topLeft.y + screenSize.y / camera.GetZoom()) / TILE_SIZE) * TILE_SIZE); y += (int)TILE_SIZE)
    {
        float screenY = (y - camera.GetPosition().y * TILE_SIZE) * camera.GetZoom() + screenSize.y / 2.;
        DrawLineV(Vector2(0, screenY), Vector2(screenSize.x, screenY), GRID_COLOR);
    }
}

/**
 * Draws a path as a series of lines between waypoints.
 *
 * @param path     A queue of Vector2Int positions representing the path to draw.
 * @param startPos The starting position of the path.
 * @param camera   The PlayerCam used for converting world coordinates to screen coordinates.
 */
void DrawPath(const std::deque<Vector2Int> &path, const Vector2 &startPos, const PlayerCam &camera)
{
    if (path.empty())
        return;

    Vector2 a = startPos + Vector2(.5, .5);

    for (const auto &point : path)
    {
        Vector2 b = ToVector2(point) + Vector2(.5, .5);
        DrawLineV(camera.WorldToScreen(a), camera.WorldToScreen(b), Fade(GREEN, .5));
        a = b;
    }
}

/**
 * Draws the station tiles and direct overlays.
 *
 * @param station The station to draw the tiles of.
 * @param camera  The PlayerCam used for handling overlays and converting coordinates.
 */
void DrawStationTiles(std::shared_ptr<Station> station, const PlayerCam &camera)
{
    if (!station)
        return;

    Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * camera.GetZoom();
    const Texture2D &stationTileset = AssetManager::GetTexture("STATION");

    for (const auto &tilesAtPos : station->tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            Vector2 startPos = camera.WorldToScreen(ToVector2(tile->GetPosition()));
            Rectangle sourceRect = Rectangle(tile->GetSpriteOffset().x, tile->GetSpriteOffset().y, 1, 1) * TILE_SIZE;

            DrawTexturePro(stationTileset, sourceRect, Vector2ToRect(startPos, tileSize), Vector2(), 0, WHITE);

            if (camera.GetOverlay() == PlayerCam::Overlay::OXYGEN)
            {
                if (auto oxygen = tile->GetComponent<OxygenComponent>())
                {
                    Color color = Color(50, 150, 255, oxygen->GetOxygenLevel() / TILE_OXYGEN_MAX * 255 * .8);
                    DrawRectangleV(startPos, tileSize, color);
                }
            }

            if (camera.GetOverlay() == PlayerCam::Overlay::WALL && tile->HasComponent<SolidComponent>())
            {
                DrawRectangleV(startPos, tileSize, Color(255, 0, 0, 64));
            }
        }
    }
}

/**
 * Draws the indirect visual overlays.
 *
 * @param station The station to draw the overlays of.
 * @param camera  The PlayerCam used for handling overlays and converting coordinates.
 */
void DrawStationOverlays(std::shared_ptr<Station> station, const PlayerCam &camera)
{
    if (!station)
        return;

    const Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * camera.GetZoom();
    const Texture2D &stationTileset = AssetManager::GetTexture("STATION");
    const Texture2D &iconTileset = AssetManager::GetTexture("ICON");

    for (const auto &tilesAtPos : station->tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            if (auto decorative = tile->GetComponent<DecorativeComponent>())
            {
                for (const DecorativeTile &dTile : decorative->GetDecorativeTiles())
                {
                    Rectangle sourceRect = Rectangle(dTile.spriteOffset.x, dTile.spriteOffset.y, 1, 1) * TILE_SIZE;
                    Vector2 startPos = camera.WorldToScreen(ToVector2(tile->GetPosition() + dTile.offset));
                    Rectangle destRect = Rectangle(startPos.x, startPos.y, tileSize.x, tileSize.y);

                    DrawTexturePro(stationTileset, sourceRect, destRect, Vector2(), 0, WHITE);
                }
            }

            if (auto door = tile->GetComponent<DoorComponent>())
            {
                Vector2 startPos = camera.WorldToScreen(ToVector2(tile->GetPosition()));
                Rectangle destRect = Vector2ToRect(startPos, tileSize);
                Rectangle doorSourceRect = Rectangle(0, 7, 1, 1) * TILE_SIZE;
                doorSourceRect.height = std::max(19. * door->GetProgress(), 1.);

                Rectangle doorDest1 = destRect;
                doorDest1.height = std::max(19. * door->GetProgress(), 1.) * camera.GetZoom();
                doorDest1.y += 19. * camera.GetZoom() - doorDest1.height;

                DrawTexturePro(stationTileset, doorSourceRect, doorDest1, Vector2(), 0, WHITE);

                Rectangle doorDest2 = destRect;
                doorDest2.width = -doorDest2.width;
                doorDest2.height = -doorDest1.height;
                doorDest2.y -= 19. * camera.GetZoom() + doorDest2.height;

                DrawTexturePro(stationTileset, doorSourceRect, doorDest2, Vector2(-doorDest2.width, 0), 180, WHITE);
            }

            if (camera.GetOverlay() == PlayerCam::Overlay::POWER)
            {
                if (auto powerConsumer = tile->GetComponent<PowerConsumerComponent>())
                {
                    if (!powerConsumer->IsActive())
                    {
                        Vector2 startScreenPos = camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(2. / 3., 0));
                        Rectangle destRect = Vector2ToRect(startScreenPos, tileSize / 3.);
                        Rectangle sourceRect = Rectangle(0, 1, 1, 1) * TILE_SIZE;

                        DrawTexturePro(iconTileset, sourceRect, destRect, Vector2(), 0, Fade(YELLOW, .8));
                    }
                }

                if (auto battery = tile->GetComponent<BatteryComponent>())
                {
                    float barProgress = battery->GetChargeLevel() / battery->GetMaxChargeLevel();
                    Vector2 topLeftPos = camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(1. / 16., 0));
                    Vector2 barStartPos = camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(1. / 16., 1. - barProgress));

                    Vector2 totalSize = Vector2(1. / 8., 1) * TILE_SIZE * camera.GetZoom();
                    Vector2 barSize = Vector2(1. / 8., barProgress) * TILE_SIZE * camera.GetZoom();

                    DrawRectangleV(topLeftPos, totalSize, Color(25, 25, 25, 200));
                    DrawRectangleV(barStartPos, barSize, Fade(YELLOW, .8));
                }

                if (auto powerConnector = tile->GetComponent<PowerConnectorComponent>())
                {
                    auto connections = powerConnector->GetConnections();
                    for (auto &&connection : connections)
                    {
                        if (auto connectionTile = connection->_parent.lock())
                        {
                            DrawLineEx(camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(.5, .5)),
                                       camera.WorldToScreen(ToVector2(connectionTile->GetPosition()) + Vector2(.5, .5)),
                                       POWER_CONNECTION_WIDTH * std::max(camera.GetZoom(), 1.f), POWER_CONNECTION_COLOR);
                        }
                    }
                }
            }
        }
    }
}

void DrawTileOutline(std::shared_ptr<Tile> tile, const PlayerCam &camera)
{
    if (!tile)
        return;

    std::unordered_set<Vector2Int> positions = {tile->GetPosition()};
    if (auto decorative = tile->GetComponent<DecorativeComponent>())
    {
        for (const auto &dTile : decorative->GetDecorativeTiles())
        {
            positions.insert(tile->GetPosition() + dTile.offset);
        }
    }

    Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * camera.GetZoom();
    for (const auto &pos : positions)
    {
        Vector2 startPos = camera.WorldToScreen(ToVector2(pos));
        Rectangle rect = Vector2ToRect(startPos, tileSize);

        std::array<std::pair<Vector2, Vector2>, 4> lines = {
            std::make_pair(Vector2{rect.x, rect.y}, Vector2{rect.x + rect.width, rect.y}),
            std::make_pair(Vector2{rect.x + rect.width, rect.y}, Vector2{rect.x + rect.width, rect.y + rect.height}),
            std::make_pair(Vector2{rect.x, rect.y + rect.height}, Vector2{rect.x + rect.width, rect.y + rect.height}),
            std::make_pair(Vector2{rect.x, rect.y}, Vector2{rect.x, rect.y + rect.height})};

        std::array<Direction, 4> directions = {Direction::N, Direction::E, Direction::S, Direction::W};

        for (size_t i = 0; i < directions.size(); ++i)
        {
            Vector2Int neighborPos = pos + DirectionToVector2Int(directions[i]);

            if (positions.count(neighborPos) == 0)
                DrawLineEx(lines[i].first, lines[i].second, 2, MAGENTA);
        }
    }
}

/**
 * Draws the station's environmental hazards (such as fire).
 *
 * @param station The station to draw the overlays of.
 * @param camera  The PlayerCam used for converting coordinates.
 */
void DrawEnvironmentalHazards(std::shared_ptr<Station> station, const PlayerCam &camera)
{
    if (!station)
        return;

    const Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * camera.GetZoom();
    const Texture2D &fireSpritesheet = AssetManager::GetTexture("FIRE");

    for (const auto &hazard : station->hazards)
    {
        if (const auto fire = std::dynamic_pointer_cast<const FireHazard>(hazard))
        {
            Vector2 fireSize = tileSize * fire->GetRoundedSize();
            Vector2 startPos = camera.WorldToScreen(ToVector2(fire->GetPosition()) + Vector2(1, 1) * ((1. - fire->GetRoundedSize()) / 2.));
            Rectangle sourceRect = Rectangle(GetEvenlySpacedIndex(GetTime(), 8), 0, 1, 1) * TILE_SIZE;

            DrawTexturePro(fireSpritesheet, sourceRect, Vector2ToRect(startPos, fireSize), Vector2(), 0, WHITE);
        }
    }
}

void DrawCrewCircle(const Crew &crew, const Vector2 &drawPosition, bool isSelected, const PlayerCam &camera)
{
    Vector2 crewScreenPos = camera.WorldToScreen(drawPosition + Vector2(.5, .5));

    if (isSelected)
        DrawCircleV(crewScreenPos, (CREW_RADIUS + OUTLINE_SIZE) * camera.GetZoom(), OUTLINE_COLOR);

    DrawCircleV(crewScreenPos, CREW_RADIUS * camera.GetZoom(), crew.IsAlive() ? crew.GetColor() : GRAY);
}

/**
 * Draws the crew members on the screen, accounting for their movement.
 *
 * @param timeSinceFixedUpdate The elapsed time since the last fixed update.
 * @param crewList             List of crew members to be drawn.
 * @param camera               The PlayerCam used for handling hover state and converting coordinates.
 */
void DrawCrew(double timeSinceFixedUpdate, const std::vector<Crew> &crewList, const PlayerCam &camera)
{
    for (const Crew &crew : crewList)
    {
        Vector2 drawPosition = crew.GetPosition();
        auto &taskQueue = crew.GetReadOnlyTaskQueue();

        if (!taskQueue.empty() && taskQueue.front()->GetType() == Task::Type::MOVE)
        {
            const std::shared_ptr<MoveTask> moveTask = std::dynamic_pointer_cast<MoveTask>(taskQueue.front());

            if (!camera.IsInBuildMode() && !moveTask->path.empty())
            {
                DrawPath(moveTask->path, crew.GetPosition(), camera);
                Vector2 nextPosition = ToVector2(moveTask->path.front());

                const float moveDelta = timeSinceFixedUpdate * CREW_MOVE_SPEED;
                const float distanceLeftSq = Vector2DistanceSq(crew.GetPosition(), nextPosition) - moveDelta * moveDelta;
                if (distanceLeftSq <= 0)
                {
                    drawPosition = nextPosition;

                    if (moveTask->path.size() > 1)
                    {
                        Vector2 futurePosition = ToVector2(moveTask->path.at(1));
                        drawPosition += Vector2Normalize(futurePosition - drawPosition) * sqrtf(-distanceLeftSq);
                    }
                }
                else
                {
                    bool canPath = true;
                    if (std::shared_ptr<Station> station = crew.GetCurrentTile()->GetStation())
                        canPath = station->IsDoorFullyOpenAtPos(moveTask->path.front());

                    if (canPath)
                        drawPosition += Vector2Normalize(nextPosition - crew.GetPosition()) * moveDelta;
                }
            }
        }

        DrawCrewCircle(crew, drawPosition, camera.GetSelectedCrew().contains(&crew - &crewList[0]), camera);
    }
}

/**
 * Draws a power connect line or a rectangle selection box based on mouse drag.
 *
 * @param camera   The PlayerCam that stores the mouse drag information.
 */
void DrawDragSelectBox(const PlayerCam &camera)
{
    if (!camera.IsDragging())
        return;

    Vector2 dragStart = camera.WorldToScreen(camera.GetDragStart());
    Vector2 dragEnd = camera.WorldToScreen(camera.GetDragEnd());

    switch (camera.GetDragType())
    {
    case PlayerCam::DragType::SELECT:
        DrawRectangleLinesEx(Vector2ToBoundingBox(dragStart, dragEnd), 1, BLUE);
        break;

    case PlayerCam::DragType::POWER_CONNECT:
        DrawLineEx(dragStart, dragEnd, POWER_CONNECTION_WIDTH * std::max(camera.GetZoom(), 1.f), POWER_CONNECTION_COLOR);
        break;

    default:
        break;
    }
}

/**
 * Displays the current FPS in the top-right corner.
 *
 * @param deltaTime The time taken for the last frame, used to display in milliseconds.
 * @param padding   The padding from the screen edges for positioning the text.
 * @param fontSize  The size of the text to be drawn.
 */
void DrawFpsCounter(float deltaTime, float padding, int fontSize)
{
    const Font &font = AssetManager::GetFont("DEFAULT");
    std::string fpsText = std::format("FPS: {:} ({:.2}ms)", GetFPS(), deltaTime * 1000.);
    const char *text = fpsText.c_str();
    DrawTextEx(font, text, Vector2(GetMonitorWidth(GetCurrentMonitor()) - MeasureTextEx(font, text, fontSize, 1).x - padding, padding), fontSize, 1, UI_TEXT_COLOR);
}

/**
 * Draws a tooltip with a background rectangle at the specified position.
 *
 * @param tooltip  The text to display in the tooltip.
 * @param pos      The position where the tooltip will be drawn.
 * @param padding  The padding around the text within the tooltip background.
 * @param fontSize The size of the text in the tooltip.
 */
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, float padding, int fontSize)
{
    Vector2 screenSize = GetScreenSize();
    int lineCount = 0;
    const char **lines = TextSplit(tooltip.c_str(), '\n', &lineCount);
    const Font &font = AssetManager::GetFont("DEFAULT");

    float textWidth = 0;
    for (int i = 0; i < lineCount; i++)
    {
        textWidth = std::max(textWidth, MeasureTextEx(font, lines[i], fontSize, 1).x);
    }

    Vector2 size = Vector2(textWidth + 2 * padding, lineCount * fontSize + 2 * padding);
    Vector2 tooltipPos = pos;

    if (tooltipPos.x + size.x > screenSize.x)
        tooltipPos.x = screenSize.x - size.x;

    if (tooltipPos.x < 0)
        tooltipPos.x = 0;

    if (tooltipPos.y + size.y > screenSize.y)
        tooltipPos.y = screenSize.y - size.y;
    else if (tooltipPos.y < 0)
        tooltipPos.y = 0;

    DrawRectangleRec(Vector2ToRect(tooltipPos, size), Fade(LIGHTGRAY, .7));

    for (int i = 0; i < lineCount; i++)
    {
        DrawTextEx(font, lines[i], tooltipPos + Vector2(padding, padding + (i * fontSize)), fontSize, 1, DARKGRAY);
    }
}

std::string GetTileInfo(std::shared_ptr<Tile> tile)
{
    std::string tileInfo = " - " + tile->GetName();

    if (auto durability = tile->GetComponent<DurabilityComponent>())
    {
        tileInfo += std::format("\n   + HP: {:.1} / {:.1}", durability->GetHitpoints(), durability->GetMaxHitpoints());
    }
    if (auto door = tile->GetComponent<DoorComponent>())
    {
        tileInfo += std::format("\n   + {}", door->IsOpen() ? "Open" : "Closed");
        tileInfo += std::format("\n   + State: {} ({:.0}%)", door->GetMovementName(), door->GetProgress() * 100.);
    }
    if (auto oxygen = tile->GetComponent<OxygenComponent>())
    {
        tileInfo += std::format("\n   + Oxygen: {:.0}", oxygen->GetOxygenLevel());
    }
    if (auto battery = tile->GetComponent<BatteryComponent>())
    {
        tileInfo += std::format("\n   + Energy: {:.0} / {:.0}", battery->GetChargeLevel(), battery->GetMaxChargeLevel());
    }
    if (auto powerConnector = tile->GetComponent<PowerConnectorComponent>())
    {
        tileInfo += std::format("\n   + Power Connector: {}", magic_enum::enum_flags_name(powerConnector->GetIO()));
    }

    return tileInfo;
}

std::string GetHazardInfo(std::shared_ptr<Hazard> hazard)
{
    std::string hazardInfo = " - " + hazard->GetName();

    if (auto fire = std::dynamic_pointer_cast<const FireHazard>(hazard))
    {
        hazardInfo += std::format("\n   + Size: {:.0f}", fire->GetRoundedSize() / FireHazard::SIZE_INCREMENT);
    }

    return hazardInfo;
}

/**
 * Draws a tooltip about the crew member or station tile under the mouse cursor.
 *
 * @param crewList   A vector of Crew objects, used to retrieve the hovered crew member's information.
 * @param camera     The PlayerCam used for handling hover state and converting coordinates.
 * @param station    A shared pointer to the Station, used to fetch tiles and their components.
 */
void DrawMainTooltip(const std::vector<Crew> &crewList, const PlayerCam &camera, std::shared_ptr<Station> station)
{
    std::string hoverText;
    const Vector2 mousePos = GetMousePosition();

    // Add crew info we are hovering over
    if (!camera.IsInBuildMode() && camera.GetCrewHoverIndex() >= 0)
    {
        const Crew &crew = crewList[camera.GetCrewHoverIndex()];
        hoverText += " - " + crew.GetName();
        if (crew.IsAlive())
        {
            hoverText += std::format("\n   + Health: {:.1}", crew.GetHealth());
            hoverText += std::format("\n   + Oxygen: {:.0}", crew.GetOxygen());
        }
        else
        {
            hoverText += "\n   + DEAD";
        }
    }

    // Add tile info we are hovering over
    if (station)
    {
        Vector2Int tileHoverPos = camera.ScreenToTile(mousePos);
        auto allTiles = station->GetAllTilesAtPosition(tileHoverPos);

        for (const auto &tile : allTiles)
        {
            if (!hoverText.empty())
                hoverText += "\n";

            hoverText += GetTileInfo(tile);
        }

        if (!camera.IsInBuildMode())
        {
            auto hazards = station->GetHazardsAtPosition(tileHoverPos);
            for (auto &&hazard : hazards)
            {
                if (!hoverText.empty())
                    hoverText += "\n";

                hoverText += GetHazardInfo(hazard);
            }
        }
    }

    if (hoverText.empty())
        return;

    DrawTooltip(hoverText, mousePos);
}

void InitializeEscapeMenu(GameState &state, PlayerCam &camera)
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
    auto exitButton = std::make_shared<UiButton>(exitButtonRect, "Exit", [&state]()
                                                 { SetBit(state, false, GameState::RUNNING); });
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

void UiManager::InitializeElements(GameState &state, PlayerCam &camera)
{
    InitializeSidebar(camera);
    InitializeEscapeMenu(state, camera);
    InitializeSettingsMenu(camera);
}