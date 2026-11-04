#ifndef BS_UI_SHIPPALETTE_H
#define BS_UI_SHIPPALETTE_H

#include "core/Types.h"

#include <QPoint>
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

class ThemeButton;

/// 侧栏里的一艘舰：画出舰体与剩余数量，按住即可拖到棋盘上。
///
/// 只负责"发起一次拖拽"，不碰棋盘数据——能不能放由 PlacementView 问 Board。
class ShipTile : public QWidget
{
    Q_OBJECT

public:
    explicit ShipTile(ShipType type, QWidget* parent = nullptr);

    ShipType type() const { return m_type; }
    int remaining() const { return m_remaining; }
    Direction direction() const { return m_direction; }

    void setDirection(Direction direction);
    void setRemaining(int remaining);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void startDrag();

    ShipType m_type = ShipType::Submarine;
    Direction m_direction = Direction::Down;
    int m_remaining = 0;
    bool m_hovered = false;
    bool m_pressed = false;
    QPoint m_pressPos;
};

/// 侧栏底部的回收区：把棋盘上的舰拖到这里，整艘收回。
///
/// 单独做成一个控件而不是让整个侧栏接收拖放：它有自己明确的边界与文案，
/// 玩家一眼知道"拖到这里是删除"。
class HullReturnZone : public QWidget
{
    Q_OBJECT

public:
    explicit HullReturnZone(QWidget* parent = nullptr);

    QSize sizeHint() const override;

signals:
    void hullReturned(bs::Coord sourceHead);

protected:
    void paintEvent(QPaintEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    bool m_hovered = false;
};

/// 布置页右侧的舰队面板：方向选择 + 四种舰 + 回收区。
class ShipPalette : public QWidget
{
    Q_OBJECT

public:
    explicit ShipPalette(QWidget* parent = nullptr);

    /// 新建拖拽时使用的朝向（点一次按钮向右转 90°）。
    Direction direction() const { return m_direction; }
    /// 顺时针转一格：上 → 右 → 下 → 左 → 上。
    void rotateDirection();

    void setSide(Side side);
    void setRemaining(ShipType type, int remaining);

signals:
    void hullReturned(bs::Coord sourceHead);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setDirection(Direction direction);
    void updateSummary();
    ShipTile* tileFor(ShipType type) const;

    std::array<ShipTile*, kShipTypeCount> m_tiles{};
    ThemeButton* m_directionButton = nullptr;
    QLabel* m_summary = nullptr;
    HullReturnZone* m_returnZone = nullptr;

    Direction m_direction = Direction::Down;
    Side m_side = Side::Player1;
    std::array<int, kShipTypeCount> m_remaining{};
};

} // namespace ui
} // namespace bs

#endif // BS_UI_SHIPPALETTE_H
