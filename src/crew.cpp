#include "action.hpp"
#include "crew.hpp"

std::string Crew::GetActionName() const
{
    if (actionQueue.empty())
        return "Idle";

    return actionQueue.front()->GetActionName();
}

std::string Crew::GetInfo() const
{
    std::string info = " - " + GetName();

    if (IsAlive())
    {
        info += std::format("\n   + Health: {:.1f}", GetHealth());
        info += std::format("\n   + Oxygen: {:.0f}", GetOxygen());
        info += std::format("\n   + Action: {}", GetActionName());
    }
    else
    {
        info += "\n   + DEAD";
    }

    return info;
}

std::atomic<uint64_t> Crew::nextInstanceId{1};