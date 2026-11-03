#include "ui/BattleView.h"

#include "ui/BoardWidget.h"
#include "ui/GameControls.h"
#include "ui/Theme.h"
#include "ui/TurnBar.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QVBoxLayout>

namespace bs {
namespace ui {
namespace {

/// 分割线上下各留一点呼吸空间，避免和棋盘格线粘在一起。
constexpr int kDividerGap = 5;

/// 两块棋盘之间的墨色分割线：取代原来的 1..10 列号标尺行。
class DividerLine : public QWidget
{
public:
    using QWidget::QWidget;

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), theme::ink());
    }
};

} // namespace

BattleView::BattleView(GameSession* session, QWidget* parent)
    : QWidget(parent)
    , m_session(session)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 6, 8, 6);
    outer->setSpacing(0);
    // 不允许布局把自己的最小尺寸写回窗口，窗口始终按 MainWindow 的尺寸走。
    outer->setSizeConstraint(QLayout::SetNoConstraint);

    for (int i = 0; i < kSideCount; ++i) {
        const Side side = sideOf(i);
        m_bars[i] = new TurnBar(side, this);
        m_boards[i] = new BoardWidget(this);
        // 对战页：去掉棋盘周围所有 1..10 数字，格子随窗口按比例伸缩、铺满可用区域。
        m_boards[i]->setScalable(true);

        connect(m_boards[i], &BoardWidget::cellActivated, this,
                [this, side](Coord coord) { onCellActivated(side, coord); });
        connect(m_bars[i], &TurnBar::armRequested, this, [this, side] { onArm(side); });
        connect(m_bars[i], &TurnBar::fireRequested, this, [this, side] { onFire(side); });
        connect(m_bars[i], &TurnBar::cancelRequested, this, [this, side] { onCancel(side); });
    }

    auto* divider = new DividerLine(this);
    divider->setObjectName(QStringLiteral("boardDivider"));
    divider->setFixedHeight(2);
    divider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 顶部工具区：Player1 行动条 + 右侧返回按钮，不单独占一行。
    auto* top = new QHBoxLayout;
    top->setContentsMargins(0, 0, 0, 0);
    top->setSpacing(6);
    top->addWidget(m_bars[0], 1);
    auto* back = new ThemeButton(QStringLiteral("返回主菜单"), this);
    back->setFixedWidth(96);
    top->addWidget(back, 0);
    connect(back, &ThemeButton::clicked, this, &BattleView::exitRequested);

    // 按比例分配：两条工具区各 1 份，两块棋盘各 9 份，中间只留一条分割线。
    // 没有滚动区，窗口多大棋盘就按比例铺多大。
    outer->addLayout(top, 1);
    outer->addWidget(m_boards[0], 9);
    outer->addSpacing(kDividerGap);
    outer->addWidget(divider, 0);
    outer->addSpacing(kDividerGap);
    outer->addWidget(m_boards[1], 9);
    outer->addWidget(m_bars[1], 1);

    connect(m_session, &GameSession::cellChanged, this,
            [this](Side boardOwner, Coord coord) { refreshCell(boardOwner, coord); });
    connect(m_session, &GameSession::sideStateChanged, this, [this](Side side) {
        refreshBar(side);
        refreshBoard(side);
        refreshBoard(otherSide(side));
    });
    connect(m_session, &GameSession::turnChanged, this, [this](Side) { refreshAll(); });
    connect(m_session, &GameSession::finished, this, &BattleView::onFinished);
}

void BattleView::startBattle()
{
    for (int i = 0; i < kSideCount; ++i) {
        m_volleyHits[i].clear();
        m_bars[i]->clearHits();
        m_bars[i]->clearInput();
    }
    m_finishedHandled = false;

    m_session->beginBattle();
    refreshAll();
}

CellVisual BattleView::visualFor(Side boardOwner, Coord coord) const
{
    CellVisual visual;
    const CellView view = m_session->board(boardOwner).view(coord);

    if (!view.resolved) {
        // 选中态只出现在攻击方要打的那块（对方）海域上
        if (boardOwner != m_session->currentSide()
            && m_session->isSelected(m_session->currentSide(), coord)) {
            visual.kind = CellVisual::Kind::Selected;
        }
        return visual;
    }

    if (!view.occupied) {
        visual.kind = CellVisual::Kind::Miss;
        return visual;
    }

    visual.ship = view.type;
    visual.sunk = view.sunk;
    visual.kind = m_volleyHits[index(boardOwner)].contains(coord.flatIndex())
                      ? CellVisual::Kind::VolleyHit
                      : CellVisual::Kind::Hit;
    return visual;
}

bool BattleView::cellInteractive(Side boardOwner, Coord coord) const
{
    if (m_session->phase() != GameSession::Phase::Battle)
        return false;

    const Side attacker = m_session->currentSide();
    // 只允许攻击对方海域
    if (boardOwner == attacker)
        return false;
    if (m_session->board(boardOwner).isResolved(coord))
        return false;
    if (m_session->isVolleyArmed(attacker))
        return m_session->isSelectable(attacker, coord);
    return true;
}

void BattleView::refreshCell(Side boardOwner, Coord coord)
{
    m_boards[index(boardOwner)]->setCell(coord.row, coord.col, visualFor(boardOwner, coord),
                                         cellInteractive(boardOwner, coord));
}

void BattleView::refreshBoard(Side boardOwner)
{
    BoardWidget* board = m_boards[index(boardOwner)];
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            const Coord coord(row, col);
            board->setCell(row, col, visualFor(boardOwner, coord),
                           cellInteractive(boardOwner, coord));
        }
    }
}

void BattleView::refreshBar(Side side)
{
    TurnBar* bar = m_bars[index(side)];
    const bool active =
        m_session->phase() == GameSession::Phase::Battle && m_session->currentSide() == side;
    bar->setActive(active);
    bar->setVolleyCredits(m_session->volleyCredits(side));
    bar->setArmed(m_session->isVolleyArmed(side));
    if (m_session->isVolleyArmed(side))
        bar->setProgress(m_session->selectedCount(side), m_session->volleyTarget(side));
    else
        bar->setProgress(0, 0);
}

void BattleView::refreshAll()
{
    for (int i = 0; i < kSideCount; ++i) {
        const Side side = sideOf(i);
        refreshBoard(side);
        refreshBar(side);
    }
}

void BattleView::onCellActivated(Side boardOwner, Coord coord)
{
    if (m_session->phase() != GameSession::Phase::Battle)
        return;

    const Side attacker = m_session->currentSide();
    if (attacker == boardOwner)
        return;

    m_bars[index(attacker)]->clearHits();

    if (m_session->isVolleyArmed(attacker))
        m_session->toggleSelection(attacker, coord);
    else
        m_session->fireAt(attacker, coord);
}

void BattleView::onArm(Side attacker)
{
    m_session->armVolley(attacker, m_bars[index(attacker)]->requestedVolleys());
}

void BattleView::onFire(Side attacker)
{
    if (!m_session->isVolleyArmed(attacker))
        return;

    const std::vector<Coord> selected = m_session->selectedCells(attacker);
    const GameSession::VolleyOutcome outcome = m_session->fireVolley(attacker);

    // 齐射命中的格子显示通用标记，除非该舰被击沉
    QSet<int>& revealed = m_volleyHits[index(otherSide(attacker))];
    for (const Coord& coord : selected)
        revealed.insert(coord.flatIndex());

    m_bars[index(attacker)]->setHits(outcome.hits);
    m_bars[index(attacker)]->clearInput();
    refreshAll();
}

void BattleView::onCancel(Side attacker)
{
    m_session->cancelVolley(attacker);
    m_bars[index(attacker)]->clearInput();
    m_bars[index(attacker)]->clearHits();
}

void BattleView::onFinished(Side winner)
{
    if (m_finishedHandled)
        return;
    m_finishedHandled = true;

    refreshAll();
    QMessageBox::information(this, QStringLiteral("对局结束"),
                             QStringLiteral("恭喜 %1 取得胜利").arg(sideTitle(winner)),
                             QMessageBox::Close);
    emit exitRequested();
}

} // namespace ui
} // namespace bs
