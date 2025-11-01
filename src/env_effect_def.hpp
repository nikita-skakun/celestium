#pragma once
#include "utils.hpp"

struct ParticleSystemDef
{
    std::string id;
    std::string onCreateLua;
    std::string onUpdateLua;
    ParticleSystemDef(const std::string &id_, const std::string &onCreateLua_, const std::string &onUpdateLua_)
        : id(id_), onCreateLua(onCreateLua_), onUpdateLua(onUpdateLua_) {}
};

struct EffectDef
{
private:
    std::string id;
    uint16_t sizeIncrements;
    std::vector<ParticleSystemDef> particleSystems;

public:
    EffectDef(const std::string &id, uint16_t sizeIncrements = 1, std::vector<ParticleSystemDef> psystems = {})
        : id(id), sizeIncrements(sizeIncrements), particleSystems(std::move(psystems)) {}

    constexpr const std::string &GetId() const { return id; }
    constexpr uint16_t GetSizeIncrements() const { return sizeIncrements; }
    const std::vector<ParticleSystemDef> &GetParticleSystems() const { return particleSystems; }
};