#include "ui/BoardWidget.h"

#include "ui/GameControls.h"
#include "ui/Pixmaps.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGridLayout>
#include <QLabel>

namespace bs {
namespace ui {
namespace {

constexpr int kHeaderPx = 24;

} // namespace

BoardWidget::BoardWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 10 行 / 10 列等比分配可用空间：固定尺寸模式下仍是每格 kCellSizePx，
    // 可伸缩模式下则随父布局尺寸一起变大变小。
    for (int i = 0; i < kBoardSize; ++i) {
        layout->setRowStretch(i + 1, 1);
        layout->setColumnStretch(i + 1, 1);
    }

    // 行列表头放在第 0 行 / 第 0 列，隐藏时布局会自动忽略它们。
    for (int i = 0; i < kBoardSize; ++i) {
        auto* column = new QLabel(QString::number(i + 1), this);
        column->setAlignment(Qt::AlignCenter);
        column->setFixedSize(kCellSizePx, kHeaderPx);
        styleLabel(column, 12);
        layout->addWidget(column, 0, i + 1);
        m_headerLabels[i] = column;

        auto* row = new QLabel(QString::number(i + 1), this);
        row->setAlignment(Qt::AlignCenter);
        row->setFixedSize(kHeaderPx, kCellSizePx);
        styleLabel(row, 12);
        layout->addWidget(row, i + 1, 0);
        m_headerLabels[kBoardSize + i] = row;
    }

    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            auto* cell = new CellButton(Coord(row, col), this);
            connect(cell, &CellButton::activated, this, &BoardWidget::cellActivated);
            connect(cell, &CellButton::dragRequested, this, &BoardWidget::hullDragRequested);
            m_cells[row][col] = cell;
            layout->addWidget(cell, row + 1, col + 1);
        }
    }

    layout->activate();
    setGridLabelsVisible(false);
}

void BoardWidget::setGridLabelsVisible(bool visible)
{
    m_rowLabelsVisible = visible;
    m_columnLabelsVisible = visible;
    for (QLabel* label : m_headerLabels)
        label->setVisible(visible);
    updateFixedSize();
}

void BoardWidget::setColumnLabelsVisible(bool visible)
{
    m_columnLabelsVisible = visible;
    // 前 10 个是顶部列号，后 10 个是左侧行号。
    for (int i = 0; i < kBoardSize; ++i)
        m_headerLabels[i]->setVisible(visible);
    updateFixedSize();
}

void BoardWidget::setScalable(bool scalable)
{
    m_scalable = scalable;
    if (scalable) {
        // 对战页：无坐标数字、尺寸完全交给外层布局按比例分配。
        setGridLabelsVisible(false);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(kBoardSize * 2, kBoardSize * 2);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    } else {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        updateFixedSize();
    }
    layout()->activate();
}

void BoardWidget::setDropTargetEnabled(bool enabled)
{
    m_dropTarget = enabled;
    setAcceptDrops(enabled);

    // 格子是棋盘的子控件，拖拽时鼠标其实停在一个格子上。格子也声明接收拖放，
    // 再用事件过滤器统一转成"棋盘级"处理，避免依赖 Qt 向上冒泡的具体行为。
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            CellButton* cell = m_cells[row][col];
            cell->setAcceptDrops(enabled);
            if (enabled) {
                cell->installEventFilter(this);
            } else {
                cell->removeEventFilter(this);
            }
        }
    }
    if (!enabled)
        clearDropPreview();
}

void BoardWidget::updateFixedSize()
{
    if (m_scalable)
        return;
    const int width = kCellSizePx * kBoardSize + (m_rowLabelsVisible ? kHeaderPx : 0);
    const int height = kCellSizePx * kBoardSize + (m_columnLabelsVisible ? kHeaderPx : 0);
    setFixedSize(width, height);
    layout()->activate();
}

void BoardWidget::setCell(int row, int col, const CellVisual& visual, bool interactive,
                          bool draggable)
{
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize)
        return;
    CellButton* cell = m_cells[row][col];
    cell->setVisual(visual);
    cell->setInteractive(interactive);
    cell->setDragSource(draggable);
}

void BoardWidget::clearCells()
{
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            m_cells[row][col]->setVisual(CellVisual());
            m_cells[row][col]->setInteractive(false);
            m_cells[row][col]->setDragSource(false);
        }
    }
}

void BoardWidget::showDropPreview(ShipType type, Coord head, Direction direction, bool valid)
{
    if (m_previewActive && m_previewType == type && m_previewHead == head
        && m_previewDirection == direction && m_previewValid == valid) {
        return;
    }

    clearDropPreview();

    m_previewActive = true;
    m_previewType = type;
    m_previewHead = head;
    m_previewDirection = direction;
    m_previewValid = valid;

    const int len = lengthOf(type);
    for (int i = 0; i < len; ++i) {
        const Coord cell = step(head, direction, i);
        if (!cell.isValid())
            continue;

        CellButton* button = m_cells[cell.row][cell.col];
        PreviewCell& backup = m_previewBackup[static_cast<size_t>(m_previewCount++)];
        backup.coord = cell;
        backup.previous = button->visual();

        CellVisual ghost;
        ghost.kind = (i == 0) ? CellVisual::Kind::GhostHead : CellVisual::Kind::GhostBody;
        ghost.ship = type;
        ghost.direction = direction;
        ghost.valid = valid;
        button->setVisual(ghost);
    }
}

void BoardWidget::clearDropPreview()
{
    for (int i = 0; i < m_previewCount; ++i) {
        const PreviewCell& backup = m_previewBackup[static_cast<size_t>(i)];
        if (!backup.coord.isValid())
            continue;
        m_cells[backup.coord.row][backup.coord.col]->setVisual(backup.previous);
    }
    m_previewCount = 0;
    m_previewActive = false;
}

Coord BoardWidget::coordAt(const QPoint& boardPos) const
{
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            if (m_cells[row][col]->geometry().contains(boardPos))
                return Coord(row, col);
        }
    }
    return Coord(-1, -1);
}

bool BoardWidget::handleDragOver(QDragMoveEvent* event, const QPoint& boardPos)
{
    HullPayload payload;
    if (!m_dropTarget || !readHullMime(event->mimeData(), &payload)) {
        event->ignore();
        return false;
    }
    event->acceptProposedAction();
    // 坐标无效（拖到表头或棋盘外）也照发，让视图把虚影撤掉。
    emit hullHovered(coordAt(boardPos), payload);
    return true;
}

bool BoardWidget::handleDrop(QDropEvent* event, const QPoint& boardPos)
{
    HullPayload payload;
    if (!m_dropTarget || !readHullMime(event->mimeData(), &payload)) {
        event->ignore();
        return false;
    }

    const Coord cell = coordAt(boardPos);
    clearDropPreview();
    if (!cell.isValid()) {
        event->ignore();
        return false;
    }

    event->acceptProposedAction();
    emit hullDropped(cell, payload);
    return true;
}

void BoardWidget::handleDragAway()
{
    clearDropPreview();
    emit hullDragLeft();
}

void BoardWidget::dragEnterEvent(QDragEnterEvent* event)
{
    handleDragOver(event, event->pos());
}

void BoardWidget::dragMoveEvent(QDragMoveEvent* event)
{
    handleDragOver(event, event->pos());
}

void BoardWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    QWidget::dragLeaveEvent(event);
    handleDragAway();
}

void BoardWidget::dropEvent(QDropEvent* event)
{
    handleDrop(event, event->pos());
}

bool BoardWidget::eventFilter(QObject* watched, QEvent* event)
{
    auto* cell = qobject_cast<CellButton*>(watched);
    if (!cell || !m_dropTarget)
        return QWidget::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::DragEnter:
    case QEvent::DragMove: {
        auto* dragEvent = static_cast<QDragMoveEvent*>(event);
        return handleDragOver(dragEvent, cell->mapTo(this, dragEvent->pos()));
    }
    case QEvent::DragLeave:
        handleDragAway();
        return true;
    case QEvent::Drop: {
        auto* dropEvent = static_cast<QDropEvent*>(event);
        return handleDrop(dropEvent, cell->mapTo(this, dropEvent->pos()));
    }
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace ui
} // namespace bs
