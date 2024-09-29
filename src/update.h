#pragma once
#include "astar.h"
#include "camera.h"
#include "crew.h"

void HandleCrewHover(const std::vector<Crew> &crewList, PlayerCam &camera);
void HandleMouseDragging(std::shared_ptr<Station> station, PlayerCam &camera);
void HandleCrewSelection(const std::vector<Crew> &crewList, PlayerCam &camera);
void AssignCrewTasks(std::vector<Crew> &crewList, const PlayerCam &camera);
void HandleCrewTasks(std::vector<Crew> &crewList);
void HandleCrewEnvironment(std::vector<Crew> &crewList);
void UpdateCrewCurrentTile(std::vector<Crew> &crewList, std::shared_ptr<Station> station);
void UpdateTiles(std::shared_ptr<Station> station);
void MouseDeleteExistingConnection(std::shared_ptr<Station> station, const PlayerCam &camera);