#include "component.hpp"
#include "power_grid.hpp"
#include "tile.hpp"

void PowerGrid::Disconnect(const std::shared_ptr<Tile> &parentTile)
{
    if (!parentTile)
        return;

    // TODO: Replace with loop when the map will have a list of connections
    auto erase_by_parent = [&](auto &...containers)
    {
        (std::erase_if(containers, [&](const auto &entry)
                       { 
            if (auto ptr = entry.second.lock())
                return ptr->GetParent() == parentTile;
            return false; }),
         ...);
    };

    erase_by_parent(_consumers, _producers, _batteries);

    if (auto connector = parentTile->GetComponent<PowerConnectorComponent>())
        connector->SetPowerGrid(nullptr);

    dirty = true;
}

float PowerGrid::GetTotalPowerConsumption() const
{
    return std::accumulate(cachedConsumers.begin(), cachedConsumers.end(), 0.f,
                           [](float sum, const auto &consumer)
                           { return sum + consumer->GetPowerConsumption(); });
}

float PowerGrid::GetTotalPowerProduction() const
{
    return std::accumulate(cachedProducers.begin(), cachedProducers.end(), 0.f,
                           [](float sum, const auto &producer)
                           { return sum + producer->GetPowerProduction(); });
}

float PowerGrid::GetTotalBatteryCharge() const
{
    return std::accumulate(cachedBatteries.begin(), cachedBatteries.end(), 0.f,
                           [](float sum, const auto &battery)
                           { return sum + battery->GetChargeLevel(); });
}

float PowerGrid::GetTotalMaxBatteryCharge() const
{
    return std::accumulate(cachedBatteries.begin(), cachedBatteries.end(), 0.f,
                           [](float sum, const auto &battery)
                           { return sum + battery->GetMaxChargeLevel(); });
}

float PowerGrid::GetTotalBatteryCapacity() const
{
    return std::accumulate(cachedBatteries.begin(), cachedBatteries.end(), 0.f,
                           [](float sum, const auto &battery)
                           { return sum + (battery->GetMaxChargeLevel() - battery->GetChargeLevel()); });
}

float PowerGrid::DrainBatteriesProportionally(float amount)
{
    if (amount <= 0.f || cachedBatteries.empty())
        return 0.f;

    struct BatteryInfo
    {
        std::shared_ptr<BatteryComponent> ptr;
        float charge, maxCharge, soc;
    };
    std::vector<BatteryInfo> bats;
    bats.reserve(cachedBatteries.size());

    for (auto &bat : cachedBatteries)
    {
        const float c = bat->GetChargeLevel();
        const float m = bat->GetMaxChargeLevel();
        if (c > 0.f && m > 0.f)
            bats.emplace_back(bat, c, m, c / m);
    }
    if (bats.empty())
        return 0.f;

    const float totalCharge = std::accumulate(bats.begin(), bats.end(), 0.f,
                                              [](float s, const BatteryInfo &b)
                                              { return s + b.charge; });

    // drain everything if requested amount exceeds available charge
    if (amount >= totalCharge)
        return std::accumulate(bats.begin(), bats.end(), 0.f,
                               [](float s, const BatteryInfo &b)
                               { return s + b.ptr->Drain(b.charge); });

    const float targetTotal = totalCharge - amount;

    std::sort(bats.begin(), bats.end(), [](auto &a, auto &b)
              { return a.soc < b.soc; });

    float sumFixed = 0.f;
    float sumMaxRemaining = std::accumulate(bats.begin(), bats.end(), 0.f,
                                            [](float s, const BatteryInfo &b)
                                            { return s + b.maxCharge; });

    size_t idx = 0;
    float s = 0.f;

    while (idx < bats.size())
    {
        s = (targetTotal - sumFixed) / sumMaxRemaining;
        if (s <= 0.f)
        {
            s = 0.f;
            break;
        }
        if (s <= bats[idx].soc)
            break;

        sumFixed += bats[idx].charge;
        sumMaxRemaining -= bats[idx].maxCharge;
        ++idx;
    }

    float drainedTotal = 0.f;
    for (size_t i = 0; i < bats.size(); ++i)
    {
        const float finalCharge = (i < idx) ? bats[i].charge : std::min(bats[i].charge, bats[i].maxCharge * s);
        const float toDrain = bats[i].charge - finalCharge;
        if (toDrain > 0.f)
            drainedTotal += bats[i].ptr->Drain(toDrain);
    }

    return drainedTotal;
}

void PowerGrid::RebuildCaches()
{
    // Rebuild cached consumers: remove expired and collect non-offline
    std::erase_if(_consumers, [](auto &entry)
                  { return entry.second.expired(); });
    cachedConsumers.clear();
    cachedConsumers.reserve(_consumers.size());
    for (auto &[_k, wptr] : _consumers)
        if (auto consumer = wptr.lock(); consumer && consumer->GetPowerPriority() != PowerConsumerComponent::PowerPriority::OFFLINE)
            cachedConsumers.push_back(consumer);

    std::ranges::sort(cachedConsumers, [](const auto &a, const auto &b)
                      {
                          if (a->GetPowerPriority() != b->GetPowerPriority())
                              return a->GetPowerPriority() < b->GetPowerPriority();
                          return a->GetPowerConsumption() > b->GetPowerConsumption(); });

    // Rebuild producers and batteries: remove expired entries and collect
    std::erase_if(_producers, [](auto &entry)
                  { return entry.second.expired(); });
    cachedProducers.clear();
    cachedProducers.reserve(_producers.size());
    for (auto &[_k, wptr] : _producers)
        if (auto p = wptr.lock())
            cachedProducers.push_back(p);

    std::erase_if(_batteries, [](auto &entry)
                  { return entry.second.expired(); });
    cachedBatteries.clear();
    cachedBatteries.reserve(_batteries.size());
    for (auto &[_k, wptr] : _batteries)
        if (auto b = wptr.lock())
            cachedBatteries.push_back(b);

    dirty = false;
}

void PowerGrid::Update(float deltaTime)
{
    if (dirty)
        RebuildCaches();

    float totalDemand = GetTotalPowerConsumption();
    float availablePower = GetTotalPowerProduction();
    float totalBatteryCharge = GetTotalBatteryCharge();

    float deficit = (totalDemand - availablePower) * deltaTime;

    if (deficit <= 0.f)
    {
        // ✅ Enough production to power everything
        for (const auto &consumer : cachedConsumers)
        {
            consumer->SetActive(true);
        }

        // ✅ Charge batteries with surplus
        float surplus = -deficit;

        // Compute total remaining capacity
        float totalCapacity = GetTotalBatteryCapacity();

        if (totalCapacity <= 0.f || surplus <= 0.f)
            return; // Nothing to charge or nothing to charge with

        // Charge batteries with priority to the emptier ones
        for (const auto &battery : cachedBatteries)
        {
            float remaining = battery->GetMaxChargeLevel() - battery->GetChargeLevel();
            if (remaining <= 0.f)
                continue;

            float shareRatio = remaining / totalCapacity;
            float share = surplus * shareRatio;

            float added = battery->AddCharge(share);
            surplus -= added;

            if (surplus <= 0.f)
                break;
        }
    }
    else if (totalBatteryCharge >= deficit)
    {
        // ✅ Not enough production, but batteries can cover it

        // Power all consumers
        for (const auto &consumer : cachedConsumers)
        {
            consumer->SetActive(true);
        }

        // Drain batteries evenly
        DrainBatteriesProportionally(deficit);
    }
    else
    {
        // ❌ Not enough even with batteries: allocate power by priority

        float usablePower = availablePower + totalBatteryCharge;

        for (const auto &consumer : cachedConsumers)
        {
            float demand = consumer->GetPowerConsumption();
            if (usablePower >= demand)
            {
                consumer->SetActive(true);
                usablePower -= demand;
            }
            else
            {
                consumer->SetActive(false);
            }
        }

        // Drain batteries proportionally if any battery power was used
        float toDrain = std::max(0.f, (availablePower + totalBatteryCharge - usablePower - availablePower)) * deltaTime;
        DrainBatteriesProportionally(toDrain);
    }
}