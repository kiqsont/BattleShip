#ifndef BS_UI_CELLBUTTON_H
#define BS_UI_CELLBUTTON_H

#include "core/Types.h"

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
        ShipBody   ///< 布置阶段的船体
    };

    Kind kind = Kind::Empty;
    ShipType ship = ShipType::Submarine;
    Direction direction = Direction::Down;
};

/// 棋盘格控件。旧版 button 类有 6 个 public bool 供外部随意改写，
/// 这里收敛成 setVisual() / setInteractive() 两个入口。
class CellButton : public QPushButton
{
    Q_OBJECT

public:
    explicit CellButton(Coord coord, QWidget* parent = nullptr);

    Coord coord() const { return m_coord; }

    void setVisual(const CellVisual& visual);
    void setInteractive(bool interactive);
    bool isInteractive() const { return m_interactive; }

signals:
    void activated(bs::Coord coord);

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void applyIcon();
    bool canHover() const;

    Coord m_coord;
    CellVisual m_visual;
    bool m_interactive = false;
    bool m_hovered = false;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_CELLBUTTON_H
