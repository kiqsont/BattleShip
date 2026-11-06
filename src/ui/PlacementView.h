#ifndef BS_UI_PLACEMENTVIEW_H
#define BS_UI_PLACEMENTVIEW_H

#include "core/GameSession.h"
#include "core/Types.h"
#include "ui/ShipDrag.h"

#include <QWidget>

class QLabel;

namespace bs {
namespace ui {

class BoardWidget;
class ShipPalette;

/// 布置界面（替代旧版 Configuration）。
///
/// 交互模型：左边海域、右边舰队侧栏。
/// - 从侧栏把一艘舰拖到海域上，松手落子（拖动过程中棋盘给出半透明虚影）；
/// - 拖动海域上已有的舰可以换位置，船体跟着鼠标、位置不会跳；
/// - 把海域上的舰拖回侧栏底部即整艘收回；
/// - 点「随机」清空当前布置并随机摆好整支舰队，可反复点换一套，之后仍可手动拖放调整。
///
/// 页面只管交互，"能不能放 / 还剩几艘"一律问 Board，不复制一份规则判断。
class PlacementView : public QWidget
{
    Q_OBJECT

public:
    explicit PlacementView(GameSession* session, QWidget* parent = nullptr);

    /// 切换要布置的玩家，棋盘状态保留在 session 里。
    void setSide(Side side);

    /// 落子的唯一入口：新建（fromBoard == false）与挪动（true）共用。
    /// 落点不合法时返回 false，棋盘保持原样。
    bool dropHull(const HullPayload& payload, Coord cell);

    /// 整艘收回（把棋盘上的舰拖回侧栏）。
    bool returnHull(Coord cell);

signals:
    void backRequested();
    void placementFinished();

private:
    void refreshBoard();
    void refreshPalette();
    void commit();
    void randomizeFleet();

    void onHullHovered(Coord cell, const HullPayload& payload);
    void onHullDropped(Coord cell, const HullPayload& payload);
    void onHullDragLeft();
    void onBoardDragRequested(Coord cell);

    /// 这个落点能不能放：越界 / 相撞 / 没余量。拖动虚影与松手都问这一个函数。
    PlacementError checkDrop(const HullPayload& payload, Coord cell) const;
    /// 抓住的是这艘舰的第几格（0 = 船头）。
    int grabOffsetOf(const Ship& ship, Coord cell) const;

    GameSession* m_session = nullptr;
    Side m_side = Side::Player1;

    QLabel* m_title = nullptr;
    BoardWidget* m_board = nullptr;
    ShipPalette* m_palette = nullptr;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_PLACEMENTVIEW_H
