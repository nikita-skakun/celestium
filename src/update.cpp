#include "action.hpp"
#include "camera.hpp"
#include "component.hpp"
#include "env_effect.hpp"
#include "game_server.hpp"
#include "game_state.hpp"
#include "pawn.hpp"
#include "planned_task.hpp"
#include "power_grid.hpp"
#include "render_snapshot.hpp"
#include "station.hpp"
#include "tile.hpp"
#include "update.hpp"

void HandlePlaceTile(const std::shared_ptr<const Station> &station)
{
    const std::string &tileIdToPlace = GameManager::GetBuildTileId();
    const auto &tileDefinition = DefinitionManager::GetTileDefinition(tileIdToPlace);
    if (!tileDefinition)
        return;

    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());
    std::vector<Vector2Int> posListToPlace = GameManager::GetSymmetryPositions(cursorPos);

    for (const auto &pos : posListToPlace)
    {
        // Don't plan if the same tile already exists at this position and height
        if (auto t = station->GetTileAtPosition(pos, tileDefinition->GetHeight()))
            if (t->GetId() == tileIdToPlace)
                continue;

        GameManager::GetServer().RequestPlannedTask(pos, tileIdToPlace, true);
        TraceLog(TraceLogLevel::LOG_INFO, std::format("Planned to place {} at {}", tileDefinition->GetName(), ToString(pos)).c_str());
    }
}

void HandleDeleteTile(const std::shared_ptr<const Station> &station)
{
    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());
    std::vector<Vector2Int> posListToDelete = GameManager::GetSymmetryPositions(cursorPos);

    for (const auto &pos : posListToDelete)
    {
        const auto &tilesAtPos = station->GetTilesAtPosition(pos);
        if (!tilesAtPos.empty())
        {
            const auto &topTile = tilesAtPos.back();
            GameManager::GetServer().RequestPlannedTask(pos, topTile->GetId(), false);
            TraceLog(TraceLogLevel::LOG_INFO, std::format("Planned to remove {} at {}", topTile->GetId(), ToString(pos)).c_str());
        }
    }
}

void HandleBuildMode()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;
    auto station = snapshot->station;
    if (!station)
        return;
    // If we're in cancel mode, left-click cancels planned tasks (respecting symmetry)
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && GameManager::IsInCancelMode())
    {
        Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());
        std::vector<Vector2Int> posListToCancel = GameManager::GetSymmetryPositions(cursorPos);

        for (const auto &pos : posListToCancel)
        {
            if (station->HasPlannedTaskAt(pos))
            {
                GameManager::GetServer().RequestCancelPlannedTask(pos);
                TraceLog(TraceLogLevel::LOG_INFO, std::format("Canceled planned task at {}", ToString(pos)).c_str());
            }
        }

        return; // Don't also place/delete while canceling
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !GameManager::GetBuildTileId().empty())
        HandlePlaceTile(station);

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        HandleDeleteTile(station);
}

void HandlePawnHover()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;
    const auto &pawnList = snapshot->pawnList;
    const Vector2 worldMousePos = GameManager::GetWorldMousePos() - Vector2(.5, .5);
    const float pawnSize = PAWN_RADIUS / TILE_SIZE;
    const float pawnSizeSq = pawnSize * pawnSize;

    GameManager::ClearHoveredPawn();

    for (const auto &entry : pawnList)
    {
        const auto &pawn = entry.second;
        if (!pawn || Vector2DistanceSq(worldMousePos, pawn->GetPosition()) > pawnSizeSq)
            continue;

        GameManager::AddHoveredPawn(pawn->GetInstanceId());
    }
}

void HandleMouseDragStart()
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        GameManager::GetCamera().SetDragStart(GameManager::GetWorldMousePos());
}

void HandleMouseDragDuring()
{
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        return;

    auto &camera = GameManager::GetCamera();

    if (!camera.IsDragging() && Vector2DistanceSq(camera.GetDragStart(), GameManager::GetWorldMousePos()) > DRAG_THRESHOLD * DRAG_THRESHOLD)
        camera.SetDragType(PlayerCam::DragType::SELECT);

    if (camera.IsDragging())
        camera.SetDragEnd(GameManager::GetWorldMousePos());
}

void HandleMouseDragEnd()
{
    auto &camera = GameManager::GetCamera();
    if (!IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || !camera.IsDragging())
        return;

    camera.SetDragType(PlayerCam::DragType::NONE);
}

void HandleMouseDrag()
{
    HandleMouseDragStart();
    HandleMouseDragDuring();
    HandleMouseDragEnd();
}

void HandlePawnSelection()
{
    if (!IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        return;

    auto &camera = GameManager::GetCamera();
    if (camera.IsDragging() && camera.GetDragType() != PlayerCam::DragType::SELECT)
        return;

    if (!IsKeyDown(KEY_LEFT_SHIFT))
        GameManager::ClearSelectedPawn();
    if (camera.GetDragType() == PlayerCam::DragType::SELECT)
    {
        auto snapshot = GameManager::GetRenderSnapshot();
        if (!snapshot)
            return;
        auto &pawnList = snapshot->pawnList;
        for (const auto &entry : pawnList)
        {
            const auto &pawn = entry.second;
            bool isWithinRect = IsVector2WithinRect(camera.GetDragRect(), pawn->GetPosition() + Vector2(.5, .5));
            if (!pawn || !isWithinRect)
                continue;

            GameManager::AddSelectedPawn(pawn->GetInstanceId());
        }

        return;
    }

    const auto &hoveredPawn = GameManager::GetHoveredPawn();
    if (hoveredPawn.empty())
        return;

    GameManager::ToggleSelectedPawn(hoveredPawn.at(0));
}

void AssignPawnActions()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;

    const auto &selectedPawn = GameManager::GetSelectedPawn();
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !selectedPawn.empty())
    {
        Vector2Int worldPos = ToVector2Int(GameManager::GetWorldMousePos());

        for (const auto pawnId : selectedPawn)
        {
            std::shared_ptr<const Pawn> snapshotPawn = nullptr;
            auto itPawn = snapshot->pawnList.find(pawnId);
            if (itPawn != snapshot->pawnList.end())
                snapshotPawn = itPawn->second;

            if (!snapshotPawn || !snapshotPawn->IsAlive() || !snapshotPawn->GetCurrentTile())
                continue;

            if (!IsKeyDown(KEY_LEFT_SHIFT))
                GameManager::GetServer().ClearPawnActions(snapshotPawn->GetInstanceId());

            GameManager::GetServer().SendPlayerAction(snapshotPawn->GetInstanceId(), std::make_unique<MoveAction>(ToVector2(worldPos)));
        }
    }
}

void HandlePawnActions()
{
    const auto &pawnList = GameManager::GetServer().GetPawnList();
    for (const auto &entry : pawnList)
    {
        const auto &pawn = entry.second;
        if (!pawn->IsAlive() || pawn->GetActionQueue().empty())
            continue;

        const auto &currentAction = pawn->GetActionQueue().front();
        if (currentAction->Update(pawn))
            pawn->RemoveFirstAction();
    }
}

void HandlePawnEnvironment()
{
    const auto &pawnList = GameManager::GetServer().GetPawnList();
    for (const auto &entry : pawnList)
    {
        const auto &pawn = entry.second;
        if (!pawn->IsAlive())
            continue;

        pawn->ConsumeOxygen(FIXED_DELTA_TIME);
        auto tile = pawn->GetCurrentTile();
        if (!tile)
            continue;

        if (auto oxygen = tile->GetComponent<OxygenComponent>())
            pawn->RefillOxygen(FIXED_DELTA_TIME, oxygen->GetOxygenLevel());

        auto effects = tile->GetStation()->GetEffectsAtPosition(tile->GetPosition());
        for (const auto &effect : effects)
            effect->EffectPawn(pawn, FIXED_DELTA_TIME);
    }
}

void UpdatePawnCurrentTile()
{
    auto station = GameManager::GetServer().GetStation();
    if (!station || station->tileMap.empty())
        return;

    const auto &pawnList = GameManager::GetServer().GetPawnList();
    for (const auto &entry : pawnList)
    {
        const auto &pawn = entry.second;
        if (!pawn->IsAlive())
            continue;

        Vector2Int floorPawnPos = ToVector2Int(pawn->GetPosition());

        if (pawn->GetCurrentTile() && pawn->GetCurrentTile()->GetPosition() == floorPawnPos)
            continue;

        pawn->SetCurrentTile(station->GetTileAtPosition(floorPawnPos, TileHeight::FLOOR));
    }
}

void UpdatePowerGrids()
{
    auto station = GameManager::GetServer().GetStation();
    if (!station)
        return;

    for (const auto &powerGrid : station->powerGrids)
    {
        powerGrid->Update(FIXED_DELTA_TIME);
    }
}

void UpdateTiles()
{
    auto station = GameManager::GetServer().GetStation();
    if (!station)
        return;

    for (const auto &tilesAtPos : station->tileMap)
    {
        for (const auto &tile : tilesAtPos.second)
        {
            if (auto door = tile->GetComponent<DoorComponent>())
            {
                door->Animate(FIXED_DELTA_TIME);
            }

            if (auto oxygenProducer = tile->GetComponent<OxygenProducerComponent>())
                oxygenProducer->ProduceOxygen(FIXED_DELTA_TIME);

            if (auto oxygen = tile->GetComponent<OxygenComponent>())
                oxygen->Diffuse(FIXED_DELTA_TIME);
        }
    }
}

void UpdateEnvironmentalEffects()
{
    auto station = GameManager::GetServer().GetStation();
    if (!station)
        return;

    for (int i = station->effects.size() - 1; i >= 0; --i)
    {
        auto &effect = station->effects.at(i);
        effect->Update(station, i);
    }
}
