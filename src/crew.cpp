#include "crew.hpp"
#include "task.hpp"

std::string Crew::GetActionName() const
{
    if (taskQueue.empty())
        return "Idle";

    return taskQueue.at(0)->GetActionName();
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