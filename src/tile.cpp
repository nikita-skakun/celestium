#include "tile.h"

template <>
struct magic_enum::customize::enum_range<Tile::Height>
{
    static constexpr bool is_flags = true;
};

std::string Tile::GetName() const
{
    std::string name = magic_enum::enum_name(id).data();
    std::replace(name.begin(), name.end(), '_', ' ');
    return ToTitleCase(name);
}