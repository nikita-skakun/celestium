#pragma once
#include "utils.hpp"

struct Hazard
{
    enum Type : u_int8_t
    {
        NONE,
        FIRE,
    };

protected:
    Vector2Int position;
    // [0, 1] range
    float size;

public:
    constexpr Hazard(const Vector2Int &position, float size) : position(position), size(std::clamp(size, 0.f, 1.f)) {}
    virtual ~Hazard() = default;

    constexpr const Vector2Int &GetPosition() const { return position; };

    constexpr const float &GetSize() const { return size; }
    constexpr void SetSize(float newSize) { size = std::clamp(newSize, 0.f, 1.f); }
    constexpr virtual Type GetType() const { return Type::NONE; }

    constexpr std::string GetName() const
    {
        std::string name = std::string(magic_enum::enum_name<Type>(GetType()));
        std::replace(name.begin(), name.end(), '_', ' ');
        return StringToTitleCase(name);
    }
};

struct FireHazard : public Hazard
{
    static constexpr float SIZE_INCREMENT = 1.f / 8.f;
    static constexpr float OXYGEN_CONSUMPTION = 100.f;
    static constexpr float GROWTH_IF_FED = 1.f / 12.f;
    static constexpr float SPREAD_CHANCE_PER_SECOND = .1f;

    constexpr FireHazard(const Vector2Int &position, float size) : Hazard(position, size) {}

    constexpr float GetRoundedSize() const { return std::ceil(size / SIZE_INCREMENT) * SIZE_INCREMENT; }
    constexpr float GetOxygenConsumption() const { return OXYGEN_CONSUMPTION * GetRoundedSize(); }
    constexpr Type GetType() const override { return Type::FIRE; }
};