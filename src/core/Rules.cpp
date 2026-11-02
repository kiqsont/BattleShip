#include "core/Rules.h"

namespace bs {
namespace rules {

int initialCount(ShipType type)
{
    for (const FleetEntry& entry : kFleet) {
        if (entry.type == type)
            return entry.count;
    }
    return 0;
}

int totalShipCount()
{
    int total = 0;
    for (const FleetEntry& entry : kFleet)
        total += entry.count;
    return total;
}

} // namespace rules
} // namespace bs
