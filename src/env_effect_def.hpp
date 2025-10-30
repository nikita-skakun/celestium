#pragma once
#include "utils.hpp"

struct EffectDef
{
private:
    std::string id;
    uint16_t sizeIncrements;
public:
    EffectDef(const std::string &id, uint16_t sizeIncrements = 1)
        : id(id), sizeIncrements(sizeIncrements) {}

    constexpr const std::string &GetId() const { return id; }
    constexpr uint16_t GetSizeIncrements() const { return sizeIncrements; }
};