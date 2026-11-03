#include "ui/BoardWidget.h"

#include "ui/GameControls.h"
#include "ui/Pixmaps.h"

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

void BoardWidget::updateFixedSize()
{
    if (m_scalable)
        return;
    const int width = kCellSizePx * kBoardSize + (m_rowLabelsVisible ? kHeaderPx : 0);
    const int height = kCellSizePx * kBoardSize + (m_columnLabelsVisible ? kHeaderPx : 0);
    setFixedSize(width, height);
    layout()->activate();
}

void BoardWidget::setCell(int row, int col, const CellVisual& visual, bool interactive)
{
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize)
        return;
    CellButton* cell = m_cells[row][col];
    cell->setVisual(visual);
    cell->setInteractive(interactive);
}

void BoardWidget::clearCells()
{
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            m_cells[row][col]->setVisual(CellVisual());
            m_cells[row][col]->setInteractive(false);
        }
    }
}

} // namespace ui
} // namespace bs
