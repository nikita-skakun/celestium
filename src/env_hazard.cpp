#include "env_hazard.hpp"
#include "crew.hpp"

void FireHazard::EffectCrew(Crew &crew, float deltaTime) const
{
    crew.SetHealth(crew.GetHealth() - DAMAGE_PER_SECOND * deltaTime);
}
