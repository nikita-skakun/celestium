#pragma once
#include "utils.hpp"

struct Crew;
struct Station;

void HandleBuildMode(std::shared_ptr<Station> station);
void HandleCrewHover(const std::vector<Crew> &crewList);
void HandleMouseDrag(std::shared_ptr<Station> station);
void HandleCrewSelection(const std::vector<Crew> &crewList);
void AssignCrewTasks(std::vector<Crew> &crewList);
void HandleCrewTasks(std::vector<Crew> &crewList);
void HandleCrewEnvironment(std::vector<Crew> &crewList);
void UpdateCrewCurrentTile(std::vector<Crew> &crewList, std::shared_ptr<Station> station);
void UpdateTiles(std::shared_ptr<Station> station);
void UpdateEnvironmentalEffects(std::shared_ptr<Station> station);
void MouseDeleteExistingConnection(std::shared_ptr<Station> station);