#ifndef BS_UI_BATTLEVIEW_H
#define BS_UI_BATTLEVIEW_H

#include "core/GameSession.h"
#include "core/Types.h"
#include "ui/CellButton.h"

#include <QSet>
#include <QWidget>

namespace bs {
namespace ui {

class BoardWidget;
class TurnBar;

/// 对战界面。只负责"把状态画出来"和"把点击转成意图"，
/// 回合、预算、胜负全部在 GameSession 里。
///
/// 固定竖排、无滚动条：两条工具区各占一小份高度，其余空间全部由两块棋盘铺满。
/// 棋盘格子随窗口尺寸按比例伸缩，中间只隔一条分割线。
class BattleView : public QWidget
{
    Q_OBJECT

public:
    explicit BattleView(GameSession* session, QWidget* parent = nullptr);

    /// 开新一局：清空齐射显示缓存并重置 session。
    void startBattle();

signals:
    void exitRequested();

private:
    void refreshAll();
    void refreshBoard(Side boardOwner);
    void refreshCell(Side boardOwner, Coord coord);
    void refreshBar(Side side);
    CellVisual visualFor(Side boardOwner, Coord coord) const;
    bool cellInteractive(Side boardOwner, Coord coord) const;

    void onCellActivated(Side boardOwner, Coord coord);
    void onArm(Side attacker);
    void onFire(Side attacker);
    void onCancel(Side attacker);
    void onFinished(Side winner);

    GameSession* m_session = nullptr;
    BoardWidget* m_boards[kSideCount] = {nullptr, nullptr};
    TurnBar* m_bars[kSideCount] = {nullptr, nullptr};

    QSet<int> m_volleyHits[kSideCount]; ///< 齐射命中的格子（按棋盘归属），仅用于选择贴图
    bool m_finishedHandled = false;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_BATTLEVIEW_H
