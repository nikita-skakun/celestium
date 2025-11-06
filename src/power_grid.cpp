#include "component.hpp"
#include "power_grid.hpp"
#include "tile.hpp"

void PowerGrid::Disconnect(const std::shared_ptr<Tile> &parentTile)
{
    if (!parentTile)
        return;

    for (auto it = _consumers.begin(); it != _consumers.end();)
    {
        if (auto ptr = it->second.lock())
        {
            if (ptr->GetParent() == parentTile)
            {
                ptr->SetActive(false);
                it = _consumers.erase(it);
                continue;
            }
        }
        ++it;
    }

    for (auto it = _producers.begin(); it != _producers.end();)
    {
        if (auto ptr = it->second.lock())
        {
            if (ptr->GetParent() == parentTile)
            {
                it = _producers.erase(it);
                continue;
            }
        }
        ++it;
    }

    for (auto it = _batteries.begin(); it != _batteries.end();)
    {
        if (auto ptr = it->second.lock())
        {
            if (ptr->GetParent() == parentTile)
            {
                it = _batteries.erase(it);
                continue;
            }
        }
        ++it;
    }

    if (auto connector = parentTile->GetComponent<PowerConnectorComponent>())
        connector->SetPowerGrid(nullptr);

    dirty = true;
}

float PowerGrid::GetTotalPowerConsumption() const
{
    return std::accumulate(cachedConsumers.begin(), cachedConsumers.end(), 0.f,
                           [](float sum, const auto &wp)
                           { if (auto consumer = wp.lock()) return sum + consumer->GetPowerConsumption(); return sum; });
}

float PowerGrid::GetTotalPowerProduction() const
{
    return std::accumulate(cachedProducers.begin(), cachedProducers.end(), 0.f,
                           [](float sum, const auto &wp)
                           { if (auto producer = wp.lock()) return sum + producer->GetPowerProduction(); return sum; });
}

float PowerGrid::GetTotalBatteryCharge() const
{
    return std::accumulate(cachedBatteries.begin(), cachedBatteries.end(), 0.f,
                           [](float sum, const auto &wp)
                           { if (auto battery = wp.lock()) return sum + battery->GetChargeLevel(); return sum; });
}

float PowerGrid::GetTotalMaxBatteryCharge() const
{
    return std::accumulate(cachedBatteries.begin(), cachedBatteries.end(), 0.f,
                           [](float sum, const auto &wp)
                           { if (auto battery = wp.lock()) return sum + battery->GetMaxChargeLevel(); return sum; });
}

float PowerGrid::GetTotalBatteryCapacity() const
{
    return std::accumulate(cachedBatteries.begin(), cachedBatteries.end(), 0.f,
                           [](float sum, const auto &wp)
                           { if (auto battery = wp.lock()) return sum + (battery->GetMaxChargeLevel() - battery->GetChargeLevel()); return sum; });
}

void PowerGrid::RebuildCaches()
{
    // Remove expired entries
    std::erase_if(_producers, [](auto &entry)
                  { return entry.second.expired(); });

    std::erase_if(_batteries, [](auto &entry)
                  { return entry.second.expired(); });

    std::erase_if(_consumers, [](auto &entry)
                  { return entry.second.expired(); });

    // Rebuild cached weak_ptr lists for faster iteration in Update()
    cachedProducers.clear();
    cachedConsumers.clear();
    cachedBatteries.clear();

    cachedProducers.reserve(_producers.size());
    for (auto &p : _producers)
        cachedProducers.push_back(p.second);

    cachedConsumers.reserve(_consumers.size());
    for (auto &c : _consumers)
        if (c.second.lock()->GetPowerPriority() != PowerConsumerComponent::PowerPriority::OFFLINE)
            cachedConsumers.push_back(c.second);

    cachedBatteries.reserve(_batteries.size());
    for (auto &b : _batteries)
        cachedBatteries.push_back(b.second);

    std::ranges::sort(cachedConsumers, [](const auto &_a, const auto &_b)
                      {
                        auto a = _a.lock();
                        auto b = _b.lock();
                        if (!a || !b) return false;
                        if (a->GetPowerPriority() != b->GetPowerPriority())
                            return a->GetPowerPriority() < b->GetPowerPriority();
                        return a->GetPowerConsumption() > b->GetPowerConsumption(); });

    dirty = false;
}

template <typename T>
static std::vector<std::shared_ptr<T>> LockWeakVector(const std::vector<std::weak_ptr<T>> &in)
{
    std::vector<std::shared_ptr<T>> out;
    out.reserve(in.size());
    for (auto &wp : in)
        if (auto sp = wp.lock())
            out.push_back(std::move(sp));
    return out;
}

void PowerGrid::Update(float deltaTime)
{
    if (dirty)
        RebuildCaches();

    const auto producers = LockWeakVector<PowerProducerComponent>(cachedProducers);
    const auto consumers = LockWeakVector<PowerConsumerComponent>(cachedConsumers);
    auto batteries = LockWeakVector<BatteryComponent>(cachedBatteries);

    std::sort(batteries.begin(), batteries.end(), [](const auto &a, const auto &b)
              { return (a->GetChargeLevel() / a->GetMaxChargeLevel()) > (b->GetChargeLevel() / b->GetMaxChargeLevel()); });

    for (auto &b : batteries)
        b->ResetDeltaCharge();

    float remainingProduction = GetTotalPowerProduction() * deltaTime;

    // First pass: attempt to power consumers from production only
    std::vector<std::shared_ptr<PowerConsumerComponent>> unpoweredConsumers;

    for (auto &c : consumers)
    {
        const float demand = c->GetPowerConsumption() * deltaTime;
        if (remainingProduction >= demand)
        {
            c->SetActive(true);
            remainingProduction -= demand;
        }
        else
            unpoweredConsumers.push_back(c);
    }

    // If there are unpowered consumers, try to cover them using batteries (by priority)
    if (!unpoweredConsumers.empty())
    {
        float batteryRemaining = GetTotalBatteryCharge();
        float batteryUsed = 0.f;

        // Walk consumers in priority order and allocate from batteries until exhausted
        for (auto &c : unpoweredConsumers)
        {
            const float demand = c->GetPowerConsumption() * deltaTime;
            bool powered = batteryRemaining >= demand;
            c->SetActive(powered);
            if (powered)
            {
                batteryRemaining -= demand;
                batteryUsed += demand;
            }
        }

        // Greedy drain: prefer fullest batteries first
        for (auto &b : batteries)
        {
            if (batteryUsed <= 0.f)
                break;
            float removed = b->Drain(batteryUsed);
            b->AccumulateDeltaCharge(-removed / deltaTime);
            batteryUsed -= removed;
        }
    }

    if (remainingProduction > 0.f && GetTotalBatteryCapacity() > 0.f)
    {
        std::reverse(batteries.begin(), batteries.end());

        // Greedy top-up: prefer emptier batteries first
        for (auto &b : batteries)
        {
            float added = b->AddCharge(remainingProduction);
            b->AccumulateDeltaCharge(added / deltaTime);
            remainingProduction -= added;
            if (remainingProduction <= 0.f)
                break;
        }
    }
}