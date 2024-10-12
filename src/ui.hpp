#pragma once
#include "camera.hpp"
#include "crew.hpp"
#include "game_state.hpp"

void DrawTileGrid(const PlayerCam &camera);
void DrawPath(const std::deque<Vector2Int> &path, const Vector2 &startPos, const PlayerCam &camera);
void DrawStationTiles(std::shared_ptr<Station> station, const Texture2D &tileset, const PlayerCam &camera);
void DrawStationOverlays(std::shared_ptr<Station> station, const Texture2D &stationTileset, const Texture2D &iconTileset, const PlayerCam &camera);
void DrawEnvironmentalHazards(std::shared_ptr<Station> station, const Texture2D &fireSpritesheet, const PlayerCam &camera);
void DrawCrew(double timeSinceFixedUpdate, const std::vector<Crew> &crewList, const PlayerCam &camera);
void DrawDragSelectBox(const PlayerCam &camera);
void DrawFpsCounter(float deltaTime, float padding = DEFAULT_PADDING, int fontSize = DEFAULT_FONT_SIZE, const Font &font = GetFontDefault());
void DrawOverlay(const PlayerCam &camera, float padding = DEFAULT_PADDING, int fontSize = DEFAULT_FONT_SIZE, const Font &font = GetFontDefault());
void DrawTooltip(const std::string &tooltip, const Vector2 &pos, const Font &font = GetFontDefault(), float padding = DEFAULT_PADDING, int fontSize = DEFAULT_FONT_SIZE);
void DrawMainTooltip(const std::vector<Crew> &crewList, const PlayerCam &camera, std::shared_ptr<Station> station, const Font &font = GetFontDefault());
void DrawUiButtons(const Texture2D &iconTileset, PlayerCam &camera);
void DrawUi(GameState &state, PlayerCam &camera, const Font &font = GetFontDefault());