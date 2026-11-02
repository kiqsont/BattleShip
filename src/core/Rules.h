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

/// 一次"一起扫射"覆盖的格数。
/// 设计文档（Battle Ship.docx）："一起扫射：攻击五处"、"一同扫射：选五个位置"。
/// 界面上的输入框填的是"扫射次数"，不是格数。
constexpr int kCellsPerVolley = 5;

} // namespace rules
} // namespace bs

#endif // BS_CORE_RULES_H
