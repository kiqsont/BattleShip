#ifndef BS_CORE_BOARD_H
#define BS_CORE_BOARD_H

#include "core/Rules.h"
#include "core/Ship.h"
#include "core/Types.h"

#include <array>
#include <vector>

namespace bs {

/// 单个格子的只读快照，供界面渲染使用。
struct CellView {
    bool occupied = false;   ///< 该格是否有船
    bool resolved = false;   ///< 该格是否已被揭开（命中或落空）
    bool isHead = false;     ///< 是否是船头（布置界面用来画船头贴图）
    ShipType type = ShipType::Submarine;
    Direction direction = Direction::Down;
};

/// 一块 10×10 海域。舰船是对象，格子只记"归哪艘船 / 是否已被揭开"。
/// 这里不弹任何对话框，错误一律用枚举返回，由界面层决定怎么提示。
class Board
{
public:
    Board();

    void clear();
    void clearResolutions();

    // ---- 布置 ----
    PlacementError canPlace(ShipType type, Coord head, Direction direction) const;
    bool place(ShipType type, Coord head, Direction direction);
    bool removeShipAt(Coord coord);
    int placedCount(ShipType type) const;
    int remainingToPlace(ShipType type) const;

    // ---- 对战 ----
    ShotResult shoot(Coord coord);
    bool hasShipAt(Coord coord) const;
    bool isResolved(Coord coord) const;
    bool allSunk() const;
    int sunkCount() const;

    CellView view(Coord coord) const;
    const std::vector<Ship>& ships() const { return m_ships; }

private:
    int shipIndexAt(Coord coord) const;

    std::vector<Ship> m_ships;
    std::array<int, kCellCount> m_owner;      ///< 每个格子的舰船下标，-1 表示空
    std::array<bool, kCellCount> m_resolved;  ///< 每个格子是否已被揭开
};

} // namespace bs

#endif // BS_CORE_BOARD_H
