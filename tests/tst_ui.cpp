#include "core/GameSession.h"
#include "ui/BattleView.h"
#include "ui/BoardWidget.h"
#include "ui/CellButton.h"
#include "ui/MainWindow.h"
#include "ui/PlacementView.h"

#include <QApplication>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QtTest>

using namespace bs;

/// 界面层的连通性测试：重点是"点击有没有正确落到 Core 上"，
/// 不校验像素，因此不依赖真实窗口。
class UiTest : public QObject
{
    Q_OBJECT

private slots:
    void mainWindowBuildsMenu();
    void placementWiresInputsToBoard();
    void battleCellClickReachesSession();
    void restartingBattleDoesNotDuplicateCells();
};

namespace {

template <typename T>
T* firstByText(QWidget* root, const QString& text)
{
    for (T* widget : root->findChildren<T*>()) {
        if (widget->text() == text)
            return widget;
    }
    return nullptr;
}

ui::CellButton* cellAt(QWidget* board, Coord coord)
{
    for (ui::CellButton* cell : board->findChildren<ui::CellButton*>()) {
        if (cell->coord() == coord)
            return cell;
    }
    return nullptr;
}

void sendClick(QWidget* widget)
{
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(5, 5), Qt::LeftButton, Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
}

} // namespace

void UiTest::mainWindowBuildsMenu()
{
    ui::MainWindow window;
    QVERIFY(!window.findChildren<QPushButton*>().isEmpty());
    QVERIFY(firstByText<QPushButton>(&window, QStringLiteral("开始")) != nullptr);
    QVERIFY(firstByText<QPushButton>(&window, QStringLiteral("Player1")) != nullptr);
    QVERIFY(firstByText<QPushButton>(&window, QStringLiteral("规则说明")) != nullptr);
}

void UiTest::placementWiresInputsToBoard()
{
    GameSession session;
    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    const auto edits = view.findChildren<QLineEdit*>();
    QVERIFY(edits.size() >= 3);
    // 第一行是战舰：填行、列，方向留空（默认 Down）
    edits.at(0)->setText(QStringLiteral("1"));
    edits.at(1)->setText(QStringLiteral("1"));

    QPushButton* place = firstByText<QPushButton>(&view, QStringLiteral("放置"));
    QVERIFY(place != nullptr);
    place->click();

    QCOMPARE(session.board(Side::Player1).placedCount(ShipType::Battleship), 1);
    QVERIFY(session.board(Side::Player1).hasShipAt(Coord(0, 0)));
    QCOMPARE(session.board(Side::Player1).remainingToPlace(ShipType::Battleship), 0);
}

void UiTest::battleCellClickReachesSession()
{
    GameSession session;
    session.board(Side::Player1).place(ShipType::Battleship, Coord(0, 0), Direction::Right);

    ui::BattleView view(&session);
    view.startBattle();

    const auto boards = view.findChildren<ui::BoardWidget*>();
    QCOMPARE(boards.size(), 2);

    ui::CellButton* cell = cellAt(boards.at(0), Coord(0, 0));
    QVERIFY(cell != nullptr);
    QVERIFY(cell->isInteractive());

    sendClick(cell);

    QVERIFY(session.board(Side::Player1).isResolved(Coord(0, 0)));
    QCOMPARE(session.budget(Side::Player1), 0);
    // 命中不换手
    QCOMPARE(session.currentSide(), Side::Player1);
}

void UiTest::restartingBattleDoesNotDuplicateCells()
{
    GameSession session;
    session.board(Side::Player1).place(ShipType::Battleship, Coord(0, 0), Direction::Right);

    ui::BattleView view(&session);
    view.startBattle();

    const int expected = 2 * kCellCount;
    QCOMPARE(view.findChildren<ui::CellButton*>().size(), expected);

    // 旧版每次点"开始"都会重新 install 一遍，控件与信号连接成倍增长
    view.startBattle();
    view.startBattle();
    QCOMPARE(view.findChildren<ui::CellButton*>().size(), expected);
}

QTEST_MAIN(UiTest)
#include "tst_ui.moc"
