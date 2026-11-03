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

    /// 是否显示 1..10 的行列编号（布置界面两块都要）。
    void setGridLabelsVisible(bool visible);

    /// 单独控制顶部 1..10 列号：对战页第二块棋盘用它去掉夹在两盘之间的标尺行，
    /// 保留左侧行号用于瞄准。
    void setColumnLabelsVisible(bool visible);

    /// 可伸缩模式：去掉所有坐标数字、格子交给网格布局按可用空间均分，
    /// 棋盘随父布局尺寸变化；默认 false（布置页保持固定尺寸）。
    void setScalable(bool scalable);

    void setCell(int row, int col, const CellVisual& visual, bool interactive);
    void clearCells();

signals:
    void cellActivated(bs::Coord coord);

private:
    void updateFixedSize();

    CellButton* m_cells[kBoardSize][kBoardSize];
    QLabel* m_headerLabels[2 * kBoardSize];
    bool m_rowLabelsVisible = false;
    bool m_columnLabelsVisible = false;
    bool m_scalable = false;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_BOARDWIDGET_H
