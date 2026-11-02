#include "ui/BattleView.h"

#include "ui/BoardWidget.h"
#include "ui/TurnBar.h"

#include <QMessageBox>
#include <QVBoxLayout>

namespace bs {
namespace ui {

BattleView::BattleView(GameSession* session, QWidget* parent)
    : QWidget(parent)
    , m_session(session)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(4);

    for (int i = 0; i < kSideCount; ++i) {
        const Side side = sideOf(i);

        m_bars[i] = new TurnBar(side, this);
        m_boards[i] = new BoardWidget(this);
        m_boards[i]->setGridLabelsVisible(false);

        connect(m_boards[i], &BoardWidget::cellActivated, this,
                [this, side](Coord coord) { onCellActivated(side, coord); });
        connect(m_bars[i], &TurnBar::armRequested, this, [this, side] { onArm(side); });
        connect(m_bars[i], &TurnBar::fireRequested, this, [this, side] { onFire(side); });
        connect(m_bars[i], &TurnBar::cancelRequested, this, [this, side] { onCancel(side); });
    }

    // 版面顺序：Player1 行动条 / 棋盘 / 留白 / 棋盘 / Player2 行动条
    layout->addWidget(m_bars[0], 0, Qt::AlignHCenter);
    layout->addWidget(m_boards[0], 0, Qt::AlignHCenter);
    layout->addStretch(1);
    layout->addWidget(m_boards[1], 0, Qt::AlignHCenter);
    layout->addWidget(m_bars[1], 0, Qt::AlignHCenter);

    connect(m_session, &GameSession::cellChanged, this, [this](Side boardOwner, Coord coord) {
        refreshCell(boardOwner, coord);
    });
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
    QMessageBox::information(this, QStringLiteral("Congratulations"),
                             QStringLiteral("恭喜玩家%1取得胜利").arg(index(winner) + 1),
                             QMessageBox::Close);
    emit exitRequested();
}

} // namespace ui
} // namespace bs
