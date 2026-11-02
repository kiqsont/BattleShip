#ifndef BS_CORE_RULES_H
#define BS_CORE_RULES_H

#include "core/Types.h"

#include <array>

namespace bs {
namespace rules {

/// 舰队编制：每种舰的初始数量。改玩法只需要动这张表。
struct FleetEntry {
    ShipType type;
    int count;
};

inline constexpr std::array<FleetEntry, kShipTypeCount> kFleet{{
    {ShipType::Battleship, 1},
    {ShipType::Cruiser, 2},
    {ShipType::Destroyer, 4},
    {ShipType::Submarine, 2},
}};

/// 编制数量（不是已放置数量）。
int initialCount(ShipType type);

/// 编制总数，旧版为 1 + 2 + 4 + 2 = 9。
int totalShipCount();

/// 瞄准输入 1 个单位 = 5 格预算。这是旧版沿用的换算，见 REFACTORING_PLAN 待确认项。
constexpr int kCellsPerVolleyUnit = 5;

} // namespace rules
} // namespace bs

#endif // BS_CORE_RULES_H
