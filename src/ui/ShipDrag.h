#ifndef BS_UI_SHIPDRAG_H
#define BS_UI_SHIPDRAG_H

#include "core/Types.h"

#include <QPixmap>
#include <QPoint>
#include <QString>

class QMimeData;

namespace bs {
namespace ui {

/// 一次拖拽携带的信息。
///
/// 侧栏拖出来的是"新建一艘"（fromBoard = false），棋盘上拖的是"把这艘挪走"
/// （fromBoard = true）。接收方据此决定是往棋盘上加一艘，还是先让原舰让开再落子。
struct HullPayload {
    ShipType type = ShipType::Submarine;
    Direction direction = Direction::Down;
    bool fromBoard = false;
    Coord sourceHead;   ///< fromBoard 时：被拖动那艘舰的船头
    int grabOffset = 0; ///< fromBoard 时：抓住的是船体第几格（0 = 船头）
};

/// 本进程内自用的拖拽 MIME 类型。
QString hullMimeType();

/// 打包 / 解包拖拽数据。调用方负责 delete 返回的 QMimeData。
QMimeData* makeHullMime(const HullPayload& payload);
bool readHullMime(const QMimeData* data, HullPayload* payload);

/// 「沿朝向第 offset 格」在摆正的矩形贴图里落在第几行第几列。
///
/// 贴图永远是正向矩形：向下 = 船头在上、向上 = 船头在下、向右 = 船头在左、
/// 向左 = 船头在右。绘制与拖拽热点都用这一份换算，避免两处各写一遍。
Coord hullStripCell(Direction direction, int length, int offset);

/// 把一艘舰渲染成一条贴图（船头朝向 direction），拖拽时跟着鼠标走。
QPixmap renderHull(ShipType type, Direction direction, int cellPx, qreal opacity = 1.0);

/// 松手落在 dropCell 时，这艘舰的船头应该在哪。
/// 从棋盘上拖的舰保持"抓住的那一格"贴在鼠标下，位置不会跳。
Coord hullHeadForDrop(const HullPayload& payload, Coord dropCell);

/// 船体第 offset 格在贴图里的中心点（算 QDrag 的热点用）。
QPoint hullHotSpot(Direction direction, int length, int offset, int cellPx);

} // namespace ui
} // namespace bs

#endif // BS_UI_SHIPDRAG_H
