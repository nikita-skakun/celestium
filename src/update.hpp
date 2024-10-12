#pragma once
#include "astar.hpp"
#include "camera.hpp"
#include "crew.hpp"

void HandleCrewHover(const std::vector<Crew> &crewList, PlayerCam &camera);
void HandleMouseDrag(std::shared_ptr<Station> station, PlayerCam &camera);
void HandleCrewSelection(const std::vector<Crew> &crewList, PlayerCam &camera);
void AssignCrewTasks(std::vector<Crew> &crewList, const PlayerCam &camera);
void HandleCrewTasks(std::vector<Crew> &crewList);
void HandleCrewEnvironment(std::vector<Crew> &crewList);
void UpdateCrewCurrentTile(std::vector<Crew> &crewList, std::shared_ptr<Station> station);
void UpdateTiles(std::shared_ptr<Station> station);
void UpdateEnvironmentalHazards(std::shared_ptr<Station> station);
void MouseDeleteExistingConnection(std::shared_ptr<Station> station, const PlayerCam &camera);