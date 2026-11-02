#ifndef BS_CORE_SHIP_H
#define BS_CORE_SHIP_H

#include "core/Types.h"

#include <vector>

namespace bs {

/// 一艘舰：种类、船头坐标、朝向、被命中格数。
/// 旧版把"船型 / 朝向 / 船体"全部压进一个 int（type + 4/8/12/16），这里彻底拆开。
class Ship
{
public:
    Ship() = default;
    Ship(ShipType type, Coord head, Direction direction);

    ShipType type() const { return m_type; }
    int length() const { return lengthOf(m_type); }
    Coord head() const { return m_head; }
    Direction direction() const { return m_direction; }
    int hitCount() const { return m_hitCount; }
    bool isSunk() const { return m_hitCount >= length(); }

    /// 按船头 + 朝向展开出的所有格子，长度恒为 length()。
    std::vector<Coord> cells() const;
    Coord cellAt(int offset) const;

    bool occupies(Coord coord) const;

    /// 记录一次命中；该格不属于本舰时返回 false。
    bool registerHit(Coord coord);

    void clearHits() { m_hitCount = 0; }

private:
    ShipType m_type = ShipType::Submarine;
    Coord m_head;
    Direction m_direction = Direction::Down;
    int m_hitCount = 0;
};

} // namespace bs

#endif // BS_CORE_SHIP_H
