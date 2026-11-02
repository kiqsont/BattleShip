#include "ui/Pixmaps.h"

#include <QTransform>

namespace bs {
namespace ui {
namespace pixmaps {
namespace {

QPixmap loadScaled(const char* path)
{
    QPixmap pix;
    pix.load(QString::fromLatin1(path));
    if (pix.isNull())
        return pix;
    return pix.scaled(kCellSizePx, kCellSizePx, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

} // namespace

const QPixmap& emptyCell()
{
    static const QPixmap pix = loadScaled(":/Image/BasicSquare.png");
    return pix;
}

const QPixmap& traceCell()
{
    static const QPixmap pix = loadScaled(":/Image/trace.png");
    return pix;
}

const QPixmap& missCell()
{
    static const QPixmap pix = loadScaled(":/Image/Miss.png");
    return pix;
}

const QPixmap& volleyCell()
{
    static const QPixmap pix = loadScaled(":/Image/togeShoot.png");
    return pix;
}

const QPixmap& shipBody()
{
    static const QPixmap pix = loadScaled(":/Image/ShipBody.png");
    return pix;
}

const QPixmap& shipHead(Direction direction)
{
    // 与旧版一致：船头原图朝上，Down 为 0°，Up 旋转 180°，Right 270°，Left 90°。
    static const QPixmap base = loadScaled(":/Image/ShipHead.png");
    static const QPixmap up = base.transformed(QTransform().rotate(180), Qt::SmoothTransformation);
    static const QPixmap right = base.transformed(QTransform().rotate(270), Qt::SmoothTransformation);
    static const QPixmap left = base.transformed(QTransform().rotate(90), Qt::SmoothTransformation);

    switch (direction) {
    case Direction::Up:    return up;
    case Direction::Right: return right;
    case Direction::Left:  return left;
    case Direction::Down:  break;
    }
    return base;
}

const QPixmap& shipIcon(ShipType type)
{
    static const QPixmap battleship = loadScaled(":/Image/B.png");
    static const QPixmap cruiser = loadScaled(":/Image/Cruiser.png");
    static const QPixmap destroyer = loadScaled(":/Image/D.png");
    static const QPixmap submarine = loadScaled(":/Image/S.png");

    switch (type) {
    case ShipType::Battleship: return battleship;
    case ShipType::Cruiser:    return cruiser;
    case ShipType::Destroyer:  return destroyer;
    case ShipType::Submarine:  return submarine;
    }
    return battleship;
}

const QPixmap& startBackground()
{
    static const QPixmap pix = [] {
        QPixmap loaded;
        loaded.load(QStringLiteral(":/Image/StartBackground.png"));
        return loaded;
    }();
    return pix;
}

} // namespace pixmaps
} // namespace ui
} // namespace bs
