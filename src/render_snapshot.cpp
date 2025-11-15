#include "render_snapshot.hpp"
#include "crew.hpp"
#include "station.hpp"
#include "tile.hpp"
#include "env_effect.hpp"

std::vector<std::shared_ptr<Crew>> RenderSnapshot::GetCrewAtPosition(const Vector2 &pos) const
{
    std::vector<std::shared_ptr<Crew>> result;
    float radius = CREW_RADIUS / TILE_SIZE;
    float r2 = radius * radius;
    for (const auto &crew : crewList)
    {
        if (crew && Vector2DistanceSq(pos, crew->GetPosition()) <= r2)
            result.push_back(crew);
    }
    return result;
}
