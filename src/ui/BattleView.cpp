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

    connect(m_session, &GameSession::cellChanged, this, [this](Side side, Coord coord) {
        refreshCell(side, coord);
    });
    connect(m_session, &GameSession::sideStateChanged, this, [this](Side side) {
        refreshBar(side);
        refreshBoard(side);
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

CellVisual BattleView::visualFor(Side side, Coord coord) const
{
    CellVisual visual;
    const CellView view = m_session->board(side).view(coord);

    if (!view.resolved) {
        if (m_session->isSelected(side, coord))
            visual.kind = CellVisual::Kind::Selected;
        return visual;
    }

    if (!view.occupied) {
        visual.kind = CellVisual::Kind::Miss;
        return visual;
    }

    visual.ship = view.type;
    visual.kind = m_volleyHits[index(side)].contains(coord.flatIndex()) ? CellVisual::Kind::VolleyHit
                                                                      : CellVisual::Kind::Hit;
    return visual;
}

bool BattleView::cellInteractive(Side side, Coord coord) const
{
    if (m_session->phase() != GameSession::Phase::Battle)
        return false;
    if (side != m_session->currentSide())
        return false;
    if (m_session->board(side).isResolved(coord))
        return false;
    if (m_session->isVolleyArmed(side))
        return m_session->isSelectable(side, coord);
    return true;
}

void BattleView::refreshCell(Side side, Coord coord)
{
    m_boards[index(side)]->setCell(coord.row, coord.col, visualFor(side, coord),
                                   cellInteractive(side, coord));
}

void BattleView::refreshBoard(Side side)
{
    BoardWidget* board = m_boards[index(side)];
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            const Coord coord(row, col);
            board->setCell(row, col, visualFor(side, coord), cellInteractive(side, coord));
        }
    }
}

void BattleView::refreshBar(Side side)
{
    TurnBar* bar = m_bars[index(side)];
    const bool active =
        m_session->phase() == GameSession::Phase::Battle && m_session->currentSide() == side;
    bar->setActive(active);
    bar->setBudget(m_session->budget(side));
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

void BattleView::onCellActivated(Side side, Coord coord)
{
    if (m_session->phase() != GameSession::Phase::Battle)
        return;

    m_bars[index(side)]->clearHits();

    if (m_session->isVolleyArmed(side))
        m_session->toggleSelection(side, coord);
    else
        m_session->fireAt(side, coord);
}

void BattleView::onArm(Side side)
{
    m_session->armVolley(side, m_bars[index(side)]->requestedUnits());
}

void BattleView::onFire(Side side)
{
    if (!m_session->isVolleyArmed(side))
        return;

    const std::vector<Coord> selected = m_session->selectedCells(side);
    const GameSession::VolleyOutcome outcome = m_session->fireVolley(side);

    for (const Coord& coord : selected)
        m_volleyHits[index(side)].insert(coord.flatIndex());

    m_bars[index(side)]->setHits(outcome.hits);
    m_bars[index(side)]->clearInput();
    refreshAll();
}

void BattleView::onCancel(Side side)
{
    m_session->cancelVolley(side);
    m_bars[index(side)]->clearInput();
    m_bars[index(side)]->clearHits();
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
