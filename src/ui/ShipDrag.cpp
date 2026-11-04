#include "ui/ShipDrag.h"

#include "ui/Pixmaps.h"

#include <QDataStream>
#include <QMimeData>
#include <QPainter>

namespace bs {
namespace ui {
namespace {

/// 拖拽只在同一个进程内发生，用私有 MIME 类型即可，不必考虑与外部程序互通。
constexpr const char* kHullMime = "application/x-battleship-hull";

} // namespace

QString hullMimeType()
{
    return QString::fromLatin1(kHullMime);
}

QMimeData* makeHullMime(const HullPayload& payload)
{
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << static_cast<qint32>(payload.type) << static_cast<qint32>(payload.direction)
        << payload.fromBoard << static_cast<qint32>(payload.sourceHead.row)
        << static_cast<qint32>(payload.sourceHead.col) << static_cast<qint32>(payload.grabOffset);

    auto* mime = new QMimeData;
    mime->setData(hullMimeType(), bytes);
    return mime;
}

bool readHullMime(const QMimeData* data, HullPayload* payload)
{
    if (!data || !payload || !data->hasFormat(hullMimeType()))
        return false;

    QDataStream in(data->data(hullMimeType()));
    in.setVersion(QDataStream::Qt_5_12);

    qint32 type = 0;
    qint32 direction = 0;
    bool fromBoard = false;
    qint32 row = 0;
    qint32 col = 0;
    qint32 offset = 0;
    in >> type >> direction >> fromBoard >> row >> col >> offset;
    if (in.status() != QDataStream::Ok)
        return false;

    // 外部数据不可信：枚举值越界时直接拒绝，别让后面的 lengthOf() 拿到脏值。
    if (type < 0 || type >= kShipTypeCount || direction < 0 || direction >= kDirectionCount)
        return false;

    payload->type = static_cast<ShipType>(type);
    payload->direction = static_cast<Direction>(direction);
    payload->fromBoard = fromBoard;
    payload->sourceHead = Coord(row, col);
    payload->grabOffset = offset;
    return true;
}

Coord hullStripCell(Direction direction, int length, int offset)
{
    switch (direction) {
    case Direction::Down:  return Coord(offset, 0);
    case Direction::Up:    return Coord(length - 1 - offset, 0);
    case Direction::Right: return Coord(0, offset);
    case Direction::Left:  return Coord(0, length - 1 - offset);
    }
    return Coord(0, offset);
}

QPixmap renderHull(ShipType type, Direction direction, int cellPx, qreal opacity)
{
    const int len = lengthOf(type);
    const bool vertical = direction == Direction::Up || direction == Direction::Down;
    const QSize size = vertical ? QSize(cellPx, cellPx * len) : QSize(cellPx * len, cellPx);

    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setOpacity(opacity);
    for (int i = 0; i < len; ++i) {
        const Coord slot = hullStripCell(direction, len, i);
        const QRect target(slot.col * cellPx, slot.row * cellPx, cellPx, cellPx);
        const QPixmap& source = (i == 0) ? pixmaps::shipHead(direction) : pixmaps::shipBody();
        painter.drawPixmap(target,
                           source.scaled(target.size(), Qt::IgnoreAspectRatio,
                                         Qt::SmoothTransformation));
    }
    return pixmap;
}

Coord hullHeadForDrop(const HullPayload& payload, Coord dropCell)
{
    if (!payload.fromBoard)
        return dropCell;

    const int offset = qBound(0, payload.grabOffset, lengthOf(payload.type) - 1);
    switch (payload.direction) {
    case Direction::Down:  return Coord(dropCell.row - offset, dropCell.col);
    case Direction::Up:    return Coord(dropCell.row + offset, dropCell.col);
    case Direction::Right: return Coord(dropCell.row, dropCell.col - offset);
    case Direction::Left:  return Coord(dropCell.row, dropCell.col + offset);
    }
    return dropCell;
}

QPoint hullHotSpot(Direction direction, int length, int offset, int cellPx)
{
    const Coord slot = hullStripCell(direction, length, offset);
    return QPoint(slot.col * cellPx + cellPx / 2, slot.row * cellPx + cellPx / 2);
}

} // namespace ui
} // namespace bs
