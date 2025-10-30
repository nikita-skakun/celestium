#include "asset_manager.hpp"
#include "crew.hpp"
#include "env_effect.hpp"
#include "game_state.hpp"
#include "station.hpp"

Effect::Effect(const std::string &defName, const Vector2Int &position, float s)
    : position(position)
{
    effectDef = DefinitionManager::GetEffectDefinition(defName);
    if (s == 0)
        s = 1.f / effectDef->GetSizeIncrements();
    size = std::clamp(s, 0.f, 1.f);
}

void Effect::Render() const
{
    // Render the effect's sprite with animation
    Vector2 screenSize = Vector2(1, 1) * TILE_SIZE * GameManager::GetCamera().GetZoom() * GetRoundedSize();
    Vector2 startPos = GameManager::WorldToScreen(GetPosition());
    int frame = GetEvenlySpacedIndex(GetTime() * effectDef->GetAnimationSpeed(), effectDef->GetSpriteCount());
    Rectangle sourceRect = Rectangle(frame, 0, 1, 1) * TILE_SIZE;
    DrawTexturePro(AssetManager::GetTexture(effectDef->GetSpriteSheet()), sourceRect, Vector2ToRect(startPos, screenSize), screenSize / 2.f, 0, WHITE);
}

FireEffect::FireEffect(const Vector2Int &position, float size)
    : Effect("FIRE", position, size)
{
    particleSystem.SetBlendMode(BLEND_ADDITIVE);
}

void FireEffect::Render() const
{
    particleSystem.Draw();
}

std::string Effect::GetInfo() const
{
    std::string effectInfo = " - " + GetName();
    effectInfo += "\n   + Size: " + std::to_string((int)(GetRoundedSize() * effectDef->GetSizeIncrements()));
    return effectInfo;
}

void FireEffect::EffectCrew(const std::shared_ptr<Crew> &crew, float deltaTime) const
{
    crew->SetHealth(crew->GetHealth() - DAMAGE_PER_SECOND * deltaTime);
}

static Color ComputeFireColor(int overlapCount)
{
    // Base orange
    unsigned char baseR = 255;
    unsigned char baseG = 110 + rand() % 50;
    unsigned char baseB = 20 + rand() % 30;
    unsigned char baseA = 140 + rand() % 60;

    if (overlapCount > 1)
    {
        float blend = std::min(1.f, 0.4f + 0.3f * (overlapCount - 1));
        baseR = (unsigned char)(baseR * (1.f - blend) + 255 * blend);
        baseG = (unsigned char)(baseG * (1.f - blend) + 255 * blend);
        baseB = (unsigned char)(baseB * (1.f - blend) + 200 * blend);
        baseA = (unsigned char)(baseA * (1.f - blend) + 220 * blend);
    }
    return Color(baseR, baseG, baseB, baseA);
}

void FireEffect::Update(const std::shared_ptr<Station> &station, size_t index)
{
    Particle proto;
    // Random initial offset proportional to fire size
    float spread = std::clamp(0.08f + 0.32f * GetRoundedSize(), 0.01f, 2.f);
    float angle0 = ((rand() / (float)RAND_MAX) * 2.f * PI);
    float radius = spread * std::clamp((rand() / (float)RAND_MAX), 0.f, 1.f);
    proto.position = Vector2(
        (float)GetPosition().x + cosf(angle0) * radius,
        (float)GetPosition().y + sinf(angle0) * radius);
    // Size variety
    float baseSize = std::max(10.f * GetRoundedSize() * GameManager::GetCamera().GetZoom(), 1.f);
    float sizeVar = std::clamp(0.6f + 0.4f * (rand() / (float)RAND_MAX), 0.3f, 1.2f);
    proto.size = std::max(baseSize * sizeVar, 1.f);
    proto.lifetime = 0.5f + 0.3f * (rand() / (float)RAND_MAX);
    proto.age = 0.0f;
    // Velocity: upward, with some horizontal randomness
    float angle = ((rand() / (float)RAND_MAX) - 0.5f) * 1.2f;
    float speed = (1.f + 0.5f * (rand() / (float)RAND_MAX));
    proto.velocity = Vector2(sinf(angle) * speed, -speed);

    // Count overlapping fires at this position
    int overlapCount = 0;
    for (const auto &eff : station->effects)
    {
        if (std::dynamic_pointer_cast<FireEffect>(eff) && eff->GetPosition() == GetPosition())
        {
            ++overlapCount;
        }
    }
    proto.color = ComputeFireColor(overlapCount);

    particleSystem.Emit(proto, 2 + rand() % 2);
    particleSystem.Update(FIXED_DELTA_TIME);

    auto tileWithOxygen = station->GetTileWithComponentAtPosition<OxygenComponent>(GetPosition());
    if (!tileWithOxygen || station->GetEffectOfTypeAtPosition<FoamEffect>(GetPosition()))
    {
        if (index < station->effects.size())
            station->effects.erase(std::begin(station->effects) + index);
        return;
    }

    // Damage durability of all tiles at this position
    for (auto &tile : station->GetTilesWithComponentAtPosition<DurabilityComponent>(GetPosition()))
    {
        auto durability = tile->GetComponent<DurabilityComponent>();
        durability->SetHitpoints(durability->GetHitpoints() - DAMAGE_PER_SECOND * FIXED_DELTA_TIME);
    }

    // Oxygen consumption and fire size
    auto oxygen = tileWithOxygen->GetComponent<OxygenComponent>();
    float oxygenToConsume = GetOxygenConsumption() * FIXED_DELTA_TIME;
    if (oxygen->GetOxygenLevel() < oxygenToConsume * 2.f)
        SetSize(GetSize() * (2.f / 3.f));
    if (oxygen->GetOxygenLevel() < oxygenToConsume)
    {
        oxygen->SetOxygenLevel(0.f);
        if (index < station->effects.size())
            station->effects.erase(std::begin(station->effects) + index);
        return;
    }
    oxygen->SetOxygenLevel(oxygen->GetOxygenLevel() - oxygenToConsume);
    SetSize(GetSize() + (GROWTH_IF_FED_PER_SECOND * FIXED_DELTA_TIME));

    if (!station->GetTileWithComponentAtPosition<SolidComponent>(GetPosition()) && CheckIfEventHappens(SPREAD_CHANCE_PER_SECOND, FIXED_DELTA_TIME))
    {
        std::vector<Vector2Int> possibleOffsets;
        for (auto &dir : CARDINAL_DIRECTIONS)
        {
            auto neighborPos = GetPosition() + DirectionToVector2Int(dir);
            bool neighborHasOxygen = station->GetTileWithComponentAtPosition<OxygenComponent>(neighborPos) != nullptr;
            bool neighborHasFire = station->GetEffectOfTypeAtPosition<FireEffect>(neighborPos) != nullptr;
            bool neighborHasFoam = station->GetEffectOfTypeAtPosition<FoamEffect>(neighborPos) != nullptr;
            if (neighborHasOxygen && !neighborHasFire && !neighborHasFoam)
                possibleOffsets.push_back(DirectionToVector2Int(dir));
        }
        if (!possibleOffsets.empty())
        {
            int selected = RandomIntWithRange(0, static_cast<int>(possibleOffsets.size()) - 1);
            station->effects.push_back(std::make_shared<FireEffect>(GetPosition() + possibleOffsets[selected]));
        }
    }
}
