#include "ui/PlacementView.h"

#include "core/Rules.h"
#include "ui/BoardWidget.h"
#include "ui/GameControls.h"
#include "ui/Pixmaps.h"
#include "ui/ShipPalette.h"
#include "ui/Theme.h"

#include <QDrag>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QVBoxLayout>

namespace bs {
namespace ui {

PlacementView::PlacementView(GameSession* session, QWidget* parent)
    : QWidget(parent)
    , m_session(session)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 8, 12, 8);
    root->setSpacing(8);

    m_title = new QLabel(this);
    m_title->setAlignment(Qt::AlignCenter);
    m_title->setFont(theme::displayFont(24));
    QPalette titlePalette = m_title->palette();
    titlePalette.setColor(QPalette::WindowText, theme::ink());
    m_title->setPalette(titlePalette);

    auto* hint = new QLabel(
        QStringLiteral("① 侧栏选方向　② 把舰拖到海域　③ 拖动可换位置　④ 拖回侧栏即收回"
                       "　⑤ 点「随机」一键换一套"), this);
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);
    styleLabel(hint, 13);

    m_board = new BoardWidget(this);
    m_board->setGridLabelsVisible(true);
    m_board->setDropTargetEnabled(true);

    m_palette = new ShipPalette(this);

    auto* body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(10);
    body->addWidget(m_board, 0, Qt::AlignTop);
    body->addWidget(m_palette, 0, Qt::AlignTop);

    auto* buttons = new QHBoxLayout;
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->setSpacing(8);
    buttons->addStretch(1);

    auto* random = new ThemeButton(QStringLiteral("随机"), this);
    random->setFixedWidth(96);
    random->setToolTip(QStringLiteral("清空当前布置，随机摆好全部 %1 艘舰（位置与朝向都随机）；"
                                     "可以反复点击换一套，之后仍可手动拖放调整")
                           .arg(rules::totalShipCount()));
    buttons->addWidget(random);

    auto* save = new ThemeButton(QStringLiteral("保存"), this, ThemeButton::Accent::Seal);
    save->setFixedWidth(96);
    buttons->addWidget(save);

    auto* back = new ThemeButton(QStringLiteral("返回"), this);
    back->setFixedWidth(96);
    buttons->addWidget(back);

    // 整块内容（标题→提示→棋盘+侧栏→按钮）垂直居中：窗口拉高时上下留白，
    // 而不是把棋盘与侧栏拉长。
    root->addStretch(1);
    root->addWidget(m_title);
    root->addWidget(hint);
    root->addLayout(body);
    root->addLayout(buttons);
    root->addStretch(1);

    connect(m_board, &BoardWidget::hullHovered, this, &PlacementView::onHullHovered);
    connect(m_board, &BoardWidget::hullDropped, this, &PlacementView::onHullDropped);
    connect(m_board, &BoardWidget::hullDragLeft, this, &PlacementView::onHullDragLeft);
    connect(m_board, &BoardWidget::hullDragRequested, this, &PlacementView::onBoardDragRequested);
    connect(m_palette, &ShipPalette::hullReturned, this, &PlacementView::returnHull);
    connect(random, &ThemeButton::clicked, this, &PlacementView::randomizeFleet);
    connect(save, &ThemeButton::clicked, this, &PlacementView::commit);
    connect(back, &ThemeButton::clicked, this, &PlacementView::backRequested);

    setSide(Side::Player1);
}

void PlacementView::setSide(Side side)
{
    m_side = side;
    m_title->setText(sideTitle(m_side) + QStringLiteral("  布阵"));
    m_palette->setSide(m_side);
    refreshBoard();
    refreshPalette();
}

void PlacementView::refreshBoard()
{
    // 先撤掉可能残留的虚影，再按棋盘真实状态重画，避免预演覆盖被"固化"。
    m_board->clearDropPreview();

    const Board& board = m_session->board(m_side);
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            const CellView view = board.view(Coord(row, col));
            CellVisual visual;
            if (view.occupied) {
                visual.ship = view.type;
                visual.direction = view.direction;
                visual.kind = view.isHead ? CellVisual::Kind::ShipHead : CellVisual::Kind::ShipBody;
            }
            // 布置阶段点击不做任何事；已放置的舰可以按住拖走。
            m_board->setCell(row, col, visual, false, view.occupied);
        }
    }
}

void PlacementView::refreshPalette()
{
    const Board& board = m_session->board(m_side);
    for (ShipType type : kAllShipTypes)
        m_palette->setRemaining(type, board.remainingToPlace(type));
}

int PlacementView::grabOffsetOf(const Ship& ship, Coord cell) const
{
    int offset = 0;
    switch (ship.direction()) {
    case Direction::Down:  offset = cell.row - ship.head().row; break;
    case Direction::Up:    offset = ship.head().row - cell.row; break;
    case Direction::Right: offset = cell.col - ship.head().col; break;
    case Direction::Left:  offset = ship.head().col - cell.col; break;
    }
    return qBound(0, offset, ship.length() - 1);
}

PlacementError PlacementView::checkDrop(const HullPayload& payload, Coord cell) const
{
    const Board& board = m_session->board(m_side);
    const Coord head = hullHeadForDrop(payload, cell);

    if (payload.fromBoard) {
        // 拖动已放置的舰：校验时把它自己让开，否则挪一格就会跟自己相撞。
        return board.canPlaceIgnoring(payload.type, head, payload.direction, payload.sourceHead);
    }
    if (board.remainingToPlace(payload.type) <= 0)
        return PlacementError::Overlap; // 编制里这型舰已经放完了
    return board.canPlace(payload.type, head, payload.direction);
}

bool PlacementView::dropHull(const HullPayload& payload, Coord cell)
{
    if (!cell.isValid())
        return false;

    Board& board = m_session->board(m_side);
    const Coord head = hullHeadForDrop(payload, cell);

    if (checkDrop(payload, cell) != PlacementError::None) {
        // 落点不合法：棋盘保持原样，只把虚影撤掉。拖动途中已经用朱红提示过了，
        // 这里不再弹窗打断。
        refreshBoard();
        return false;
    }

    if (payload.fromBoard) {
        const Ship* source = board.shipAt(payload.sourceHead);
        if (!source)
            return false;
        if (source->head() == head && source->direction() == payload.direction) {
            refreshBoard(); // 原地松手
            return true;
        }
        const ShipType type = source->type();
        const Coord sourceHead = source->head();
        const Direction sourceDirection = source->direction();
        board.removeShipAt(payload.sourceHead);
        if (!board.place(type, head, payload.direction)) {
            // 上面的校验已经通过，理论上不会失败；真失败了也得把原舰放回去，不能凭空消失。
            board.place(type, sourceHead, sourceDirection);
            refreshBoard();
            refreshPalette();
            return false;
        }
    } else {
        board.place(payload.type, head, payload.direction);
    }

    refreshBoard();
    refreshPalette();
    return true;
}

bool PlacementView::returnHull(Coord cell)
{
    if (!m_session->board(m_side).removeShipAt(cell))
        return false;
    refreshBoard();
    refreshPalette();
    return true;
}

void PlacementView::onHullHovered(Coord cell, const HullPayload& payload)
{
    if (!cell.isValid()) {
        m_board->clearDropPreview();
        return;
    }
    const Coord head = hullHeadForDrop(payload, cell);
    const bool valid = checkDrop(payload, cell) == PlacementError::None;
    m_board->showDropPreview(payload.type, head, payload.direction, valid);
}

void PlacementView::onHullDropped(Coord cell, const HullPayload& payload)
{
    dropHull(payload, cell);
}

void PlacementView::onHullDragLeft()
{
    // 虚影只是视觉覆盖，BoardWidget 已经还原；这里再按数据重画一次，
    // 保证拖动取消 / 拖出棋盘后棋盘与模型完全一致。
    refreshBoard();
}

void PlacementView::onBoardDragRequested(Coord cell)
{
    const Board& board = m_session->board(m_side);
    const Ship* ship = board.shipAt(cell);
    if (!ship)
        return;

    HullPayload payload;
    payload.type = ship->type();
    payload.direction = ship->direction();
    payload.fromBoard = true;
    payload.sourceHead = ship->head();
    payload.grabOffset = grabOffsetOf(*ship, cell);

    // QDrag 由拖放系统接管并在结束时销毁。
    auto* drag = new QDrag(m_board);
    drag->setMimeData(makeHullMime(payload));
    drag->setPixmap(renderHull(payload.type, payload.direction, kCellSizePx, 0.9));
    drag->setHotSpot(hullHotSpot(payload.direction, lengthOf(payload.type), payload.grabOffset,
                                 kCellSizePx));
    drag->exec(Qt::MoveAction);

    m_board->clearDropPreview();
    refreshBoard();
    refreshPalette();
}

void PlacementView::randomizeFleet()
{
    if (!m_session->board(m_side).randomizeFleet()) {
        QMessageBox::warning(this, QStringLiteral("随机布阵"),
                             QStringLiteral("这次没能排下整支舰队，请再点一次「随机」。"),
                             QMessageBox::Close);
        return;
    }
    // 写的是 Board，棋盘与侧栏都按数据重画；之后玩家仍可继续拖放微调。
    refreshBoard();
    refreshPalette();
}

void PlacementView::commit()
{
    const Board& board = m_session->board(m_side);
    bool allPlaced = true;
    for (ShipType type : kAllShipTypes) {
        if (board.remainingToPlace(type) > 0) {
            allPlaced = false;
            break;
        }
    }

    if (allPlaced) {
        const auto answer = QMessageBox::question(this, QStringLiteral("确认保存"),
                                                  QStringLiteral("确定保存该配置吗？"),
                                                  QMessageBox::Save | QMessageBox::Cancel);
        if (answer == QMessageBox::Save)
            emit placementFinished();
        return;
    }

    const auto answer = QMessageBox::warning(this, QStringLiteral("返回主菜单"),
                                             QStringLiteral("没有使用完所有的船只，是否返回主界面？"),
                                             QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::Yes)
        emit placementFinished();
}

} // namespace ui
} // namespace bs
