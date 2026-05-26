#include "component.hpp"
#include "tile_def.hpp"

bool TileDef::HasComponent(ComponentType type) const
{
    for (const auto &comp : refComponents)
        if (comp && comp->GetType() == type)
            return true;
    return false;
}
