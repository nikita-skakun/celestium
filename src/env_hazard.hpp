#pragma once
#include "utils.hpp"

struct Hazard
{
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
    constexpr virtual std::string GetName() const = 0;
};

struct FireHazard : public Hazard
{
    static constexpr float SIZE_INCREMENT = 1.f / 8.f;
    static constexpr float OXYGEN_CONSUMPTION = 100.f;
    static constexpr float GROWTH_IF_FED = 1.f / 12.f;

    constexpr FireHazard(const Vector2Int &position, float size) : Hazard(position, size) {}

    constexpr float GetRoundedSize() const { return std::ceil(size / SIZE_INCREMENT) * SIZE_INCREMENT; }
    constexpr float GetOxygenConsumption() const { return OXYGEN_CONSUMPTION * GetRoundedSize(); }
    constexpr std::string GetName() const { return "Fire"; }
};