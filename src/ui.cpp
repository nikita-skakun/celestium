#include "action.hpp"
#include "asset_manager.hpp"
#include "camera.hpp"
#include "component.hpp"
#include "def_manager.hpp"
#include "env_effect.hpp"
#include "game_server.hpp"
#include "game_state.hpp"
#include "lua_bindings.hpp"
#include "particle_system.hpp"
#include "pawn_def.hpp"
#include "pawn.hpp"
#include "planned_task.hpp"
#include "power_grid.hpp"
#include "render_snapshot.hpp"
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
    uint16_t x, y;
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

void DrawDoorPanels(const Vector2Int &pos, float progress, float rotation, const Color &tint)
{
    float zoom = GameManager::GetCamera().GetZoom();
    const Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * zoom;
    Texture2D stationTileset = AssetManager::GetTexture("STATION");

    Vector2 startPos = GameManager::WorldToScreen(pos);
    Rectangle destRect = Vector2ToRect(startPos, tileSize);
    destRect.height = std::max(25. * progress, 1.) * zoom;
    Rectangle doorSourceRect = Rectangle(0, 7, 1, 1) * TILE_SIZE;
    doorSourceRect.height = std::max(25. * progress, 1.);
    Vector2 pivot = Vector2(tileSize.x / 2., destRect.height - 25. * zoom);
    DrawTexturePro(stationTileset, doorSourceRect, destRect, pivot, rotation, tint);
    Rectangle doorSourceRect2 = doorSourceRect;
    doorSourceRect2.width = -doorSourceRect2.width;
    DrawTexturePro(stationTileset, doorSourceRect2, destRect, pivot, rotation + 180., tint);
}

void DrawSpriteDefGhost(const std::shared_ptr<SpriteDef> &spriteDef, const Vector2Int &pos, const Color &tint, float rotation, const std::shared_ptr<const Station> &station, const std::string &tileId, TileHeight height)
{
    if (auto basicDef = std::dynamic_pointer_cast<BasicSpriteDef>(spriteDef))
        BasicSprite(basicDef->spriteOffset).Draw(pos, tint, rotation);
    else if (auto multiDef = std::dynamic_pointer_cast<MultiSliceSpriteDef>(spriteDef))
    {
        auto status = station ? station->GetSpriteConditionForPosition(pos, tileId, height) : SpriteCondition::NONE;
        std::vector<SpriteSlice> slices;
        for (const auto &swc : multiDef->slices)
            if ((status & swc.conditions) == swc.conditions)
                slices.push_back(swc.slice);
        MultiSliceSprite(slices).Draw(pos, tint, rotation);
    }
}

void DrawTileDefGhost(const std::shared_ptr<TileDef> &tileDef, const Vector2Int &pos, const Color &tint, float rotation, const std::shared_ptr<const Station> &station)
{
    if (!tileDef)
        return;
    Rotation rotEnum = AngleToRotation(rotation);
    const std::string &tileId = tileDef->GetId();
    TileHeight height = tileDef->GetHeight();

    DrawSpriteDefGhost(tileDef->GetReferenceSprite(), pos, tint, rotation, station, tileId, height);
    for (const auto &cell : tileDef->GetExtraParts())
        DrawSpriteDefGhost(cell.spriteDef, pos + OffsetWithRotation(rotEnum, cell.offset), tint, rotation, station, tileId, height);
    if (tileDef->HasComponent(ComponentType::DOOR))
        DrawDoorPanels(pos, 1.f, rotation, tint);
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
void DrawPath(const std::deque<Vector2> &path, const Vector2 &startPos)
{
    if (path.empty())
        return;

    Vector2 prevPos = startPos;
    for (const auto &point : path)
    {
        DrawLineEx(GameManager::WorldToScreen(prevPos), GameManager::WorldToScreen(point), 3, Fade(GREEN, .5));
        prevPos = point;
    }
}

static std::vector<std::shared_ptr<Tile>> GetUniqueTiles(const std::shared_ptr<const Station> &station)
{
    std::vector<std::shared_ptr<Tile>> tiles;
    if (!station)
        return tiles;
    std::unordered_set<std::shared_ptr<Tile>> uniqueTiles;
    for (const auto &kv : station->tileMap)
        for (const auto &tile : kv.second)
            if (uniqueTiles.insert(tile).second)
                tiles.push_back(tile);
    return tiles;
}

/**
 * Draws the station tiles and direct overlays.
 */
void DrawStationTiles()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot || !snapshot->station)
        return;

    auto &camera = GameManager::GetCamera();
    const float zoom = camera.GetZoom();
    const Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * zoom;

    auto tiles = GetUniqueTiles(snapshot->station);

    // Pass 1: Draw main/reference sprites for all tiles
    for (const auto &tile : tiles)
    {
        float rotation = 0;
        if (auto rotatable = tile->GetComponent<RotatableComponent>())
            rotation = RotationToAngle(rotatable->GetRotation());

        Color tint = GetTileTint(tile);
        if (!tile->GetSprites().empty() && tile->GetSprites().front())
            tile->GetSprites().front()->Draw(tile->GetPosition(), tint, rotation);
    }

    // Pass 2: Draw extra parts / offset sprites for all tiles
    for (const auto &tile : tiles)
    {
        float rotation = 0;
        if (auto rotatable = tile->GetComponent<RotatableComponent>())
            rotation = RotationToAngle(rotatable->GetRotation());

        Color tint = GetTileTint(tile);
        const auto &sprites = tile->GetSprites();
        for (size_t i = 1; i < sprites.size(); ++i)
        {
            if (sprites[i])
                sprites[i]->Draw(tile->GetPosition(), tint, rotation);
        }
    }

    // Pass 3: Draw oxygen and wall debug overlays
    for (const auto &tile : tiles)
    {
        Vector2 startPos = GameManager::WorldToScreen(ToVector2(tile->GetPosition()) - Vector2(.5f, .5f));

        if (camera.IsOverlay(PlayerCam::Overlay::OXYGEN))
        {
            if (auto oxygen = tile->GetComponent<OxygenComponent>())
            {
                Color color = Color(50, 150, 255, oxygen->GetOxygenLevel() / TILE_OXYGEN_MAX * 255 * .8f);
                DrawRectangleV(startPos, tileSize, color);
            }
        }

        if (camera.IsOverlay(PlayerCam::Overlay::WALL) && tile->HasComponent(ComponentType::SOLID))
            DrawRectangleV(startPos, tileSize, Color(255, 0, 0, 64));
    }
}

void DrawStationOverlays()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot || !snapshot->station)
        return;

    float zoom = GameManager::GetCamera().GetZoom();
    const Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * zoom;
    bool isPowerOverlay = GameManager::GetCamera().IsOverlay(PlayerCam::Overlay::POWER);
    Texture2D iconTileset = AssetManager::GetTexture("ICON");

    auto tiles = GetUniqueTiles(snapshot->station);

    for (const auto &tile : tiles)
    {
        Color tint = GetTileTint(tile);
        float rotation = 0;
        if (auto rotatable = tile->GetComponent<RotatableComponent>())
            rotation = RotationToAngle(rotatable->GetRotation());

        if (auto door = tile->GetComponent<DoorComponent>())
            DrawDoorPanels(tile->GetPosition(), door->GetProgress(), rotation, tint);

        if (isPowerOverlay && magic_enum::enum_flags_test_any(tile->GetHeight(), TileHeight::POWER))
        {
            if (auto connector = tile->GetComponent<PowerConnectorComponent>())
            {
                auto grid = connector->GetPowerGrid();
                Color gridColor = grid ? grid->GetDebugColor() : Color(200, 200, 200, 128);
                DrawCircleV(GameManager::WorldToScreen(ToVector2(tile->GetPosition())), 3.f * zoom, gridColor);
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

    // Draw NavMesh debug outlines
    for (int pIdx = 0; pIdx < (int)snapshot->station->navPolygons.size(); ++pIdx)
    {
        const auto &poly = snapshot->station->navPolygons[pIdx];

        std::vector<Vector2> screenPoints;
        for (int i = 0; i < 4; ++i)
            screenPoints.push_back(GameManager::WorldToScreen(poly.vertices[i]));

        // Draw outline
        for (size_t i = 0; i < screenPoints.size(); ++i)
            DrawLineEx(screenPoints[i], screenPoints[(i + 1) % screenPoints.size()], 2.0f, Fade(ORANGE, 0.5f));

        // Optional: Draw links between polygon centers
        Vector2 center = GameManager::WorldToScreen(poly.GetCenter());
        for (const auto &link : poly.links)
            DrawLineEx(center, GameManager::WorldToScreen(snapshot->station->navPolygons[link.targetPolyIdx].GetCenter()), 1.0f, Fade(YELLOW, 0.3f));
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

/**
 * Draws the station's environmental effects (such as fire or foam).
 */
void DrawEnvironmentalEffects()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot || !snapshot->station)
        return;

    // Collect current effect IDs for cleanup detection
    std::unordered_set<uint64_t> currentIds;
    for (const auto &effect : snapshot->station->effects)
        currentIds.insert(effect->GetInstanceId());

    sol::state &lua = GameManager::GetLua();
    const float dt = GetFrameTime();
    const bool paused = GameManager::GetServer().IsGamePaused();

    // Ensure render systems exist for each active effect (create on first encounter)
    for (const auto &effect : snapshot->station->effects)
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

void DrawPawnSprite(const std::shared_ptr<const Pawn> &pawn, const Vector2 &drawPosition, bool isSelected, bool isMoving)
{
    auto &camera = GameManager::GetCamera();
    Vector2 pawnScreenPos = GameManager::WorldToScreen(drawPosition);
    float pawnRadius = (PAWN_DRAW_SIZE * 0.5f) * camera.GetZoom();

    auto DrawPawnOutline = [&](float radius, Color color)
    {
        float innerRadius = radius - OUTLINE_SIZE * camera.GetZoom();
        if (innerRadius < 0.f)
            innerRadius = 0.f;
        DrawRing(pawnScreenPos, innerRadius, radius, 0.f, 360.f, 24, color);
    };

    float outlineOpacity = isSelected ? .75f : .5f;
    bool isDead = !pawn->IsAlive();

    if (isDead)
        DrawPawnOutline(pawnRadius * 1.25f, Fade(GRAY, outlineOpacity));
    else
        DrawPawnOutline(pawnRadius * 1.25f, Fade(pawn->GetColor(), outlineOpacity));

    // Try to draw sprite, but fall back to circle on any error
    try
    {
        // Determine animation type
        PawnAnimationType animType = PawnAnimationType::IDLE;
        if (!isDead && isMoving)
            animType = PawnAnimationType::WALKING;

        // Get pawn definition
        auto pawnDef = DefinitionManager::GetPawnDefinition("BASE");
        if (!pawnDef)
            return;

        // Get animation and frame
        float animSpeed = pawnDef->GetAnimationSpeed(animType);

        // Update animation elapsed time
        pawn->AddAnimationElapsedTime(GetFrameTime());
        size_t frameIndex = static_cast<size_t>(pawn->GetAnimationElapsedTime() / animSpeed);

        Vector2Int frameOffset = pawnDef->GetFrame(animType, pawn->GetFacingDirection(), frameIndex);

        // Draw sprite from spritesheet
        Vector2 tileSize = Vector2(PAWN_DRAW_SIZE, PAWN_DRAW_SIZE) * camera.GetZoom();
        Rectangle sourceRect = Rectangle(frameOffset.x * PAWN_SPRITE_SIZE, frameOffset.y * PAWN_SPRITE_SIZE, PAWN_SPRITE_SIZE, PAWN_SPRITE_SIZE);
        Rectangle destRect = Vector2ToRect(pawnScreenPos, tileSize);
        Color spriteTint = isDead ? Fade(GRAY, .5f) : WHITE;

        DrawTexturePro(AssetManager::GetTexture("PAWN"), sourceRect, destRect, tileSize / 2.f, 0, spriteTint);
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_WARNING, "Error drawing pawn sprite: %s", e.what());
    }
}

/**
 * Draws the pawn members on the screen, accounting for their movement.
 */
void DrawPawn()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;
    for (const auto &kv : snapshot->pawnList)
    {
        const auto &pawn = kv.second;
        Vector2 drawPosition = pawn->GetPosition();
        auto &actionQueue = pawn->GetReadOnlyActionQueue();
        bool isMoving = false;

        if (!actionQueue.empty() && actionQueue.front()->GetType() == Action::Type::MOVE)
        {
            const auto moveAction = std::dynamic_pointer_cast<MoveAction>(actionQueue.front());
            isMoving = moveAction && moveAction->IsMoving();

            if (isMoving && !GameManager::IsInBuildMode() && !moveAction->path.empty())
            {
                DrawPath(moveAction->path, pawn->GetPosition());
                Vector2 nextPosition = moveAction->path.front();

                const float moveDelta = static_cast<float>(snapshot->timeSinceFixedUpdate * PAWN_MOVE_SPEED);
                const float distToNext = Vector2Distance(pawn->GetPosition(), nextPosition);

                if (distToNext <= moveDelta)
                {
                    drawPosition = nextPosition;

                    if (moveAction->path.size() > 1)
                    {
                        Vector2 futurePosition = moveAction->path.at(1);
                        drawPosition += Vector2Normalize(futurePosition - drawPosition) * (moveDelta - distToNext);
                    }
                }
                else
                {
                    bool canPath = true;
                    if (std::shared_ptr<Station> station = pawn->GetCurrentTile()->GetStation())
                        canPath = station->IsDoorFullyOpenAtPos(ToVector2Int(nextPosition / TILE_SIZE));

                    if (canPath)
                        drawPosition += Vector2Normalize(nextPosition - pawn->GetPosition()) * moveDelta;
                }
            }
        }

        bool isSelected = Find(GameManager::GetSelectedPawn(), pawn->GetInstanceId()).has_value();
        DrawPawnSprite(pawn, drawPosition, isSelected, isMoving);
    }
}

void DrawPawnActionProgress()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;

    for (const auto &kv : snapshot->pawnList)
    {
        const auto &pawn = kv.second;
        if (!pawn->IsAlive() || pawn->GetReadOnlyActionQueue().empty())
            continue;

        const auto &action = pawn->GetReadOnlyActionQueue().front();

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
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot || !snapshot->station)
        return;
    auto station = snapshot->station;
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
        textWidth = std::max(textWidth, MeasureTextEx(font, lines[i], fontSize, 1).x);

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
        DrawTextEx(font, lines[i], tooltipPos + Vector2(padding, padding + (i * fontSize)), fontSize, 1, DARKGRAY);
}

/**
 * Draws a tooltip about the pawn member or station tile under the mouse cursor.
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
        auto hoveredPawn = snapshot->GetPawnAtPosition(worldMousePos);
        for (const auto &pawn : hoveredPawn)
        {
            if (!hoverText.empty())
                hoverText += "\n";
            hoverText += pawn->GetInfo();
        }
    }

    Vector2Int tileHoverPos = GameManager::ScreenToTile(mousePos);

    if (snapshot->station)
    {
        auto hoveredTiles = snapshot->station->GetTilesAtPosition(tileHoverPos);
        for (const auto &tile : hoveredTiles)
        {
            if (!hoverText.empty())
                hoverText += "\n";
            hoverText += tile->GetInfo();
        }

        if (!GameManager::IsInBuildMode())
        {
            auto hoveredEffects = snapshot->station->GetEffectsAtPosition(tileHoverPos);
            for (const auto &effect : hoveredEffects)
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
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot || UiManager::IsMouseOverUiElement())
        return;

    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());
    const std::string &buildTileId = GameManager::GetBuildTileId();
    if (buildTileId.empty())
        return;

    auto tileDef = DefinitionManager::GetTileDefinition(buildTileId);
    if (!tileDef)
        return;

    float rotAngle = RotationToAngle(GameManager::GetBuildRotation());
    for (const auto &pos : GameManager::GetSymmetryPositions(cursorPos))
        DrawTileDefGhost(tileDef, pos, Fade(WHITE, 0.5f), rotAngle, snapshot->station);
}

void DrawPlannedTasks()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot || !snapshot->station)
        return;

    Texture2D iconTileset = AssetManager::GetTexture("ICON");
    Vector2 tileSize = Vector2(1, 1) * TILE_SIZE * GameManager::GetCamera().GetZoom();

    for (const auto &task : snapshot->station->plannedTasks)
    {
        if (task->isBuild)
        {
            auto tileDef = DefinitionManager::GetTileDefinition(task->tileId);
            if (tileDef)
                DrawTileDefGhost(tileDef, task->position, Fade(WHITE, 0.4f), RotationToAngle(task->rotation), snapshot->station);
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
