#ifndef BS_UI_PIXMAPS_H
#define BS_UI_PIXMAPS_H

#include "core/Types.h"

#include <QPixmap>

namespace bs {
namespace ui {

/// 棋盘格边长（像素）。界面布局的唯一尺寸来源。
constexpr int kCellSizePx = 40;

/// 贴图缓存。旧版每次 hover 都从资源重新 load + scaled，这里统一做一次并复用。
namespace pixmaps {

const QPixmap& emptyCell();      ///< BasicSquare.png
const QPixmap& traceCell();      ///< trace.png（悬停 / 已选中）
const QPixmap& missCell();       ///< Miss.png
const QPixmap& volleyCell();     ///< togeShoot.png（齐射命中）
const QPixmap& shipBody();       ///< ShipBody.png
const QPixmap& shipHead(Direction direction);
const QPixmap& shipIcon(ShipType type);
const QPixmap& startBackground();

} // namespace pixmaps
} // namespace ui
} // namespace bs

#endif // BS_UI_PIXMAPS_H
