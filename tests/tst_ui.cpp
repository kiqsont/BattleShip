#include "core/GameSession.h"
#include "ui/BattleView.h"
#include "ui/BoardWidget.h"
#include "ui/CellButton.h"
#include "ui/MainWindow.h"
#include "ui/PlacementView.h"
#include "ui/RulesView.h"
#include "ui/TurnBar.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QWidget>
#include <QtTest>

using namespace bs;

/// 界面层的连通性测试：重点是"点击有没有正确落到 Core 上"，
/// 不校验像素，因此不依赖真实窗口。
class UiTest : public QObject
{
    Q_OBJECT

private slots:
    void mainWindowBuildsMenu();
    void mainWindowIsResizable();
    void menuNavigatesToRulesAndBack();
    void rulesPageScrollsThroughAllSections();
    void placementWiresInputsToBoard();
    void placementBoardClickFillsActiveRow();
    void deleteUsesSameAxisAsPlacement();
    void battleCellClickReachesSession();
    void battleBoardsSeparatedByDivider();
    void battleBoardsScaleWithWindow();
    void placementBoardKeepsFixedSize();
    void battleBackButtonRequestsExit();
    void turnBarUsesFireWording();
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

void UiTest::mainWindowIsResizable()
{
    ui::MainWindow window;

    // 旧版 setFixedSize(480, 900)：规则页在宽屏上只能拉一条很窄的文字栏
    QVERIFY(window.minimumWidth() < 900);
    QVERIFY(window.maximumWidth() > 900);

    window.resize(900, 780);
    QCOMPARE(window.size(), QSize(900, 780));
}

void UiTest::menuNavigatesToRulesAndBack()
{
    ui::MainWindow window;
    auto* stack = window.findChild<QStackedWidget*>();
    QVERIFY(stack != nullptr);

    QPushButton* rulesButton = firstByText<QPushButton>(&window, QStringLiteral("规则说明"));
    QVERIFY(rulesButton != nullptr);
    rulesButton->click();

    auto* rules = window.findChild<ui::RulesView*>();
    QVERIFY(rules != nullptr);
    QCOMPARE(stack->currentWidget(), static_cast<QWidget*>(rules));

    // 页眉的返回按钮要能回到主菜单
    QPushButton* back = firstByText<QPushButton>(rules, QStringLiteral("←"));
    QVERIFY(back != nullptr);
    back->click();
    QVERIFY(stack->currentWidget() != static_cast<QWidget*>(rules));
}

void UiTest::rulesPageScrollsThroughAllSections()
{
    ui::RulesView view;
    view.resize(480, 900);
    view.show();
    QCoreApplication::processEvents();

    auto* area = view.findChild<QScrollArea*>();
    QVERIFY(area != nullptr);
    QWidget* content = area->widget();
    QVERIFY(content != nullptr);

    // 内容比窗口高，必须能滚动
    QVERIFY2(area->verticalScrollBar()->maximum() > 0, "规则页内容没有超出窗口，说明排版塌陷了");

    // 六个章节编号齐全
    QStringList numbers;
    for (QLabel* label : content->findChildren<QLabel*>()) {
        const QString text = label->text();
        if (text.size() == 2 && text.startsWith(QLatin1Char('0')))
            numbers << text;
    }
    numbers.sort();
    QCOMPARE(numbers, QStringList({QStringLiteral("01"), QStringLiteral("02"), QStringLiteral("03"),
                                   QStringLiteral("04"), QStringLiteral("05"), QStringLiteral("06")}));
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

void UiTest::deleteUsesSameAxisAsPlacement()
{
    GameSession session;
    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    const auto edits = view.findChildren<QLineEdit*>();
    // 4 行船只各 2 个输入框（方向改为下拉），之后是删除行的行/列
    QCOMPARE(edits.size(), 10);

    // 放置在(行1, 列2)
    edits.at(0)->setText(QStringLiteral("1"));
    edits.at(1)->setText(QStringLiteral("2"));
    firstByText<QPushButton>(&view, QStringLiteral("放置"))->click();
    QVERIFY(session.board(Side::Player1).hasShipAt(Coord(0, 1)));

    // 删除框用同样的 X=行 / Y=列
    edits.at(8)->setText(QStringLiteral("1"));
    edits.at(9)->setText(QStringLiteral("2"));
    firstByText<QPushButton>(&view, QStringLiteral("删除"))->click();

    QVERIFY(!session.board(Side::Player1).hasShipAt(Coord(0, 1)));
    QCOMPARE(session.board(Side::Player1).placedCount(ShipType::Battleship), 0);
}

void UiTest::placementBoardClickFillsActiveRow()
{
    GameSession session;
    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    auto* board = view.findChild<ui::BoardWidget*>();
    QVERIFY(board != nullptr);
    ui::CellButton* cell = cellAt(board, Coord(4, 6));
    QVERIFY(cell != nullptr);

    sendClick(cell);

    // 默认激活第一行（战舰）：点 (行5, 列7) 应自动填入 5 / 7
    const auto edits = view.findChildren<QLineEdit*>();
    QCOMPARE(edits.at(0)->text(), QStringLiteral("5"));
    QCOMPARE(edits.at(1)->text(), QStringLiteral("7"));
}

void UiTest::battleCellClickReachesSession()
{
    GameSession session;
    // 对方海域放一艘战舰
    session.board(Side::Player2).place(ShipType::Battleship, Coord(0, 0), Direction::Right);

    ui::BattleView view(&session);
    view.startBattle();

    const auto boards = view.findChildren<ui::BoardWidget*>();
    QCOMPARE(boards.size(), 2);

    // Player1 回合：只能打对方海域
    QVERIFY(!cellAt(boards.at(0), Coord(3, 3))->isInteractive());

    ui::CellButton* cell = cellAt(boards.at(1), Coord(0, 0));
    QVERIFY(cell != nullptr);
    QVERIFY(cell->isInteractive());

    sendClick(cell);

    QVERIFY(session.board(Side::Player2).isResolved(Coord(0, 0)));
    QVERIFY(!session.board(Side::Player1).isResolved(Coord(0, 0)));
    // 命中不换手，且只有击沉才累积扫射次数
    QCOMPARE(session.currentSide(), Side::Player1);
    QCOMPARE(session.volleyCredits(Side::Player1), 0);
}

void UiTest::battleBoardsSeparatedByDivider()
{
    GameSession session;
    ui::BattleView view(&session);
    view.startBattle();
    view.resize(480, 900);
    view.show();
    QCoreApplication::processEvents();

    const auto boards = view.findChildren<ui::BoardWidget*>();
    QCOMPARE(boards.size(), 2);

    auto* divider = view.findChild<QWidget*>(QStringLiteral("boardDivider"));
    QVERIFY(divider != nullptr);
    // 两盘之间只隔着「一点空隙 + 一条线 + 一点空隙」，不能粘在一起
    const int gapAbove = divider->y() - (boards.at(0)->y() + boards.at(0)->height());
    const int gapBelow = boards.at(1)->y() - (divider->y() + divider->height());
    QVERIFY2(gapAbove > 0 && gapAbove <= 10, "分割线上方应留一点空隙");
    QVERIFY2(gapBelow > 0 && gapBelow <= 10, "分割线下方应留一点空隙");
    QCOMPARE(gapBelow, gapAbove);
    // 两块棋盘周围都不再有 1..10 坐标标号
    for (ui::BoardWidget* board : boards) {
        for (QLabel* label : board->findChildren<QLabel*>())
            QVERIFY2(label->isHidden(), "棋盘周围不应再有坐标数字");
    }

    const auto bars = view.findChildren<ui::TurnBar*>();
    QCOMPARE(bars.size(), 2);
    // 无滚动条：所有区域都是 BattleView 的直接可见区域
    QVERIFY(view.findChild<QScrollArea*>() == nullptr);
    // 分列首尾
    const int topBar = bars.at(0)->mapToGlobal(QPoint(0, 0)).y();
    const int board0 = boards.at(0)->mapToGlobal(QPoint(0, 0)).y();
    const int board1 = boards.at(1)->mapToGlobal(QPoint(0, 0)).y();
    const int bottomBar = bars.at(1)->mapToGlobal(QPoint(0, 0)).y();
    QVERIFY2(topBar < board0, "第一条行动条应在棋盘上方");
    QVERIFY2(bottomBar > board1, "第二条行动条应在棋盘下方");
}

void UiTest::battleBoardsScaleWithWindow()
{
    GameSession session;
    ui::BattleView view(&session);
    view.startBattle();
    view.resize(480, 900);
    view.show();
    QCoreApplication::processEvents();

    const auto boards = view.findChildren<ui::BoardWidget*>();
    QCOMPARE(boards.size(), 2);
    const QSize small = boards.at(0)->size();
    QVERIFY(small.width() > 100 && small.height() > 100);

    view.resize(900, 1000);
    QCoreApplication::processEvents();
    const QSize large = boards.at(0)->size();
    QVERIFY2(large.width() > small.width() && large.height() > small.height(),
             "棋盘应随窗口按比例变大");
    // 两块棋盘始终等分中间区域
    QCOMPARE(boards.at(0)->size(), boards.at(1)->size());
}

void UiTest::placementBoardKeepsFixedSize()
{
    GameSession session;
    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    // 布置页仍需坐标标号与固定尺寸，不能被对战页的可伸缩模式带跑
    auto* board = view.findChild<ui::BoardWidget*>();
    QVERIFY(board != nullptr);
    QVERIFY2(board->width() == board->height(), "布置页棋盘应保持正方形");
    bool hasShownLabel = false;
    for (QLabel* label : board->findChildren<QLabel*>()) {
        if (!label->isHidden()) {
            hasShownLabel = true;
            break;
        }
    }
    QVERIFY2(hasShownLabel, "布置页应保留 1..10 坐标标号");
}

void UiTest::battleBackButtonRequestsExit()
{
    GameSession session;
    ui::BattleView view(&session);
    view.startBattle();

    QPushButton* back = firstByText<QPushButton>(&view, QStringLiteral("返回主菜单"));
    QVERIFY(back != nullptr);

    QSignalSpy spy(&view, &ui::BattleView::exitRequested);
    back->click();
    QCOMPARE(spy.count(), 1);
}

void UiTest::turnBarUsesFireWording()
{
    GameSession session;
    ui::BattleView view(&session);
    view.startBattle();

    QVERIFY(firstByText<QPushButton>(&view, QStringLiteral("开火")) != nullptr);
    QVERIFY(firstByText<QPushButton>(&view, QStringLiteral("启动")) == nullptr);
}

void UiTest::restartingBattleDoesNotDuplicateCells()
{
    GameSession session;
    session.board(Side::Player2).place(ShipType::Battleship, Coord(0, 0), Direction::Right);

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
