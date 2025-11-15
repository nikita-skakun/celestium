#include "action.hpp"
#include "component.hpp"
#include "crew.hpp"
#include "env_effect.hpp"
#include "game_state.hpp"
#include "planned_task.hpp"
#include "power_grid.hpp"
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

    std::vector<Vector2Int> posListToPlace;
    posListToPlace.push_back(cursorPos);
    if (GameManager::IsHorizontalSymmetry())
        posListToPlace.push_back(Vector2Int(cursorPos.x, -cursorPos.y - 1));

    if (GameManager::IsVerticalSymmetry())
        posListToPlace.push_back(Vector2Int(-cursorPos.x - 1, cursorPos.y));

    if (GameManager::IsHorizontalSymmetry() && GameManager::IsVerticalSymmetry())
        posListToPlace.push_back(Vector2Int(-cursorPos.x - 1, -cursorPos.y - 1));

    for (const auto &pos : posListToPlace)
    {
        // Don't plan if the same tile already exists at this position and height
        if (station->GetTileIdAtPosition(pos, tileDefinition->GetHeight()) == tileIdToPlace)
            continue;

        GameManager::GetServer().RequestPlannedTask(pos, tileIdToPlace, true);
        TraceLog(TraceLogLevel::LOG_INFO, std::format("Planned to place {} at {}", tileDefinition->GetName(), ToString(pos)).c_str());
    }
}

void HandleDeleteTile(const std::shared_ptr<const Station> &station)
{
    Vector2Int cursorPos = ToVector2Int(GameManager::GetWorldMousePos());

    std::vector<Vector2Int> posListToDelete;
    posListToDelete.push_back(cursorPos);
    if (GameManager::IsHorizontalSymmetry())
        posListToDelete.push_back(Vector2Int(cursorPos.x, -cursorPos.y - 1));

    if (GameManager::IsVerticalSymmetry())
        posListToDelete.push_back(Vector2Int(-cursorPos.x - 1, cursorPos.y));

    if (GameManager::IsHorizontalSymmetry() && GameManager::IsVerticalSymmetry())
        posListToDelete.push_back(Vector2Int(-cursorPos.x - 1, -cursorPos.y - 1));

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
        std::vector<Vector2Int> posListToCancel;
        posListToCancel.push_back(cursorPos);
        if (GameManager::IsHorizontalSymmetry())
            posListToCancel.push_back(Vector2Int(cursorPos.x, -cursorPos.y - 1));
        if (GameManager::IsVerticalSymmetry())
            posListToCancel.push_back(Vector2Int(-cursorPos.x - 1, cursorPos.y));
        if (GameManager::IsHorizontalSymmetry() && GameManager::IsVerticalSymmetry())
            posListToCancel.push_back(Vector2Int(-cursorPos.x - 1, -cursorPos.y - 1));

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

void HandleCrewHover()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;
    const auto &crewList = snapshot->crewList;
    const Vector2 worldMousePos = GameManager::GetWorldMousePos() - Vector2(.5, .5);
    const float crewSize = CREW_RADIUS / TILE_SIZE;
    const float crewSizeSq = crewSize * crewSize;

    GameManager::ClearHoveredCrew();

    for (const auto &entry : crewList)
    {
        const auto &crew = entry.second;
        if (!crew || Vector2DistanceSq(worldMousePos, crew->GetPosition()) > crewSizeSq)
            continue;

        GameManager::AddHoveredCrew(crew->GetInstanceId());
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

void HandleCrewSelection()
{
    if (!IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        return;

    auto &camera = GameManager::GetCamera();
    if (camera.IsDragging() && camera.GetDragType() != PlayerCam::DragType::SELECT)
        return;

    if (!IsKeyDown(KEY_LEFT_SHIFT))
        GameManager::ClearSelectedCrew();
    if (camera.GetDragType() == PlayerCam::DragType::SELECT)
    {
        auto snapshot = GameManager::GetRenderSnapshot();
        if (!snapshot)
            return;
        auto &crewList = snapshot->crewList;
        for (const auto &entry : crewList)
        {
            const auto &crew = entry.second;
            bool isWithinRect = IsVector2WithinRect(camera.GetDragRect(), crew->GetPosition() + Vector2(.5, .5));
            if (!crew || !isWithinRect)
                continue;

            GameManager::AddSelectedCrew(crew->GetInstanceId());
        }

        return;
    }

    const auto &hoveredCrew = GameManager::GetHoveredCrew();
    if (hoveredCrew.empty())
        return;

    GameManager::ToggleSelectedCrew(hoveredCrew.at(0));
}

void AssignCrewActions()
{
    auto snapshot = GameManager::GetRenderSnapshot();
    if (!snapshot)
        return;

    const auto &selectedCrew = GameManager::GetSelectedCrew();
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !selectedCrew.empty())
    {
        Vector2Int worldPos = ToVector2Int(GameManager::GetWorldMousePos());

        for (const auto crewId : selectedCrew)
        {
            std::shared_ptr<const Crew> snapshotCrew = nullptr;
            auto itCrew = snapshot->crewList.find(crewId);
            if (itCrew != snapshot->crewList.end())
                snapshotCrew = itCrew->second;

            if (!snapshotCrew || !snapshotCrew->IsAlive() || !snapshotCrew->GetCurrentTile())
                continue;

            if (!IsKeyDown(KEY_LEFT_SHIFT))
                GameManager::GetServer().ClearCrewActions(snapshotCrew->GetInstanceId());

            GameManager::GetServer().SendPlayerAction(snapshotCrew->GetInstanceId(), std::make_unique<MoveAction>(worldPos));
        }
    }
}

void HandleCrewActions()
{
    const auto &crewList = GameManager::GetServer().GetCrewList();
    for (const auto &entry : crewList)
    {
        const auto &crew = entry.second;
        if (!crew->IsAlive() || crew->GetActionQueue().empty())
            continue;

        const auto &currentAction = crew->GetActionQueue().front();
        currentAction->Update(crew);
    }
}

void HandleCrewEnvironment()
{
    const auto &crewList = GameManager::GetServer().GetCrewList();
    for (const auto &entry : crewList)
    {
        const auto &crew = entry.second;
        if (!crew->IsAlive())
            continue;

        crew->ConsumeOxygen(FIXED_DELTA_TIME);
        auto tile = crew->GetCurrentTile();
        if (!tile)
            continue;

        if (auto oxygen = tile->GetComponent<OxygenComponent>())
            crew->RefillOxygen(FIXED_DELTA_TIME, oxygen->GetOxygenLevel());

        auto effects = tile->GetStation()->GetEffectsAtPosition(tile->GetPosition());
        for (const auto &effect : effects)
            effect->EffectCrew(crew, FIXED_DELTA_TIME);
    }
}

void UpdateCrewCurrentTile()
{
    auto station = GameManager::GetServer().GetStation();
    if (!station || station->tileMap.empty())
        return;

    const auto &crewList = GameManager::GetServer().GetCrewList();
    for (const auto &entry : crewList)
    {
        const auto &crew = entry.second;
        if (!crew->IsAlive())
            continue;

        Vector2Int floorCrewPos = ToVector2Int(crew->GetPosition());

        if (crew->GetCurrentTile() && crew->GetCurrentTile()->GetPosition() == floorCrewPos)
            continue;

        crew->SetCurrentTile(station->GetTileAtPosition(floorCrewPos, TileDef::Height::FLOOR));
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
                door->KeepClosed();
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
