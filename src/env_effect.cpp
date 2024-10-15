#include "env_effect.hpp"
#include "crew.hpp"

void FireEffect::EffectCrew(Crew &crew, float deltaTime) const
{
    crew.SetHealth(crew.GetHealth() - DAMAGE_PER_SECOND * deltaTime);
}
