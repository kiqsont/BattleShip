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
    bool volley = false;     ///< 该格是否由「一同扫射」揭开（界面要盖 togeShoot.png）
    bool isHead = false;     ///< 是否是船头（布置界面用来画船头贴图）
    bool sunk = false;       ///< 该格的船是否已被击沉
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
    /// 与 canPlace 相同的校验，但把"位于 ignoreShipAt 的那艘舰"当成已经让开。
    /// 拖动一艘已放置的舰换位置时，它自己不能挡住自己。
    PlacementError canPlaceIgnoring(ShipType type, Coord head, Direction direction,
                                    Coord ignoreShipAt) const;
    bool place(ShipType type, Coord head, Direction direction);
    bool removeShipAt(Coord coord);
    int placedCount(ShipType type) const;
    int remainingToPlace(ShipType type) const;

    /// 清空当前布置，按编制表把整支舰队随机摆好（位置与朝向都随机，可反复调用换一套）。
    /// 只在临时棋盘上排，成功后才整体替换，因此失败时原有布置不受影响。
    /// 与手动拖放共用 canPlace() 的合法性规则：不越界、不重叠，允许舰船相邻。
    bool randomizeFleet();

    /// 该格所属的舰；空格返回 nullptr。指针在下次增删舰船后失效，不要长期持有。
    const Ship* shipAt(Coord coord) const;

    // ---- 对战 ----
    /// 单发：揭开该格，界面按命中/落空显示内容。
    ShotResult shoot(Coord coord);
    /// 一同扫射：揭开该格并打上「扫射」标记，界面一律用 togeShoot.png 遮盖，
    /// 不区分命中、击沉还是落空，也不暴露舰型（与初版 button::shoot() 一致）。
    ShotResult shootVolley(Coord coord);
    bool hasShipAt(Coord coord) const;
    bool isResolved(Coord coord) const;
    bool allSunk() const;
    int sunkCount() const;

    CellView view(Coord coord) const;
    const std::vector<Ship>& ships() const { return m_ships; }

private:
    int shipIndexAt(Coord coord) const;
    ShotResult applyShot(Coord coord, bool byVolley);

    std::vector<Ship> m_ships;
    std::array<int, kCellCount> m_owner;      ///< 每个格子的舰船下标，-1 表示空
    std::array<bool, kCellCount> m_resolved;  ///< 每个格子是否已被揭开
    std::array<bool, kCellCount> m_volley;    ///< 每个格子是否由一同扫射揭开
};

} // namespace bs

#endif // BS_CORE_BOARD_H
