#ifndef BS_UI_BOARDWIDGET_H
#define BS_UI_BOARDWIDGET_H

#include "core/Types.h"
#include "ui/CellButton.h"
#include "ui/ShipDrag.h"

#include <QWidget>

#include <array>

class QLabel;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QEvent;

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

    /// 接收拖放：开启后棋盘能接住从侧栏拖来的舰，也能发出拖动请求。
    void setDropTargetEnabled(bool enabled);

    void setCell(int row, int col, const CellVisual& visual, bool interactive,
                 bool draggable = false);
    void clearCells();

    /// 落点预演：把一艘舰按 head/direction 画成半透明虚影。valid 决定用墨色还是朱红。
    /// 只改视觉、不动棋盘数据；再次调用或 clearDropPreview() 会还原被覆盖的格子。
    void showDropPreview(ShipType type, Coord head, Direction direction, bool valid);
    void clearDropPreview();

signals:
    void cellActivated(bs::Coord coord);
    /// 玩家按住棋盘上的一艘舰开始拖（坐标是抓住的那一格）。
    void hullDragRequested(bs::Coord coord);
    /// 拖动经过某一格；坐标无效表示拖到了棋盘外（含表头）。
    void hullHovered(bs::Coord cell, bs::ui::HullPayload payload);
    void hullDragLeft();
    void hullDropped(bs::Coord cell, bs::ui::HullPayload payload);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct PreviewCell {
        Coord coord;
        CellVisual previous;
    };

    void updateFixedSize();
    Coord coordAt(const QPoint& boardPos) const;
    bool handleDragOver(QDragMoveEvent* event, const QPoint& boardPos);
    bool handleDrop(QDropEvent* event, const QPoint& boardPos);
    void handleDragAway();

    CellButton* m_cells[kBoardSize][kBoardSize];
    QLabel* m_headerLabels[2 * kBoardSize];
    bool m_rowLabelsVisible = false;
    bool m_columnLabelsVisible = false;
    bool m_scalable = false;
    bool m_dropTarget = false;

    // 预演只是视觉覆盖：记下被盖住的原始视觉，清场时原样贴回去。
    std::array<PreviewCell, kBoardSize> m_previewBackup{};
    int m_previewCount = 0;
    bool m_previewActive = false;
    ShipType m_previewType = ShipType::Submarine;
    Coord m_previewHead;
    Direction m_previewDirection = Direction::Down;
    bool m_previewValid = true;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_BOARDWIDGET_H
