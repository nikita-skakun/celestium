#include "render_snapshot.hpp"

std::vector<std::shared_ptr<Crew>> RenderSnapshot::GetCrewAtPosition(const Vector2 &pos, float radius) const
{
    std::vector<std::shared_ptr<Crew>> result;
    float r2 = radius * radius;
    for (const auto &crew : crewList)
    {
        if (crew && Vector2DistanceSq(pos, crew->GetPosition()) <= r2)
            result.push_back(crew);
    }
    return result;
}

std::vector<std::shared_ptr<Tile>> RenderSnapshot::GetTilesAtPosition(const Vector2Int &pos) const
{
    auto it = tileMap.find(pos);
    if (it == tileMap.end())
        return {};
    return it->second;
}

std::vector<std::shared_ptr<Effect>> RenderSnapshot::GetEffectsAtPosition(const Vector2Int &pos) const
{
    std::vector<std::shared_ptr<Effect>> result;
    for (const auto &effect : effects)
    {
        if (effect && effect->GetPosition() == pos)
            result.push_back(effect);
    }
    return result;
}