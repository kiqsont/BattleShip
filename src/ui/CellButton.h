#ifndef BS_UI_CELLBUTTON_H
#define BS_UI_CELLBUTTON_H

#include "core/Types.h"

#include <QPixmap>
#include <QPoint>
#include <QPushButton>

namespace bs {
namespace ui {

/// 一个格子该长什么样。由视图算好再交给 CellButton，控件本身不持有任何游戏状态。
struct CellVisual {
    enum class Kind {
        Empty,     ///< 未揭开
        Selected,  ///< 齐射瞄准时被选中
        Miss,      ///< 已揭开 - 落空
        Hit,       ///< 单发命中（显示舰种图标）
        VolleyHit, ///< 齐射命中
        ShipHead,  ///< 布置阶段的船头
        ShipBody,  ///< 布置阶段的船体
        GhostHead, ///< 拖拽落点预演的船头（半透明，不落子）
        GhostBody  ///< 拖拽落点预演的船体
    };

    Kind kind = Kind::Empty;
    ShipType ship = ShipType::Submarine;
    Direction direction = Direction::Down;
    bool sunk = false;  ///< 该格所属舰船是否已沉没（齐射命中时决定要不要暴露船型）
    bool valid = true;  ///< 仅 Ghost：这个落点合不合法（不合法画朱红）

    bool isGhost() const { return kind == Kind::GhostHead || kind == Kind::GhostBody; }
};

/// 棋盘格控件。旧版 button 类有 6 个 public bool 供外部随意改写，
/// 这里收敛成 setVisual() / setInteractive() / setDragSource() 三个入口。
class CellButton : public QPushButton
{
    Q_OBJECT

public:
    explicit CellButton(Coord coord, QWidget* parent = nullptr);

    Coord coord() const { return m_coord; }
    const CellVisual& visual() const { return m_visual; }

    void setVisual(const CellVisual& visual);
    void setInteractive(bool interactive);
    bool isInteractive() const { return m_interactive; }

    /// 布置阶段的"已放置舰船"格：按住拖走整艘舰。
    void setDragSource(bool draggable);
    bool isDragSource() const { return m_dragSource; }

signals:
    void activated(bs::Coord coord);
    void dragRequested(bs::Coord coord);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void applyIcon();
    void updateScaledPixmap();
    bool canHover() const;
    void updateCursor();

    Coord m_coord;
    CellVisual m_visual;
    const QPixmap* m_pixmap = nullptr;
    QPixmap m_scaled;
    bool m_interactive = false;
    bool m_dragSource = false;
    bool m_hovered = false;
    bool m_dragArmed = false;
    QPoint m_dragPressPos;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_CELLBUTTON_H
