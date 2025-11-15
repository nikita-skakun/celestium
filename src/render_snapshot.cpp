#include "crew.hpp"
#include "env_effect.hpp"
#include "render_snapshot.hpp"
#include "station.hpp"
#include "tile.hpp"

std::vector<std::shared_ptr<const Crew>> RenderSnapshot::GetCrewAtPosition(const Vector2 &pos) const
{
    std::vector<std::shared_ptr<const Crew>> result;
    float radius = CREW_RADIUS / TILE_SIZE;
    float r2 = radius * radius;
    for (const auto &kv : crewList)
    {
        const auto &crew = kv.second;
        if (crew && Vector2DistanceSq(pos, crew->GetPosition()) <= r2)
            result.push_back(crew);
    }
    return result;
}
