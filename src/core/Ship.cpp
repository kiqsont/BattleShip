#include "core/Ship.h"

namespace bs {

Ship::Ship(ShipType type, Coord head, Direction direction)
    : m_type(type)
    , m_head(head)
    , m_direction(direction)
    , m_hitCount(0)
{
}

Coord Ship::cellAt(int offset) const
{
    return step(m_head, m_direction, offset);
}

std::vector<Coord> Ship::cells() const
{
    std::vector<Coord> result;
    const int len = length();
    result.reserve(static_cast<size_t>(len));
    for (int i = 0; i < len; ++i)
        result.push_back(cellAt(i));
    return result;
}

bool Ship::occupies(Coord coord) const
{
    const int len = length();
    for (int i = 0; i < len; ++i) {
        if (cellAt(i) == coord)
            return true;
    }
    return false;
}

bool Ship::registerHit(Coord coord)
{
    if (!occupies(coord))
        return false;
    ++m_hitCount;
    return true;
}

} // namespace bs
