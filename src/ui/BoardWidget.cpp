#include "ui/BoardWidget.h"

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

    // 行列表头放在第 0 行 / 第 0 列，隐藏时布局会自动忽略它们。
    for (int i = 0; i < kBoardSize; ++i) {
        auto* column = new QLabel(QString::number(i + 1), this);
        column->setAlignment(Qt::AlignCenter);
        column->setFixedSize(kCellSizePx, kHeaderPx);
        layout->addWidget(column, 0, i + 1);
        m_headerLabels[i] = column;

        auto* row = new QLabel(QString::number(i + 1), this);
        row->setAlignment(Qt::AlignCenter);
        row->setFixedSize(kHeaderPx, kCellSizePx);
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
    m_labelsVisible = visible;
    for (QLabel* label : m_headerLabels)
        label->setVisible(visible);

    const int side = kCellSizePx * kBoardSize + (visible ? kHeaderPx : 0);
    setFixedSize(side, side);
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
