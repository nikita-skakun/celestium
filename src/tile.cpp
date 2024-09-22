#include "tile.h"

const std::string Tile::names[int(ID::ID_LENGTH)] = {
    "Blue Floor",
    "Wall",
};

std::string Tile::GetName() const
{
    return id == ID::ID_LENGTH ? "INVALID" : names[int(id)];
}