#pragma once
#include "utils.hpp"

struct Crew;
struct PlayerCam;
struct Station;

struct Effect
{
    enum Type : u_int8_t
    {
        NONE,
        FIRE,
        FOAM,
    };

protected:
    Vector2Int position;
    // [0, 1] range
    float size;

public:
    constexpr Effect(const Vector2Int &position, float size) : position(position), size(std::clamp(size, 0.f, 1.f)) {}
    virtual ~Effect() = default;

    constexpr const Vector2Int &GetPosition() const { return position; };

    constexpr const float &GetSize() const { return size; }
    constexpr void SetSize(float newSize) { size = std::clamp(newSize, 0.f, 1.f); }
    constexpr virtual Type GetType() const { return Type::NONE; }

    virtual void EffectCrew(Crew &crew, float deltaTime) const = 0;
    virtual void Render(const PlayerCam &camera) const = 0;
    virtual void Update(const std::shared_ptr<Station> &station, int index) = 0;

    constexpr std::string GetName() const { return EnumToName<Type>(GetType()); }
};

struct FireEffect : Effect
{
    static constexpr float SIZE_INCREMENT = 1.f / 8.f;
    static constexpr float OXYGEN_CONSUMPTION_PER_SECOND = 20.f;
    static constexpr float GROWTH_IF_FED_PER_SECOND = 1.f / 12.f;
    static constexpr float SPREAD_CHANCE_PER_SECOND = .2f;
    static constexpr float DAMAGE_PER_SECOND = 2.f;

    constexpr FireEffect(const Vector2Int &position, float size) : Effect(position, size) {}

    void EffectCrew(Crew &crew, float deltaTime) const override;
    void Render(const PlayerCam &camera) const override;
    void Update(const std::shared_ptr<Station> &station, int index) override;

    constexpr float GetRoundedSize() const { return std::ceil(size / SIZE_INCREMENT) * SIZE_INCREMENT; }
    constexpr float GetOxygenConsumption() const { return OXYGEN_CONSUMPTION_PER_SECOND * GetRoundedSize(); }
    constexpr Type GetType() const override { return Type::FIRE; }
};

struct FoamEffect : Effect
{
    //TODO: Later add a way to remove it (maybe automatic despawning?)

    constexpr FoamEffect(const Vector2Int &position, float size) : Effect(position, size) {}

    void Render(const PlayerCam &camera) const override;
    void Update(const std::shared_ptr<Station> &, int) override {}

    constexpr void EffectCrew(Crew &, float) const override {}
    constexpr Type GetType() const override { return Type::FOAM; }
};