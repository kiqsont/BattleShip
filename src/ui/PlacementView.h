#ifndef BS_UI_PLACEMENTVIEW_H
#define BS_UI_PLACEMENTVIEW_H

#include "core/GameSession.h"
#include "core/Types.h"

#include <QWidget>

#include <array>

class QLabel;
class QLineEdit;
class QPushButton;

namespace bs {
namespace ui {

class BoardWidget;

/// 布置界面（替代旧版 Configuration）。
/// 只管输入与展示，"能不能放 / 撞没撞 / 剩几艘"全部问 Board。
class PlacementView : public QWidget
{
    Q_OBJECT

public:
    explicit PlacementView(GameSession* session, QWidget* parent = nullptr);

    /// 切换要布置的玩家，棋盘状态保留在 session 里。
    void setSide(Side side);

signals:
    void backRequested();
    void placementFinished();

private:
    struct ShipRow {
        ShipType type = ShipType::Submarine;
        QLabel* remaining = nullptr;
        QLineEdit* x = nullptr;
        QLineEdit* y = nullptr;
        QLineEdit* direction = nullptr;
        QPushButton* place = nullptr;
    };

    QWidget* buildPanel();
    QWidget* buildShipRow(int rowIndex, ShipType type);
    QWidget* buildDeleteRow();

    void refreshBoard();
    void refreshCounters();
    void placeShip(const ShipRow& row);
    void deleteShip();
    void commit();

    /// 读取两个输入框（1 起）并转成 0 起的坐标。
    Coord readCoord(QLineEdit* rowEdit, QLineEdit* colEdit, bool* ok) const;
    void showPlacementError(PlacementError error, ShipType type);

    GameSession* m_session = nullptr;
    Side m_side = Side::Player1;

    QLabel* m_title = nullptr;
    BoardWidget* m_board = nullptr;
    std::array<ShipRow, kShipTypeCount> m_rows;
    QLineEdit* m_deleteX = nullptr;
    QLineEdit* m_deleteY = nullptr;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_PLACEMENTVIEW_H
