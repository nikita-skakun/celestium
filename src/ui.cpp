#include "action.hpp"
#include "asset_manager.hpp"
#include "crew.hpp"
#include "game_state.hpp"
#include "station.hpp"
#include "ui_manager.hpp"
#include "ui.hpp"

Color GetTileTint(const std::shared_ptr<Tile> &tile)
{
    Color tint = WHITE;
    // if (GameManager::IsInBuildMode() && GameManager::IsTileSelected(tile))
    //     tint = ColorLerp(WHITE, TILE_SELECTION_TINT, Oscillate(GetTime(), .5));
    // else if (GameManager::IsInBuildMode() && GameManager::GetMoveTile() == tile)
    //     tint = Fade(WHITE, .5);

    return tint;
}

/**
 * Draws a grid of tiles on the screen based on the current camera position and zoom level.
 */
void DrawTileGrid()
{
    Vector2 screenSize = GetScreenSize();
    auto &camera = GameManager::GetCamera();

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
 */
void DrawPath(const std::deque<Vector2Int> &path, const Vector2 &startPos)
{
    if (path.empty())
        return;

    Vector2 a = startPos;

    for (const auto &point : path)
    {
        Vector2 b = ToVector2(point);
        DrawLineEx(GameManager::WorldToScreen(a), GameManager::WorldToScreen(b), 3, Fade(GREEN, .5));
        a = b;
    }
}

/**
 * Draws the station tiles and direct overlays.
 */
void DrawStationTiles()
{
    auto station = GameManager::GetStation();
    if (!station)
        return;

    auto &camera = GameManager::GetCamera();
    Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * camera.GetZoom();

    for (const auto &tilesAtPos : station->tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            float rotation = 0;
            if (auto rotatable = tile->GetComponent<RotatableComponent>())
                rotation = RotationToAngle(rotatable->GetRotation());

            if (auto sprite = tile->GetSprite())
                sprite->Draw(tile->GetPosition(), GetTileTint(tile), rotation);

            Vector2 startPos = GameManager::WorldToScreen(ToVector2(tile->GetPosition()) - Vector2(.5, .5));

            if (camera.IsOverlay(PlayerCam::Overlay::OXYGEN))
            {
                if (auto oxygen = tile->GetComponent<OxygenComponent>())
                {
                    Color color = Color(50, 150, 255, oxygen->GetOxygenLevel() / TILE_OXYGEN_MAX * 255 * .8);
                    DrawRectangleV(startPos, tileSize, color);
                }
            }

            if (camera.IsOverlay(PlayerCam::Overlay::WALL) && tile->HasComponent<SolidComponent>())
            {
                DrawRectangleV(startPos, tileSize, Color(255, 0, 0, 64));
            }
        }
    }
}

/**
 * Draws the indirect visual overlays.
 */
void DrawStationOverlays()
{
    auto station = GameManager::GetStation();
    if (!station)
        return;

    float zoom = GameManager::GetCamera().GetZoom();
    const Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * zoom;
    bool isPowerOverlay = GameManager::GetCamera().IsOverlay(PlayerCam::Overlay::POWER);
    const Texture2D &stationTileset = AssetManager::GetTexture("STATION");
    const Texture2D &iconTileset = AssetManager::GetTexture("ICON");

    for (const auto &tilesAtPos : station->tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            Color tint = GetTileTint(tile);

            float rotation = 0;
            if (auto rotatable = tile->GetComponent<RotatableComponent>())
                rotation = RotationToAngle(rotatable->GetRotation());

            if (auto decorative = tile->GetComponent<DecorativeComponent>())
            {
                for (const auto &dTile : decorative->GetDecorativeTiles())
                {
                    dTile->Draw(tile->GetPosition(), tint, rotation);
                }
            }

            if (auto door = tile->GetComponent<DoorComponent>())
            {
                Vector2 startPos = GameManager::WorldToScreen(tile->GetPosition());
                Rectangle destRect = Vector2ToRect(startPos, tileSize);
                destRect.height = std::max(25. * door->GetProgress(), 1.) * zoom;
                Rectangle doorSourceRect = Rectangle(0, 7, 1, 1) * TILE_SIZE;
                doorSourceRect.height = std::max(25. * door->GetProgress(), 1.);

                Vector2 pivot = Vector2(tileSize.x / 2., destRect.height - 25. * zoom);

                DrawTexturePro(stationTileset, doorSourceRect, destRect, pivot, rotation, tint);

                Rectangle doorSourceRect2 = doorSourceRect;
                doorSourceRect2.width = -doorSourceRect2.width;

                DrawTexturePro(stationTileset, doorSourceRect2, destRect, pivot, rotation + 180., tint);
            }

            if (isPowerOverlay && magic_enum::enum_flags_test_any(tile->GetHeight(), TileDef::Height::POWER))
            {
                if (auto connector = tile->GetComponent<PowerConnectorComponent>())
                {
                    auto grid = connector->GetPowerGrid();
                    Color gridColor = grid ? grid->GetDebugColor() : Color(200, 200, 200, 128);
                    DrawCircleV(GameManager::WorldToScreen(tilesAtPos.first), 3.0f * zoom, gridColor);
                }
            }

            if (tile->HasComponent<PowerConsumerComponent>() && !tile->IsActive())
            {
                Vector2 startScreenPos = GameManager::WorldToScreen(ToVector2(tile->GetPosition()) + Vector2(2. / 3., 0));
                Rectangle destRect = Vector2ToRect(startScreenPos, tileSize / 3.);
                Rectangle sourceRect = Rectangle(0, 1, 1, 1) * TILE_SIZE;

                DrawTexturePro(iconTileset, sourceRect, destRect, tileSize / 2., 0, Fade(YELLOW, .8));
            }

            if (auto battery = tile->GetComponent<BatteryComponent>())
            {
                float barProgress = battery->GetChargeLevel() / battery->GetMaxChargeLevel();
                Vector2 topLeftPos = GameManager::WorldToScreen(ToVector2(tile->GetPosition()) - Vector2(.5 - 1. / 16., .5));
                Vector2 barStartPos = GameManager::WorldToScreen(ToVector2(tile->GetPosition()) - Vector2(.5 - 1. / 16., barProgress - .5));

                Vector2 totalSize = Vector2(1. / 8., 1) * TILE_SIZE * zoom;
                Vector2 barSize = Vector2(1. / 8., barProgress) * TILE_SIZE * zoom;

                DrawRectangleV(topLeftPos, totalSize, Color(25, 25, 25, 200));
                DrawRectangleV(barStartPos, barSize, Fade(YELLOW, .8));
            }
        }
    }

    if (GameManager::IsInBuildMode() && GameManager::IsHorizontalSymmetry())
    {
        Vector2 screenPos = GameManager::WorldToScreen(Vector2(0, 0));
        screenPos.y -= .5 * TILE_SIZE * zoom;
        DrawLineEx(Vector2(0, screenPos.y), Vector2(GetScreenSize().x, screenPos.y), 2, BLUE);
    }

    if (GameManager::IsInBuildMode() && GameManager::IsVerticalSymmetry())
    {
        Vector2 screenPos = GameManager::WorldToScreen(Vector2(0, 0));
        screenPos.x -= .5 * TILE_SIZE * zoom;
        DrawLineEx(Vector2(screenPos.x, 0), Vector2(screenPos.x, GetScreenSize().y), 2, BLUE);
    }
}

void DrawTileOutline(const std::shared_ptr<Tile> &tile, Color color)
{
    if (!tile)
        return;

    auto &camera = GameManager::GetCamera();
    std::unordered_set<Vector2Int> positions = {tile->GetPosition()};
    if (auto decorative = tile->GetComponent<DecorativeComponent>())
    {
        for (const auto &dTile : decorative->GetDecorativeTiles())
        {
            positions.insert(tile->GetPosition() + dTile->GetOffsetFromMainTile());
        }
    }

    Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * camera.GetZoom();
    for (const auto &pos : positions)
    {
        Vector2 startPos = GameManager::WorldToScreen(ToVector2(pos) - Vector2(.5, .5));
        Rectangle rect = Vector2ToRect(startPos, tileSize);

        std::array<std::pair<Vector2, Vector2>, 4> lines = {
            std::make_pair(Vector2{rect.x, rect.y}, Vector2{rect.x + rect.width, rect.y}),
            std::make_pair(Vector2{rect.x + rect.width, rect.y}, Vector2{rect.x + rect.width, rect.y + rect.height}),
            std::make_pair(Vector2{rect.x, rect.y + rect.height}, Vector2{rect.x + rect.width, rect.y + rect.height}),
            std::make_pair(Vector2{rect.x, rect.y}, Vector2{rect.x, rect.y + rect.height})};

        for (size_t i = 0; i < CARDINAL_DIRECTIONS.size(); ++i)
        {
            Vector2Int neighborPos = pos + DirectionToVector2Int(CARDINAL_DIRECTIONS[i]);

            if (positions.count(neighborPos) == 0)
                DrawLineEx(lines[i].first, lines[i].second, 3, color);
        }
    }
}

/**
 * Draws the station's environmental effects (such as fire or foam).
 */
void DrawEnvironmentalEffects()
{
    auto station = GameManager::GetStation();
    if (!station)
        return;

    for (const auto &effect : station->effects)
    {
        effect->Render();
    }
}

void DrawCrewCircle(const std::shared_ptr<Crew> &crew, const Vector2 &drawPosition, bool isSelected)
{
    auto &camera = GameManager::GetCamera();
    Vector2 crewScreenPos = GameManager::WorldToScreen(drawPosition);

    if (isSelected)
        DrawCircleV(crewScreenPos, (CREW_RADIUS + OUTLINE_SIZE) * camera.GetZoom(), OUTLINE_COLOR);

    DrawCircleV(crewScreenPos, CREW_RADIUS * camera.GetZoom(), crew->IsAlive() ? crew->GetColor() : GRAY);
}

/**
 * Draws the crew members on the screen, accounting for their movement.
 */
void DrawCrew()
{
    auto &crewList = GameManager::GetCrewList();
    for (const auto &crew : crewList)
    {
        Vector2 drawPosition = crew->GetPosition();
        auto &actionQueue = crew->GetReadOnlyActionQueue();

        if (!actionQueue.empty() && actionQueue.front()->GetType() == Action::Type::MOVE)
        {
            const auto moveAction = std::dynamic_pointer_cast<MoveAction>(actionQueue.front());

            if (!GameManager::IsInBuildMode() && !moveAction->path.empty())
            {
                DrawPath(moveAction->path, crew->GetPosition());
                Vector2 nextPosition = ToVector2(moveAction->path.front());

                const float moveDelta = GameManager::GetTimeSinceFixedUpdate() * CREW_MOVE_SPEED;
                const float distanceLeftSq = Vector2DistanceSq(crew->GetPosition(), nextPosition) - moveDelta * moveDelta;
                if (distanceLeftSq <= 0)
                {
                    drawPosition = nextPosition;

                    if (moveAction->path.size() > 1)
                    {
                        Vector2 futurePosition = ToVector2(moveAction->path.at(1));
                        drawPosition += Vector2Normalize(futurePosition - drawPosition) * sqrtf(-distanceLeftSq);
                    }
                }
                else
                {
                    bool canPath = true;
                    if (std::shared_ptr<Station> station = crew->GetCurrentTile()->GetStation())
                        canPath = station->IsDoorFullyOpenAtPos(moveAction->path.front());

                    if (canPath)
                        drawPosition += Vector2Normalize(nextPosition - crew->GetPosition()) * moveDelta;
                }
            }
        }

        const auto &selectedCrewList = GameManager::GetSelectedCrew();
        bool isSelected = std::find_if(selectedCrewList.begin(), selectedCrewList.end(), [crew](std::weak_ptr<Crew> _crew)
                                       { return !_crew.expired() && _crew.lock() == crew; }) != selectedCrewList.end();
        DrawCrewCircle(crew, drawPosition, isSelected);
    }
}

void DrawCrewActionProgress()
{
    const auto &crewList = GameManager::GetCrewList();
    for (const auto &crew : crewList)
    {
        if (!crew->IsAlive() || crew->GetReadOnlyActionQueue().empty())
            continue;

        const auto &action = crew->GetReadOnlyActionQueue().front();

        switch (action->GetType())
        {
        case Action::Type::EXTINGUISH:
        {
            const auto extinguishAction = std::dynamic_pointer_cast<ExtinguishAction>(action);

            const Vector2 barPos = GameManager::WorldToScreen(ToVector2(extinguishAction->GetTargetPosition()) - Vector2(.5 - .05, .5 - .85));
            const Vector2 barSize = Vector2(extinguishAction->GetProgress() * .9, .1) * TILE_SIZE * GameManager::GetCamera().GetZoom();
            DrawRectangleV(barPos, barSize, Fade(RED, .8));
        }
        break;

        default:
            break;
        }
    }
}

/**
 * Draws a power connect line or a rectangle selection box based on mouse drag.
 */
void DrawDragSelectBox()
{
    auto &camera = GameManager::GetCamera();
    if (!camera.IsDragging())
        return;

    Vector2 dragStart = GameManager::WorldToScreen(camera.GetDragStart() - Vector2(.5, .5));
    Vector2 dragEnd = GameManager::WorldToScreen(camera.GetDragEnd() - Vector2(.5, .5));

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
 */
void DrawFpsCounter()
{
    float deltaTime = GetFrameTime();
    const Font &font = AssetManager::GetFont("DEFAULT");
    std::string fpsText = std::format("FPS: {:} ({:.2f}ms)", GetFPS(), deltaTime * 1000.);
    const char *text = fpsText.c_str();
    DrawTextEx(font, text, Vector2(GetScreenSize().x - MeasureTextEx(font, text, DEFAULT_FONT_SIZE, 1).x - DEFAULT_PADDING, DEFAULT_PADDING), DEFAULT_FONT_SIZE, 1, UI_TEXT_COLOR);
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
    char **lines = TextSplit(tooltip.c_str(), '\n', &lineCount);
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

/**
 * Draws a tooltip about the crew member or station tile under the mouse cursor.
 */
void DrawMainTooltip()
{
    std::string hoverText;
    const Vector2 mousePos = GetMousePosition();

    if (!GameManager::IsInBuildMode())
    {
        auto &hoveredCrew = GameManager::GetHoveredCrew();

        for (const auto &_crew : hoveredCrew)
        {
            auto crew = _crew.lock();
            if (!crew)
                continue;

            if (!hoverText.empty())
                hoverText += "\n";

            hoverText += crew->GetInfo();
        }
    }

    if (auto station = GameManager::GetStation())
    {
        Vector2Int tileHoverPos = GameManager::ScreenToTile(mousePos);
        auto allTiles = station->GetAllTilesAtPosition(tileHoverPos);

        for (const auto &tile : allTiles)
        {
            if (!hoverText.empty())
                hoverText += "\n";

            hoverText += tile->GetInfo();
        }

        if (!GameManager::IsInBuildMode())
        {
            auto effects = station->GetEffectsAtPosition(tileHoverPos);
            for (auto &&effect : effects)
            {
                if (!hoverText.empty())
                    hoverText += "\n";

                hoverText += effect->GetInfo();
            }
        }
    }

    if (hoverText.empty())
        return;

    DrawTooltip(hoverText, mousePos);
}

void DrawBuildUi()
{
    auto station = GameManager::GetStation();
    if (!station || UiManager::IsMouseOverUiElement())
        return;

    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());

    std::shared_ptr<Tile> hoveredTile = nullptr;
    // if (auto moveTile = GameManager::GetMoveTile())
    // {
    //     if (auto sprite = moveTile->GetSprite())
    //         sprite->Draw(cursorPos, Fade(WHITE, .5));
    // }
    // else
    auto allTiles = station->GetAllTilesAtPosition(cursorPos);
    if (!allTiles.empty())
    {
        hoveredTile = allTiles.at(allTiles.size() - 1);
        DrawTileOutline(hoveredTile, DARKPURPLE);
    }
}
