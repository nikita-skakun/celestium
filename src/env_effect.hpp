#pragma once
#include "def_manager.hpp"

struct Crew;
struct PlayerCam;
struct Station;

struct Effect
{
protected:
    std::shared_ptr<EffectDef> effectDef;
    Vector2Int position;
    // [0, 1] range
    float size;

public:
    Effect(const std::string &defName, const Vector2Int &position, float size);
    virtual ~Effect() = default;

    std::shared_ptr<EffectDef> GetEffectDefinition() const { return effectDef; }

    constexpr const Vector2Int &GetPosition() const { return position; };

    constexpr const float &GetSize() const { return size; }
    constexpr void SetSize(float newSize) { size = std::clamp(newSize, 0.f, 1.f); }

    void Render() const;
    std::string GetInfo() const;

    virtual void EffectCrew(Crew &crew, float deltaTime) const = 0;
    virtual void Update(const std::shared_ptr<Station> &station, int index) = 0;

    constexpr float GetRoundedSize() const { return std::ceil(size * effectDef->GetSizeIncrements()) / (float)effectDef->GetSizeIncrements(); }
    constexpr const std::string &GetId() const { return effectDef->GetId(); }
    constexpr std::string GetName() const
    {
        std::string name = GetId();
        std::replace(name.begin(), name.end(), '_', ' ');
        return StringToTitleCase(name);
    }
};

struct FireEffect : Effect
{
    static constexpr float OXYGEN_CONSUMPTION_PER_SECOND = 20.f;
    static constexpr float GROWTH_IF_FED_PER_SECOND = 1.f / 12.f;
    static constexpr float SPREAD_CHANCE_PER_SECOND = .2f;
    static constexpr float DAMAGE_PER_SECOND = 2.f;

    FireEffect(const Vector2Int &position, float size = 0) : Effect("FIRE", position, size) {}

    void EffectCrew(Crew &crew, float deltaTime) const override;
    void Update(const std::shared_ptr<Station> &station, int index) override;

    constexpr float GetOxygenConsumption() const { return OXYGEN_CONSUMPTION_PER_SECOND * GetRoundedSize(); }
};

struct FoamEffect : Effect
{
    // TODO: Later add a way to remove it (maybe automatic despawning?)

    FoamEffect(const Vector2Int &position, float size = 0) : Effect("FOAM", position, size) {}

    constexpr void EffectCrew(Crew &, float) const override {}
    void Update(const std::shared_ptr<Station> &, int) override {}
};