#include "core/Board.h"

#include <QRandomGenerator>

#include <algorithm>
#include <vector>

namespace bs {
namespace {

/// 本次要放的舰，按舰长降序：大舰先占位置，后面的小舰更容易找到空位。
std::vector<ShipType> fleetQueue()
{
    std::vector<ShipType> queue;
    for (const rules::FleetEntry& entry : rules::kFleet) {
        for (int i = 0; i < entry.count; ++i)
            queue.push_back(entry.type);
    }
    std::sort(queue.begin(), queue.end(),
              [](ShipType a, ShipType b) { return lengthOf(a) > lengthOf(b); });
    return queue;
}

/// 从当前所有合法落点里等概率挑一个。用穷举候选而不是"随机试 N 次"，
/// 保证只要还有地方可放就一定放得下。
bool placeOneRandomly(Board& board, ShipType type)
{
    struct Spot {
        Coord head;
        Direction direction;
    };

    std::vector<Spot> spots;
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            const Coord head(row, col);
            for (Direction direction : kAllDirections) {
                if (board.canPlace(type, head, direction) == PlacementError::None)
                    spots.push_back({head, direction});
            }
        }
    }
    if (spots.empty())
        return false;

    const int pick = QRandomGenerator::global()->bounded(static_cast<int>(spots.size()));
    const Spot& spot = spots[static_cast<size_t>(pick)];
    return board.place(type, spot.head, spot.direction);
}

bool fillRandomly(Board& board)
{
    for (ShipType type : fleetQueue()) {
        if (!placeOneRandomly(board, type))
            return false;
    }
    return true;
}

} // namespace

Board::Board()
{
    clear();
}

void Board::clear()
{
    m_ships.clear();
    m_owner.fill(-1);
    m_resolved.fill(false);
    m_volley.fill(false);
}

void Board::clearResolutions()
{
    m_resolved.fill(false);
    m_volley.fill(false);
    for (Ship& ship : m_ships)
        ship.clearHits();
}

int Board::shipIndexAt(Coord coord) const
{
    if (!coord.isValid())
        return -1;
    return m_owner[static_cast<size_t>(coord.flatIndex())];
}

PlacementError Board::canPlace(ShipType type, Coord head, Direction direction) const
{
    return canPlaceIgnoring(type, head, direction, Coord(-1, -1));
}

PlacementError Board::canPlaceIgnoring(ShipType type, Coord head, Direction direction,
                                       Coord ignoreShipAt) const
{
    // shipIndexAt 对无效坐标返回 -1，于是 canPlace 直接走同一条路径。
    const int ignored = shipIndexAt(ignoreShipAt);

    const int len = lengthOf(type);
    for (int i = 0; i < len; ++i) {
        const Coord cell = step(head, direction, i);
        if (!cell.isValid())
            return PlacementError::OutOfBounds;
        const int owner = m_owner[static_cast<size_t>(cell.flatIndex())];
        if (owner >= 0 && owner != ignored)
            return PlacementError::Overlap;
    }
    return PlacementError::None;
}

bool Board::place(ShipType type, Coord head, Direction direction)
{
    // 先整体校验再落子：旧版边写边校验，校验失败会留下半截船体。
    if (canPlace(type, head, direction) != PlacementError::None)
        return false;

    const int newIndex = static_cast<int>(m_ships.size());
    m_ships.emplace_back(type, head, direction);

    const int len = lengthOf(type);
    for (int i = 0; i < len; ++i) {
        const Coord cell = step(head, direction, i);
        m_owner[static_cast<size_t>(cell.flatIndex())] = newIndex;
        m_resolved[static_cast<size_t>(cell.flatIndex())] = false;
        m_volley[static_cast<size_t>(cell.flatIndex())] = false;
    }
    return true;
}

bool Board::removeShipAt(Coord coord)
{
    const int index = shipIndexAt(coord);
    if (index < 0)
        return false;

    for (const Coord& cell : m_ships[static_cast<size_t>(index)].cells()) {
        if (cell.isValid()) {
            m_owner[static_cast<size_t>(cell.flatIndex())] = -1;
            m_resolved[static_cast<size_t>(cell.flatIndex())] = false;
            m_volley[static_cast<size_t>(cell.flatIndex())] = false;
        }
    }

    m_ships.erase(m_ships.begin() + index);

    // 后面的船整体前移一位，重新贴一遍下标
    for (size_t i = 0; i < m_owner.size(); ++i) {
        if (m_owner[i] > index)
            --m_owner[i];
    }
    return true;
}

int Board::placedCount(ShipType type) const
{
    int count = 0;
    for (const Ship& ship : m_ships) {
        if (ship.type() == type)
            ++count;
    }
    return count;
}

int Board::remainingToPlace(ShipType type) const
{
    return rules::initialCount(type) - placedCount(type);
}

bool Board::randomizeFleet()
{
    // 舰队总共只占 4×1 + 3×2 + 2×4 + 1×2 = 20 格，10×10 里必然排得下；
    // 重试只是防备极低概率的"随机把自己堵死"，失败就换一套重新排。
    for (int attempt = 0; attempt < 16; ++attempt) {
        Board candidate;
        if (!fillRandomly(candidate))
            continue;
        *this = candidate;
        return true;
    }
    return false;
}

bool Board::hasShipAt(Coord coord) const
{
    return shipIndexAt(coord) >= 0;
}

const Ship* Board::shipAt(Coord coord) const
{
    const int index = shipIndexAt(coord);
    if (index < 0)
        return nullptr;
    return &m_ships[static_cast<size_t>(index)];
}

bool Board::isResolved(Coord coord) const
{
    if (!coord.isValid())
        return true;
    return m_resolved[static_cast<size_t>(coord.flatIndex())];
}

ShotResult Board::shoot(Coord coord)
{
    return applyShot(coord, false);
}

ShotResult Board::shootVolley(Coord coord)
{
    return applyShot(coord, true);
}

ShotResult Board::applyShot(Coord coord, bool byVolley)
{
    if (!coord.isValid() || isResolved(coord))
        return ShotResult::AlreadyResolved;

    const size_t flat = static_cast<size_t>(coord.flatIndex());
    m_resolved[flat] = true;
    // 扫射格把结果一并盖住：命中/落空/击沉都不再往下暴露，界面只读这个标记。
    m_volley[flat] = byVolley;

    const int index = m_owner[flat];
    if (index < 0)
        return ShotResult::Miss;

    Ship& ship = m_ships[static_cast<size_t>(index)];
    ship.registerHit(coord);
    return ship.isSunk() ? ShotResult::Sunk : ShotResult::Hit;
}

bool Board::allSunk() const
{
    // 与旧版判定保持一致：一艘船都没布置时视为"已清空"。
    for (const Ship& ship : m_ships) {
        if (!ship.isSunk())
            return false;
    }
    return true;
}

int Board::sunkCount() const
{
    int count = 0;
    for (const Ship& ship : m_ships) {
        if (ship.isSunk())
            ++count;
    }
    return count;
}

CellView Board::view(Coord coord) const
{
    CellView result;
    if (!coord.isValid())
        return result;

    const size_t flat = static_cast<size_t>(coord.flatIndex());
    result.resolved = m_resolved[flat];
    result.volley = m_volley[flat];

    const int index = m_owner[flat];
    if (index < 0)
        return result;

    const Ship& ship = m_ships[static_cast<size_t>(index)];
    result.occupied = true;
    result.isHead = (ship.head() == coord);
    result.sunk = ship.isSunk();
    result.type = ship.type();
    result.direction = ship.direction();
    return result;
}

} // namespace bs
