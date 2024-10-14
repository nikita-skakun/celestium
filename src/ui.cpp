#include "ui.hpp"

/**
 * Draws a grid of tiles on the screen based on the current camera position and zoom level.
 *
 * @param camera The current player camera, used to determine the position and zoom level.
 */
void DrawTileGrid(const PlayerCam &camera)
{
    Vector2 screenSize = GetScreenSize();

    // Calculate the top-left corner in world coordinates
    Vector2 topLeft = camera.GetPosition() * TILE_SIZE - screenSize / 2.f / camera.GetZoom();

    // Draw vertical lines
    for (int x = (int)(floor(topLeft.x / TILE_SIZE) * TILE_SIZE); x <= (int)(ceil((topLeft.x + screenSize.x / camera.GetZoom()) / TILE_SIZE) * TILE_SIZE); x += (int)TILE_SIZE)
    {
        float screenX = (x - camera.GetPosition().x * TILE_SIZE) * camera.GetZoom() + screenSize.x / 2.f;
        DrawLineV(Vector2(screenX, 0), Vector2(screenX, screenSize.y), GRID_COLOR);
    }

    // Draw horizontal lines
    for (int y = (int)(floor(topLeft.y / TILE_SIZE) * TILE_SIZE); y <= (int)(ceil((topLeft.y + screenSize.y / camera.GetZoom()) / TILE_SIZE) * TILE_SIZE); y += (int)TILE_SIZE)
    {
        float screenY = (y - camera.GetPosition().y * TILE_SIZE) * camera.GetZoom() + screenSize.y / 2.f;
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

    Vector2 a = startPos + Vector2(.5f, .5f);

    for (const auto &point : path)
    {
        Vector2 b = ToVector2(point) + Vector2(.5f, .5f);
        DrawLineV(camera.WorldToScreen(a), camera.WorldToScreen(b), Fade(GREEN, .5f));
        a = b;
    }
}

/**
 * Draws the station tiles and direct overlays.
 *
 * @param station        The station to draw the tiles of.
 * @param stationTileset A texture containing all station tile assets.
 * @param camera         The PlayerCam used for handling overlays and converting coordinates.
 */
void DrawStationTiles(std::shared_ptr<Station> station, const Texture2D &stationTileset, const PlayerCam &camera)
{
    if (!station)
        return;

    Vector2 tileSize = Vector2(1.f, 1.f) * TILE_SIZE * camera.GetZoom();

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
                    Color color = Color(50, 150, 255, oxygen->GetOxygenLevel() / TILE_OXYGEN_MAX * 255 * .8f);
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
 * @param station        The station to draw the overlays of.
 * @param stationTileset A texture containing all station tile assets.
 * @param camera         The PlayerCam used for handling overlays and converting coordinates.
 */
void DrawStationOverlays(std::shared_ptr<Station> station, const Texture2D &stationTileset, const Texture2D &iconTileset, const PlayerCam &camera)
{
    if (!station)
        return;

    const Vector2 tileSize = Vector2(1.f, 1.f) * TILE_SIZE * camera.GetZoom();

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
                doorSourceRect.height = std::max(19.f * door->GetProgress(), 1.f);

                Rectangle doorDest1 = destRect;
                doorDest1.height = std::max(19.f * door->GetProgress(), 1.f) * camera.GetZoom();
                doorDest1.y += 19.f * camera.GetZoom() - doorDest1.height;

                DrawTexturePro(stationTileset, doorSourceRect, doorDest1, Vector2(), 0, WHITE);

                Rectangle doorDest2 = destRect;
                doorDest2.width = -doorDest2.width;
                doorDest2.height = -doorDest1.height;
                doorDest2.y -= 19.f * camera.GetZoom() + doorDest2.height;

                DrawTexturePro(stationTileset, doorSourceRect, doorDest2, Vector2(-doorDest2.width, 0.f), 180.f, WHITE);
            }

            if (camera.GetOverlay() == PlayerCam::Overlay::POWER)
            {
                if (auto powerConsumer = tile->GetComponent<PowerConsumerComponent>())
                {
                    if (!powerConsumer->IsActive())
                    {
                        Vector2 startScreenPos = camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(.66f, 0.f));
                        Rectangle destRect = Vector2ToRect(startScreenPos, tileSize / 3.f);
                        Rectangle sourceRect = Rectangle(0, 1, 1, 1) * TILE_SIZE;

                        DrawTexturePro(iconTileset, sourceRect, destRect, Vector2(), 0, Fade(YELLOW, .8f));
                    }
                }

                if (auto battery = tile->GetComponent<BatteryComponent>())
                {
                    float barProgress = battery->GetChargeLevel() / battery->GetMaxChargeLevel();
                    Vector2 topLeftPos = camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(1.f / 16.f, 0.f));
                    Vector2 barStartPos = camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(1.f / 16.f, 1.f - barProgress));

                    Vector2 totalSize = Vector2(1.f / 8.f, 1.f) * TILE_SIZE * camera.GetZoom();
                    Vector2 barSize = Vector2(1.f / 8.f, barProgress) * TILE_SIZE * camera.GetZoom();

                    DrawRectangleV(topLeftPos, totalSize, Color(25, 25, 25, 200));
                    DrawRectangleV(barStartPos, barSize, Fade(YELLOW, .8f));
                }

                if (auto powerConnector = tile->GetComponent<PowerConnectorComponent>())
                {
                    auto connections = powerConnector->GetConnections();
                    for (auto &&connection : connections)
                    {
                        if (auto connectionTile = connection->_parent.lock())
                        {
                            DrawLineEx(camera.WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(.5f, .5f)),
                                       camera.WorldToScreen(ToVector2(connectionTile->GetPosition()) + Vector2(.5f, .5f)),
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

    Vector2 tileSize = Vector2(1.f, 1.f) * TILE_SIZE * camera.GetZoom();
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
                DrawLineEx(lines[i].first, lines[i].second, 2.f, MAGENTA);
        }
    }
}

/**
 * Draws the station's environmental hazards (such as fire).
 *
 * @param station         The station to draw the overlays of.
 * @param fireSpritesheet A texture containing the fire animation spritesheet.
 * @param camera          The PlayerCam used for converting coordinates.
 */
void DrawEnvironmentalHazards(std::shared_ptr<Station> station, const Texture2D &fireSpritesheet, const PlayerCam &camera)
{
    if (!station)
        return;

    const Vector2 tileSize = Vector2(1.f, 1.f) * TILE_SIZE * camera.GetZoom();

    for (const auto &hazard : station->hazards)
    {
        if (const auto fire = std::dynamic_pointer_cast<const FireHazard>(hazard))
        {
            Vector2 fireSize = tileSize * fire->GetRoundedSize();
            Vector2 startPos = camera.WorldToScreen(ToVector2(fire->GetPosition()) + Vector2(1.f, 1.f) * ((1.f - fire->GetRoundedSize()) / 2.f));
            Rectangle sourceRect = Rectangle(GetEvenlySpacedIndex(GetTime(), 8), 0, 1, 1) * TILE_SIZE;

            DrawTexturePro(fireSpritesheet, sourceRect, Vector2ToRect(startPos, fireSize), Vector2(), 0, WHITE);
        }
    }
}

void DrawCrewCircle(const Crew &crew, const Vector2 &drawPosition, bool isSelected, const PlayerCam &camera)
{
    Vector2 crewScreenPos = camera.WorldToScreen(drawPosition + Vector2(.5f, .5f));

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
        DrawRectangleLinesEx(Vector2ToBoundingBox(dragStart, dragEnd), 1.f, BLUE);
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
 * @param font      The font to use, defaults to RayLib's default.
 */
void DrawFpsCounter(float deltaTime, float padding, int fontSize, const Font &font)
{
    std::string fpsText = std::format("FPS: {:} ({:.2f}ms)", GetFPS(), deltaTime * 1000.f);
    const char *text = fpsText.c_str();
    DrawTextEx(font, text, Vector2(GetMonitorWidth(GetCurrentMonitor()) - MeasureTextEx(font, text, fontSize, 1).x - padding, padding), fontSize, 1, UI_TEXT_COLOR);
}

/**
 * Draws a tooltip with a background rectangle at the specified position.
 *
 * @param tooltip  The text to display in the tooltip.
 * @param pos      The position where the tooltip will be drawn.
 * @param font     The font to use when drawing the tooltip, defaults to RayLib's default.
 * @param padding  The padding around the text within the tooltip background.
 * @param fontSize The size of the text in the tooltip.
 */
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, const Font &font, float padding, int fontSize)
{
    Vector2 screenSize = GetScreenSize();
    int lineCount = 0;
    const char **lines = TextSplit(tooltip.c_str(), '\n', &lineCount);

    float textWidth = 0;
    for (int i = 0; i < lineCount; i++)
    {
        textWidth = std::max(textWidth, MeasureTextEx(font, lines[i], fontSize, 1).x);
    }

    Vector2 size = Vector2(textWidth + 2.f * padding, lineCount * fontSize + 2.f * padding);
    Vector2 tooltipPos = pos;

    if (tooltipPos.x + size.x > screenSize.x)
        tooltipPos.x = screenSize.x - size.x;

    if (tooltipPos.x < 0)
        tooltipPos.x = 0;

    if (tooltipPos.y + size.y > screenSize.y)
        tooltipPos.y = screenSize.y - size.y;
    else if (tooltipPos.y < 0)
        tooltipPos.y = 0;

    DrawRectangleRec(Vector2ToRect(tooltipPos, size), Fade(LIGHTGRAY, 0.7f));

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
        tileInfo += std::format("\n   + HP: {:.1f} / {:.1f}", durability->GetHitpoints(), durability->GetMaxHitpoints());
    }
    if (auto door = tile->GetComponent<DoorComponent>())
    {
        tileInfo += std::format("\n   + {}", door->IsOpen() ? "Open" : "Closed");
        tileInfo += std::format("\n   + State: {} ({:.0f}%)", door->GetMovementName(), door->GetProgress() * 100.f);
    }
    if (auto oxygen = tile->GetComponent<OxygenComponent>())
    {
        tileInfo += std::format("\n   + Oxygen: {:.0f}", oxygen->GetOxygenLevel());
    }
    if (auto battery = tile->GetComponent<BatteryComponent>())
    {
        tileInfo += std::format("\n   + Energy: {:.0f} / {:.0f}", battery->GetChargeLevel(), battery->GetMaxChargeLevel());
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
 * @param font       The font to use when drawing the tooltip, defaults to RayLib's default.
 */
void DrawMainTooltip(const std::vector<Crew> &crewList, const PlayerCam &camera, std::shared_ptr<Station> station, const Font &font)
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
            hoverText += std::format("\n   + Health: {:.1f}", crew.GetHealth());
            hoverText += std::format("\n   + Oxygen: {:.0f}", crew.GetOxygen());
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

    DrawTooltip(hoverText, mousePos, font);
}

void DrawEscapeMenu(GameState &state, PlayerCam &camera, const Font &font)
{
    constexpr int buttonCount = 3;
    constexpr float buttonWidth = 120.f;
    constexpr float buttonSpacing = 20.f;
    constexpr float buttonHeight = DEFAULT_FONT_SIZE + buttonSpacing;
    constexpr float totalButtonHeight = buttonCount * buttonHeight + (buttonCount - 1) * buttonSpacing;

    // Calculate menu dimensions
    constexpr Vector2 menuSize = Vector2(buttonWidth + buttonSpacing * 2.f, totalButtonHeight + buttonSpacing * 2.f);

    // GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_FONT_SIZE);
    // GuiSetFont(font);

    Vector2 screenSize = GetScreenSize();
    Vector2 menuPos = screenSize / 2.f - menuSize / 2.f;

    // Darken the background to draw focus to the UI
    DrawRectangleV(Vector2(0.f, 0.f), screenSize, Fade(BLACK, 0.2f));

    // Draw a rectangle for the menu background
    DrawRectangleV(menuPos, menuSize, Fade(BLACK, 0.4f));

    // Dynamically position buttons in the center of the menu
    float firstButtonPosY = menuPos.y + (menuSize.y - totalButtonHeight) / 2.f;
    float buttonPosX = screenSize.x / 2 - buttonWidth / 2;

    // // Resume Button
    // if ( GuiButton(Rectangle(buttonPosX, firstButtonPosY, buttonWidth, buttonHeight), "Resume"))
    //     camera.SetUiState(PlayerCam::UiState::NONE);

    // // Settings Button
    // if ( GuiButton(Rectangle(buttonPosX, firstButtonPosY + (buttonHeight + buttonSpacing), buttonWidth, buttonHeight), "Settings"))
    //     camera.SetUiState(PlayerCam::UiState::SETTINGS_MENU);

    // // Exit Button
    // if ( GuiButton(Rectangle(buttonPosX, firstButtonPosY + 2 * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight), "Exit"))
    //     SetBit(state, false, GameState::RUNNING);
}

void DrawSettingsMenu(const Font &font)
{
    // GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_FONT_SIZE);
    // GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    // GuiSetFont(font);

    Vector2 screenSize = GetScreenSize();

    // Calculate menu dimensions
    Vector2 menuSize = screenSize * 2.f / 3.f;

    // Calculate the position of the menu
    Vector2 menuPos = screenSize / 2.f - menuSize / 2.f;

    // Darken the background to draw focus to the UI
    DrawRectangleV(Vector2(0.f, 0.f), screenSize, Fade(BLACK, 0.2f));

    // Draw a rectangle for the menu background
    DrawRectangleV(menuPos, menuSize, Fade(BLACK, 0.4f));

    float halfPanelSize = menuSize.x / 2.f - DEFAULT_PADDING * 1.5f;

    Rectangle monitorTextRect = Rectangle(menuPos.x + DEFAULT_PADDING, menuPos.y + DEFAULT_PADDING, halfPanelSize, DEFAULT_FONT_SIZE + DEFAULT_PADDING);
    // GuiStatusBar(monitorTextRect, "Render Monitor: ");

    std::string monitorNames;
    for (int i = 0; i < GetMonitorCount(); i++)
    {
        if (i > 0)
            monitorNames += ";";
        monitorNames += GetMonitorName(i);
    }

    int selectedMonitor = GetCurrentMonitor();
    Rectangle monitorNameRect = Rectangle(monitorTextRect.x + DEFAULT_PADDING + halfPanelSize, menuPos.y + DEFAULT_PADDING, halfPanelSize, DEFAULT_FONT_SIZE + DEFAULT_PADDING);
    // GuiComboBox(monitorNameRect, monitorNames.c_str(), &selectedMonitor);
    if (selectedMonitor != GetCurrentMonitor())
    {
        SetWindowMonitor(selectedMonitor);
        SetTargetFPS(GetMonitorRefreshRate(selectedMonitor));
    }
}

void DrawUi(GameState &state, PlayerCam &camera, const Font &font)
{
    PlayerCam::UiState uiState = camera.GetUiState();
    auto &uiManager = UiManager::GetInstance();

    uiManager.GetElement("BUILD_TGL")->SetVisibility(camera.IsUiClear());

    for (auto overlay : magic_enum::enum_values<PlayerCam::Overlay>())
    {
        if (overlay != PlayerCam::Overlay::NONE)
            uiManager.GetElement(std::format("OVERLAY_{}_TGL", magic_enum::enum_name(overlay)))->SetVisibility(camera.IsUiClear());
    }

    switch (uiState)
    {
    case PlayerCam::UiState::ESC_MENU:
        DrawEscapeMenu(state, camera, font);
        break;

    case PlayerCam::UiState::SETTINGS_MENU:
        DrawSettingsMenu(font);

    default:
        break;
    }
}

void InitializeUiElements(const Texture2D &iconTileset, PlayerCam &camera)
{
    auto &uiManager = UiManager::GetInstance();

    Vector2 screenSize = GetScreenSize();
    constexpr float largeButtonSize = 64.f;
    constexpr float smallButtonSize = 32.f;

    Rectangle buildButtonRect = Rectangle(DEFAULT_PADDING, (screenSize.y - largeButtonSize) / 2.f, largeButtonSize, largeButtonSize);
    bool isInBuildMode = camera.IsInBuildMode();
    auto buildToggle = std::make_shared<UiToggle>(buildButtonRect, isInBuildMode, [&camera](bool state)
                                                  { camera.SetBuildModeState(state); });

    Rectangle buildIconRect = Rectangle(buildButtonRect.x + 8.f, buildButtonRect.y + 8.f, 48.f, 48.f);
    buildToggle->AddChild(std::make_shared<UiIcon>(buildIconRect, iconTileset, Rectangle(1, 1, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8f)));

    uiManager.AddElement("BUILD_TGL", buildToggle);

    Rectangle overlayRect = Rectangle(DEFAULT_PADDING, (screenSize.y + largeButtonSize) / 2.f + DEFAULT_PADDING, smallButtonSize, smallButtonSize);

    for (auto overlay : magic_enum::enum_values<PlayerCam::Overlay>())
    {
        if (overlay == PlayerCam::Overlay::NONE)
            continue;

        bool isOverlayActive = camera.IsOverlay(overlay);
        auto overlayToggle = std::make_shared<UiToggle>(overlayRect, isOverlayActive, [&camera, overlay](bool state)
                                                        { camera.ToggleOverlay(overlay); });

        Rectangle iconRect = Rectangle(overlayRect.x + 8.f, overlayRect.y + 8.f, 16.f, 16.f);
        float iconIndex = (float)(magic_enum::enum_underlying<PlayerCam::Overlay>(overlay) - 1);
        overlayToggle->AddChild(std::make_shared<UiIcon>(iconRect, iconTileset, Rectangle(iconIndex, 0, 1, 1) * TILE_SIZE, Fade(DARKGRAY, .8f)));

        uiManager.AddElement(std::format("OVERLAY_{}_TGL", magic_enum::enum_name(overlay)), overlayToggle);

        overlayRect.y += smallButtonSize + DEFAULT_PADDING;
    }
}