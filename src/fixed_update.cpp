#include "direction.hpp"
#include "env_effect.hpp"
#include "fixed_update.hpp"
#include "game_state.hpp"
#include "station.hpp"
#include "tile.hpp"
#include "update.hpp"
#include <chrono>

std::mutex updateMutex;
std::condition_variable fixedUpdateCondition;

void FixedUpdate(double &timeSinceFixedUpdate)
{
    double previousTime = GetTime();

    while (GameManager::IsInGameSim())
    {
        if (!GameManager::IsGamePaused())
        {
            double currentTime = GetTime();
            double deltaTime = currentTime - previousTime;
            previousTime = currentTime;

            timeSinceFixedUpdate += deltaTime;

            while (timeSinceFixedUpdate >= FIXED_DELTA_TIME)
            {
                std::unique_lock<std::mutex> lock(updateMutex);

                HandleCrewActions();
                HandleCrewEnvironment();
                UpdateCrewCurrentTile();
                UpdateEnvironmentalEffects();
                UpdatePowerGrids();
                UpdateTiles();

                // Build and swap new RenderSnapshot for render thread
                auto snapshot = std::make_shared<RenderSnapshot>();
                snapshot->station = GameManager::GetStation();
                snapshot->crewList = GameManager::GetCrewList();
                snapshot->selectedCrewList = GameManager::GetSelectedCrew();
                GameManager::SetRenderSnapshot(snapshot);

                fixedUpdateCondition.notify_all();

                timeSinceFixedUpdate -= FIXED_DELTA_TIME;
            }
        }
        else
        {
            previousTime = GetTime();
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(
                std::max(1, static_cast<int>(1000 * (FIXED_DELTA_TIME - timeSinceFixedUpdate)))));
    }
}
