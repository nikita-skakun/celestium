#include "action.hpp"
#include "asset_manager.hpp"
#include "component.hpp"
#include "crew.hpp"
#include "env_effect.hpp"
#include "game_state.hpp"
#include "lua_bindings.hpp"
#include "particle_system.hpp"
#include "planned_task.hpp"
#include "power_grid.hpp"
#include "sprite.hpp"
#include "station.hpp"
#include "tile.hpp"
#include "ui_manager.hpp"
#include "ui.hpp"

struct RenderParticleSystem
{
    std::shared_ptr<ParticleSystem> system;
    std::string psysId;
    sol::protected_function onCreateFunc;
    sol::protected_function onUpdateFunc;
    sol::protected_function onDeleteFunc;
    std::shared_ptr<Effect> effectRef;
    bool deleteCalled = false;
};

struct StarfieldParticle
{
    u_int16_t x, y;
    u_char z, size;
    Color color;
};

std::unordered_map<uint64_t, std::vector<RenderParticleSystem>> g_renderSystems;
std::vector<StarfieldParticle> g_starfieldParticles;

Color GetTileTint(const std::shared_ptr<Tile> &)
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
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;

    auto &camera = GameManager::GetCamera();
    const float zoom = camera.GetZoom();
    const Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * zoom;

    for (const auto &kv : snapshot->tileMap)
    {
        for (const auto &tile : kv.second)
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

            if (camera.IsOverlay(PlayerCam::Overlay::WALL) && tile->HasComponent(ComponentType::SOLID))
                DrawRectangleV(startPos, tileSize, Color(255, 0, 0, 64));
        }
    }
}

/**
 * Draws the indirect visual overlays.
 */
void DrawStationOverlays()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;

    float zoom = GameManager::GetCamera().GetZoom();
    const Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * zoom;
    bool isPowerOverlay = GameManager::GetCamera().IsOverlay(PlayerCam::Overlay::POWER);
    Texture2D stationTileset = AssetManager::GetTexture("STATION");
    Texture2D iconTileset = AssetManager::GetTexture("ICON");
    for (const auto &kv : snapshot->tileMap)
    {
        for (const auto &tile : kv.second)
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
                    DrawCircleV(GameManager::WorldToScreen(ToVector2(tile->GetPosition())), 3.0f * zoom, gridColor);
                }
            }
            if (tile->HasComponent(ComponentType::POWER_CONSUMER) && !tile->IsActive())
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
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;

    // Collect current effect IDs for cleanup detection
    std::unordered_set<uint64_t> currentIds;
    for (const auto &effect : snapshot->effects)
        currentIds.insert(effect->GetInstanceId());

    sol::state &lua = GameManager::GetLua();
    const float dt = GetFrameTime();
    const bool paused = GameManager::IsGamePaused();

    // Ensure render systems exist for each active effect (create on first encounter)
    for (const auto &effect : snapshot->effects)
    {
        uint64_t id = effect->GetInstanceId();
        auto &vec = g_renderSystems[id];

        if (vec.empty())
        {
            for (const auto &psDef : effect->GetEffectDefinition()->GetParticleSystems())
            {
                RenderParticleSystem r;
                r.system = std::make_shared<ParticleSystem>();
                r.psysId = psDef.id;
                r.effectRef = effect; // hold reference so render callbacks remain valid

                try
                {
                    if (!psDef.onCreateLua.empty())
                    {
                        std::string wrapped = std::string("return function(system, effect)\n") + psDef.onCreateLua + "\nend";
                        auto chunk = lua.load(wrapped);
                        if (chunk.valid())
                        {
                            r.onCreateFunc = chunk();
                            r.onCreateFunc(LuaParticleSystem(r.system), LuaEffect(r.effectRef));
                        }
                    }

                    if (!psDef.onUpdateLua.empty())
                    {
                        std::string wrapped2 = std::string("return function(system, effect, dt)\n") + psDef.onUpdateLua + "\nend";
                        auto chunk2 = lua.load(wrapped2);
                        if (chunk2.valid())
                            r.onUpdateFunc = chunk2();
                    }

                    if (!psDef.onDeleteLua.empty())
                    {
                        std::string wrapped3 = std::string("return function(system, effect)\n") + psDef.onDeleteLua + "\nend";
                        auto chunk3 = lua.load(wrapped3);
                        if (chunk3.valid())
                            r.onDeleteFunc = chunk3();
                    }
                }
                catch (const sol::error &e)
                {
                    throw std::runtime_error(std::string("Error compiling render Lua for effect '") + effect->GetEffectDefinition()->GetId() +
                                             "', particle system '" + psDef.id + "': " + e.what());
                }

                vec.push_back(std::move(r));
            }
        }

        if (!paused)
        {
            for (auto &r : vec)
            {
                if (r.onUpdateFunc)
                {
                    try
                    {
                        r.onUpdateFunc(LuaParticleSystem(r.system), LuaEffect(r.effectRef), dt);
                    }
                    catch (const sol::error &e)
                    {
                        throw std::runtime_error(std::string("Error in render Lua for effect '") + effect->GetEffectDefinition()->GetId() +
                                                 "', particle system '" + r.psysId + "' on_update: " + e.what());
                    }
                }

                r.system->Update(dt);
            }
        }
    }

    // Handle systems for effects that no longer exist: call on_delete, stop spawning, and erase when fully empty
    std::vector<uint64_t> toErase;
    for (auto &kv : g_renderSystems)
    {
        uint64_t id = kv.first;
        if (currentIds.count(id) == 0)
        {
            bool allEmpty = true;

            // Mark inactive and call delete handlers once
            for (auto &r : kv.second)
            {
                if (!r.deleteCalled && r.onDeleteFunc)
                {
                    try
                    {
                        r.onDeleteFunc(LuaParticleSystem(r.system), LuaEffect(r.effectRef));
                    }
                    catch (const sol::error &e)
                    {
                        throw std::runtime_error(std::string("Error in render on_delete Lua for psys '") + r.psysId + "': " + e.what());
                    }
                    r.deleteCalled = true;
                }
            }

            // Advance particle systems until they are empty (unless paused, then just check emptiness)
            if (!paused)
            {
                for (auto &r : kv.second)
                {
                    r.system->Update(dt);
                    if (!r.system->IsEmpty())
                        allEmpty = false;
                }
            }
            else
            {
                for (auto &r : kv.second)
                {
                    if (!r.system->IsEmpty())
                    {
                        allEmpty = false;
                        break;
                    }
                }
            }

            if (allEmpty)
            {
                for (auto &r : kv.second)
                    r.effectRef.reset();
                toErase.push_back(id);
            }
        }
    }

    for (uint64_t id : toErase)
        g_renderSystems.erase(id);

    // Draw all active/remaining particle systems
    for (auto &kv : g_renderSystems)
    {
        for (auto &r : kv.second)
            r.system->Draw();
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
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;
    for (const auto &crew : snapshot->crewList)
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

        const auto &selectedCrewList = snapshot->selectedCrewList;
        bool isSelected = std::find_if(selectedCrewList.begin(), selectedCrewList.end(), [crew](std::weak_ptr<Crew> _crew)
                                       { return !_crew.expired() && _crew.lock() == crew; }) != selectedCrewList.end();
        DrawCrewCircle(crew, drawPosition, isSelected);
    }
}

void DrawCrewActionProgress()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;

    for (const auto &crew : snapshot->crewList)
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

        case Action::Type::CONSTRUCTION:
        {
            const auto constructionAction = std::dynamic_pointer_cast<ConstructionAction>(action);
            const std::shared_ptr<PlannedTask> planned = constructionAction->GetPlanned().lock();
            if (!planned)
                break;

            const Vector2 barPos = GameManager::WorldToScreen(ToVector2(planned->position) - Vector2(.5 - .05, .5 - .85));
            float progress = std::clamp(planned->progress, 0.f, 1.f);
            progress = planned->isBuild ? progress : 1.f - progress;

            const Vector2 fillSize = Vector2(progress * .9f, .1f) * TILE_SIZE * GameManager::GetCamera().GetZoom();
            DrawRectangleV(barPos, fillSize, Fade(YELLOW, .8));
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
    Font font = AssetManager::GetFont("DEFAULT");
    std::string fpsText = std::format("FPS: {:} ({:.2f}ms)", GetFPS(), deltaTime * 1000.);
    const char *text = fpsText.c_str();
    DrawTextEx(font, text, Vector2(GetScreenSize().x - MeasureTextEx(font, text, DEFAULT_FONT_SIZE, 1).x - DEFAULT_PADDING, DEFAULT_PADDING), DEFAULT_FONT_SIZE, 1, UI_TEXT_COLOR);
}

void DrawResourceUI()
{
    auto station = GameManager::GetStation();
    if (!station)
        return;

    Font font = AssetManager::GetFont("DEFAULT");
    float yOffset = DEFAULT_PADDING;

    // Get all resource definitions to show all resources, even if count is 0
    const auto &resourceDefs = DefinitionManager::GetResourceDefinitions();

    for (const auto &[resourceId, resourceDef] : resourceDefs)
    {
        int count = station->GetResourceCount(resourceId);
        std::string resourceText = std::format("{}: {}", MacroCaseToName(resourceId), count);
        const char *text = resourceText.c_str();
        DrawTextEx(font, text, Vector2(DEFAULT_PADDING, yOffset), DEFAULT_FONT_SIZE, 1, UI_TEXT_COLOR);
        yOffset += DEFAULT_FONT_SIZE + DEFAULT_PADDING / 2;
    }
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
    Font font = AssetManager::GetFont("DEFAULT");

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

    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;

    if (!GameManager::IsInBuildMode())
    {
        Vector2 worldMousePos = GameManager::GetWorldMousePos() - Vector2(.5, .5);
        auto hoveredCrew = snapshot->GetCrewAtPosition(worldMousePos);
        for (const auto &crew : hoveredCrew)
        {
            if (!hoverText.empty())
                hoverText += "\n";
            hoverText += crew->GetInfo();
        }
    }

    Vector2Int tileHoverPos = GameManager::ScreenToTile(mousePos);

    auto hoveredTiles = snapshot->GetTilesAtPosition(tileHoverPos);
    for (const auto &tile : hoveredTiles)
    {
        if (!hoverText.empty())
            hoverText += "\n";
        hoverText += tile->GetInfo();
    }
    if (!GameManager::IsInBuildMode())
    {
        auto hoveredEffects = snapshot->GetEffectsAtPosition(tileHoverPos);
        for (const auto &effect : hoveredEffects)
        {
            if (!hoverText.empty())
                hoverText += "\n";
            hoverText += effect->GetInfo();
        }
    }

    if (hoverText.empty())
        return;

    DrawTooltip(hoverText, mousePos);
}

void DrawBuildUi()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot || UiManager::IsMouseOverUiElement())
        return;

    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());
    const std::string &buildTileId = GameManager::GetBuildTileId();
    if (buildTileId.empty())
        return;

    auto tileDefs = DefinitionManager::GetTileDefinitions();
    auto it = tileDefs.find(buildTileId);
    if (it == tileDefs.end())
        return;
    auto tileDef = it->second;
    if (!tileDef || !tileDef->GetReferenceSprite())
        return;
    const auto &spriteDef = tileDef->GetReferenceSprite();

    auto drawGhost = [&](const Vector2Int &pos)
    {
        if (auto basicDef = std::dynamic_pointer_cast<BasicSpriteDef>(spriteDef))
        {
            BasicSprite ghostSprite(basicDef->spriteOffset);
            ghostSprite.Draw(pos, Fade(WHITE, 0.5f), 0);
        }
        else if (auto multiDef = std::dynamic_pointer_cast<MultiSliceSpriteDef>(spriteDef))
        {
            std::vector<SpriteSlice> slices;
            for (const auto &swc : multiDef->slices)
                slices.push_back(swc.slice);
            MultiSliceSprite ghostSprite(slices);
            ghostSprite.Draw(pos, Fade(WHITE, 0.5f), 0);
        }
    };

    // Collect all unique positions to draw (main + symmetry)
    std::unordered_set<Vector2Int> ghostPositions;
    ghostPositions.insert(cursorPos);
    if (GameManager::IsHorizontalSymmetry())
        ghostPositions.insert(Vector2Int(cursorPos.x, -cursorPos.y - 1));
    if (GameManager::IsVerticalSymmetry())
        ghostPositions.insert(Vector2Int(-cursorPos.x - 1, cursorPos.y));
    if (GameManager::IsHorizontalSymmetry() && GameManager::IsVerticalSymmetry())
        ghostPositions.insert(Vector2Int(-cursorPos.x - 1, -cursorPos.y - 1));

    for (const auto &pos : ghostPositions)
        drawGhost(pos);
}

void DrawPlannedTasks()
{
    auto station = GameManager::GetStation();
    if (!station)
        return;

    Texture2D iconTileset = AssetManager::GetTexture("ICON");
    Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * GameManager::GetCamera().GetZoom();

    for (const auto &task : station->plannedTasks)
    {
        if (task->isBuild)
        {
            auto tileDefs = DefinitionManager::GetTileDefinitions();
            auto it = tileDefs.find(task->tileId);
            if (it == tileDefs.end())
                continue;
            auto tileDef = it->second;
            if (!tileDef || !tileDef->GetReferenceSprite())
                continue;
            const auto &spriteDef = tileDef->GetReferenceSprite();

            auto drawGhost = [&](const Vector2Int &pos)
            {
                if (auto basicDef = std::dynamic_pointer_cast<BasicSpriteDef>(spriteDef))
                {
                    BasicSprite ghostSprite(basicDef->spriteOffset);
                    ghostSprite.Draw(pos, Fade(WHITE, 0.5f), 0);
                }
                else if (auto multiDef = std::dynamic_pointer_cast<MultiSliceSpriteDef>(spriteDef))
                {
                    std::vector<SpriteSlice> slices;
                    for (const auto &swc : multiDef->slices)
                        slices.push_back(swc.slice);
                    MultiSliceSprite ghostSprite(slices);
                    ghostSprite.Draw(pos, Fade(WHITE, 0.5f), 0);
                }
            };

            drawGhost(task->position);
        }

        Rectangle sourceRect = (task->isBuild ? Rectangle(1, 1, 1, 1) : Rectangle(3, 1, 1, 1)) * TILE_SIZE;
        Rectangle destRect = Vector2ToRect(GameManager::WorldToScreen(task->position) + tileSize / 4., tileSize / 2.);
        DrawTexturePro(iconTileset, sourceRect, destRect, tileSize / 2., 0, Fade(WHITE, .4));
    }
}

void ClearRenderSystems()
{
    for (auto &kv : g_renderSystems)
    {
        for (auto &r : kv.second)
        {
            r.system->Clear();
        }
    }
    g_renderSystems.clear();
}

void CreateStarfield(uint64_t seed)
{
    g_starfieldParticles.clear();
    const int starCount = 500;
    const Vector2 screenSize = GetScreenSize();

    std::mt19937 gen(seed);
    std::uniform_int_distribution<uint16_t> dist_x(0, screenSize.x);
    std::uniform_int_distribution<uint16_t> dist_y(0, screenSize.y);
    std::uniform_int_distribution<u_char> dist_z(0, 100);
    std::uniform_int_distribution<u_char> dist_size(1, 3);
    std::uniform_int_distribution<u_char> dist_brightness(100, 225);
    std::uniform_int_distribution<u_char> dist_color_offset(0, 30);
    std::uniform_int_distribution<u_char> dist_alpha(150, 255);

    for (int i = 0; i < starCount; ++i)
    {
        StarfieldParticle star;
        star.x = dist_x(gen);
        star.y = dist_y(gen);
        star.z = dist_z(gen);
        star.size = dist_size(gen);
        u_char brightness = dist_brightness(gen);
        u_char r = brightness + dist_color_offset(gen);
        u_char g = brightness + dist_color_offset(gen);
        u_char b = brightness + dist_color_offset(gen);
        star.color = Color(r, g, b, dist_alpha(gen));
        g_starfieldParticles.push_back(star);
    }
}

void DrawStarfieldBackground()
{
    const Vector2 screenSize = GetScreenSize();
    const Vector2 oldSceenSize = GameManager::GetOriginalScreenSize();
    const Vector2 scale = Vector2(screenSize.x / oldSceenSize.x, screenSize.y / oldSceenSize.y);
    const Vector2 camPixels = GameManager::GetCamera().GetPosition() * TILE_SIZE;
    auto wrap = [](float v, float dim)
    { float r = std::fmod(v, dim); return r >= 0.f ? r : r + dim; };

    for (const auto &star : g_starfieldParticles)
    {
        // depth: 0..100 -> closer stars have larger effect
        float parallax = 0.01f * (1.f + (star.z / 100.f) * 2.f);
        DrawRectangle(
            wrap(star.x * scale.x - wrap(camPixels.x * parallax, screenSize.x), screenSize.x),
            wrap(star.y * scale.y - wrap(camPixels.y * parallax, screenSize.y), screenSize.y),
            star.size, star.size, star.color);
    }
}

void ClearStarfield()
{
    g_starfieldParticles.clear();
}
