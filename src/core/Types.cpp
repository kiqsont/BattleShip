#include "core/Types.h"

namespace bs {

QString sideTitle(Side side)
{
    return side == Side::Player1 ? QStringLiteral("Player 1") : QStringLiteral("Player 2");
}

int lengthOf(ShipType type)
{
    switch (type) {
    case ShipType::Submarine:  return 1;
    case ShipType::Destroyer:  return 2;
    case ShipType::Cruiser:    return 3;
    case ShipType::Battleship: return 4;
    }
    return 1;
}

QString shipTypeName(ShipType type)
{
    switch (type) {
    case ShipType::Submarine:  return QStringLiteral("潜水艇");
    case ShipType::Destroyer:  return QStringLiteral("驱逐舰");
    case ShipType::Cruiser:    return QStringLiteral("巡洋舰");
    case ShipType::Battleship: return QStringLiteral("战舰");
    }
    return QStringLiteral("船只");
}

Direction directionFromToken(const QString& token)
{
    const QString t = token.trimmed().toLower();
    if (t == QLatin1String("x")) return Direction::Up;
    if (t == QLatin1String("z")) return Direction::Right;
    if (t == QLatin1String("y")) return Direction::Left;
    return Direction::Down;
}

QString directionToken(Direction direction)
{
    switch (direction) {
    case Direction::Up:    return QStringLiteral("x");
    case Direction::Right: return QStringLiteral("z");
    case Direction::Left:  return QStringLiteral("y");
    case Direction::Down:  break;
    }
    return QString();
}

} // namespace bs
