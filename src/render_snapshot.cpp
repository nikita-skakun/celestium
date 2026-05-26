#include "env_effect.hpp"
#include "pawn.hpp"
#include "render_snapshot.hpp"
#include "station.hpp"
#include "tile.hpp"

std::vector<std::shared_ptr<const Pawn>> RenderSnapshot::GetPawnAtPosition(const Vector2 &pos) const
{
    std::vector<std::shared_ptr<const Pawn>> result;
    float radius = (PAWN_DRAW_SIZE * .5f) / TILE_SIZE;
    float r2 = radius * radius;
    for (const auto &kv : pawnList)
    {
        const auto &pawn = kv.second;
        if (pawn && Vector2DistanceSq(pos, pawn->GetPosition()) <= r2)
            result.push_back(pawn);
    }
    return result;
}
