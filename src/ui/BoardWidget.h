#ifndef BS_UI_BOARDWIDGET_H
#define BS_UI_BOARDWIDGET_H

#include "core/Types.h"
#include "ui/CellButton.h"

#include <QWidget>

class QLabel;

namespace bs {
namespace ui {

/// 10×10 棋盘控件。旧版在两个界面里各手写了一遍 new + move 的 100/200 个格子，
/// 坐标映射散落在业务代码里；这里只保留一份。
class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardWidget(QWidget* parent = nullptr);

    /// 是否显示 1..10 的行列编号（布置界面需要，对战界面不需要）。
    void setGridLabelsVisible(bool visible);

    void setCell(int row, int col, const CellVisual& visual, bool interactive);
    void clearCells();

signals:
    void cellActivated(bs::Coord coord);

private:
    CellButton* m_cells[kBoardSize][kBoardSize];
    QLabel* m_headerLabels[2 * kBoardSize];
    bool m_labelsVisible = false;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_BOARDWIDGET_H
