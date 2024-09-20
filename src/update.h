#pragma once
#include "astar.h"
#include "camera.h"
#include "crew.h"

void HandleCrewHover(const std::vector<Crew> &crewList, PlayerCam &camera);
void HandleCrewSelection(const std::vector<Crew> &crewList, PlayerCam &camera);
void AssignCrewTasks(std::vector<Crew> &crewList, const PlayerCam &camera);
void HandleCrewTasks(float deltaTime, std::vector<Crew> &crewList);
void HandleCrewEnvironment(float deltaTime, std::vector<Crew> &crewList);
void UpdateCrewCurrentTile(std::vector<Crew> &crewList, std::shared_ptr<Station> station);
void UpdateTiles(float deltaTime, std::shared_ptr<Station> station);