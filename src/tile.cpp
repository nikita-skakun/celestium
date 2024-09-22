#include "tile.h"

std::string Tile::GetName() const
{
    std::string name = magic_enum::enum_name(id).data();
    std::replace(name.begin(), name.end(), '_', ' ');
    return ToTitleCase(name);
}