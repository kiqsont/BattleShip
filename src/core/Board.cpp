#include "core/Board.h"

namespace bs {

Board::Board()
{
    clear();
}

void Board::clear()
{
    m_ships.clear();
    m_owner.fill(-1);
    m_resolved.fill(false);
}

void Board::clearResolutions()
{
    m_resolved.fill(false);
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
    if (!coord.isValid() || isResolved(coord))
        return ShotResult::AlreadyResolved;

    const size_t flat = static_cast<size_t>(coord.flatIndex());
    m_resolved[flat] = true;

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
