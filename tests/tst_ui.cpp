#include "core/GameSession.h"
#include "core/Rules.h"
#include "ui/BattleView.h"
#include "ui/BoardWidget.h"
#include "ui/CellButton.h"
#include "ui/MainWindow.h"
#include "ui/PlacementView.h"
#include "ui/RulesView.h"
#include "ui/ShipDrag.h"
#include "ui/ShipPalette.h"
#include "ui/TurnBar.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
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
    void placementPaletteListsWholeFleet();
    void placementRandomButtonFillsFleet();
    void placementDropPlacesShip();
    void placementRejectsOverlappingDrop();
    void placementHoverShowsGhostPreview();
    void placementBoardDragMovesShip();
    void placementDropOnPaletteReturnsShip();
    void placementCellDragCanStart();
    void placementDirectionButtonCycles();
    void battleCellClickReachesSession();
    void volleyCellsAreCoveredInBattleView();
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

/// 取一格的视觉类型（转成 int 便于 QCOMPARE 打印）。
int visualKind(QWidget* board, Coord coord)
{
    ui::CellButton* cell = cellAt(board, coord);
    return cell ? static_cast<int>(cell->visual().kind) : -1;
}

void sendClick(QWidget* widget)
{
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(5, 5), Qt::LeftButton, Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
}

/// 在某一格上模拟"把一艘舰拖到这里松手"，走的是真实的拖放事件链：
/// 事件发给格子本身（和真实拖拽一致），由 BoardWidget 的事件过滤器转成棋盘级处理。
void sendHullDrop(QWidget* board, Coord coord, const ui::HullPayload& payload)
{
    ui::CellButton* cell = cellAt(board, coord);
    if (!cell)
        return;

    const QScopedPointer<QMimeData> mime(ui::makeHullMime(payload));
    const QPoint pos = cell->rect().center();

    QDragEnterEvent enter(pos, Qt::CopyAction, mime.data(), Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(cell, &enter);

    QDropEvent drop(QPointF(pos), Qt::CopyAction, mime.data(), Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(cell, &drop);
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

void UiTest::placementPaletteListsWholeFleet()
{
    GameSession session;
    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    // 侧栏按编制表列出四种舰，初始剩余数加起来就是整支舰队
    const auto tiles = view.findChildren<ui::ShipTile*>();
    QCOMPARE(tiles.size(), kShipTypeCount);

    int remaining = 0;
    for (ui::ShipTile* tile : tiles)
        remaining += tile->remaining();
    QCOMPARE(remaining, rules::totalShipCount());
}

void UiTest::placementRandomButtonFillsFleet()
{
    GameSession session;
    ui::PlacementView view(&session);
    view.setSide(Side::Player2);

    QPushButton* random = firstByText<QPushButton>(&view, QStringLiteral("随机"));
    QVERIFY(random != nullptr);

    int expectedCells = 0;
    for (const rules::FleetEntry& entry : rules::kFleet)
        expectedCells += entry.count * lengthOf(entry.type);

    random->click();

    const Board& board = session.board(Side::Player2);
    for (const rules::FleetEntry& entry : rules::kFleet)
        QCOMPARE(board.placedCount(entry.type), entry.count);
    QVERIFY(session.board(Side::Player1).ships().empty()); // 只动当前这一方

    // 侧栏剩余数归零
    int remaining = 0;
    for (ui::ShipTile* tile : view.findChildren<ui::ShipTile*>())
        remaining += tile->remaining();
    QCOMPARE(remaining, 0);

    // 棋盘上真的画出了舰体
    int drawn = 0;
    for (ui::CellButton* cell : view.findChildren<ui::CellButton*>()) {
        const auto kind = cell->visual().kind;
        if (kind == ui::CellVisual::Kind::ShipHead || kind == ui::CellVisual::Kind::ShipBody)
            ++drawn;
    }
    QCOMPARE(drawn, expectedCells);

    // 再点一次换一套，仍然是合法满编（随机布阵覆盖而非叠加）
    random->click();
    for (const rules::FleetEntry& entry : rules::kFleet)
        QCOMPARE(session.board(Side::Player2).placedCount(entry.type), entry.count);
    int drawnAgain = 0;
    for (ui::CellButton* cell : view.findChildren<ui::CellButton*>()) {
        const auto kind = cell->visual().kind;
        if (kind == ui::CellVisual::Kind::ShipHead || kind == ui::CellVisual::Kind::ShipBody)
            ++drawnAgain;
    }
    QCOMPARE(drawnAgain, expectedCells);
}

void UiTest::placementDropPlacesShip()
{
    GameSession session;
    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    auto* board = view.findChild<ui::BoardWidget*>();
    QVERIFY(board != nullptr);

    ui::HullPayload payload;
    payload.type = ShipType::Battleship;
    payload.direction = Direction::Right;
    sendHullDrop(board, Coord(0, 1), payload);

    QCOMPARE(session.board(Side::Player1).placedCount(ShipType::Battleship), 1);
    QVERIFY(session.board(Side::Player1).hasShipAt(Coord(0, 1)));
    QVERIFY(session.board(Side::Player1).hasShipAt(Coord(0, 4)));
    QCOMPARE(session.board(Side::Player1).remainingToPlace(ShipType::Battleship), 0);

    // 侧栏的剩余数要跟着变，否则玩家会以为还能再拖一艘
    for (ui::ShipTile* tile : view.findChildren<ui::ShipTile*>()) {
        if (tile->type() == ShipType::Battleship)
            QCOMPARE(tile->remaining(), 0);
    }
}

void UiTest::placementRejectsOverlappingDrop()
{
    GameSession session;
    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    auto* board = view.findChild<ui::BoardWidget*>();
    QVERIFY(board != nullptr);

    ui::HullPayload battleship;
    battleship.type = ShipType::Battleship;
    battleship.direction = Direction::Right;
    sendHullDrop(board, Coord(0, 0), battleship);
    QCOMPARE(session.board(Side::Player1).placedCount(ShipType::Battleship), 1);

    // 巡洋舰落在战舰身上：必须被拒绝，且不留下半截船体
    ui::HullPayload cruiser;
    cruiser.type = ShipType::Cruiser;
    cruiser.direction = Direction::Down;
    sendHullDrop(board, Coord(0, 2), cruiser);

    QCOMPARE(session.board(Side::Player1).placedCount(ShipType::Cruiser), 0);
    QVERIFY(!session.board(Side::Player1).hasShipAt(Coord(1, 2)));
}

void UiTest::placementHoverShowsGhostPreview()
{
    GameSession session;
    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    auto* board = view.findChild<ui::BoardWidget*>();
    QVERIFY(board != nullptr);
    ui::CellButton* cell = cellAt(board, Coord(2, 3));
    QVERIFY(cell != nullptr);

    const QScopedPointer<QMimeData> mime(
        ui::makeHullMime({ShipType::Battleship, Direction::Right, false, Coord(), 0}));
    QDragEnterEvent enter(cell->rect().center(), Qt::CopyAction, mime.data(), Qt::LeftButton,
                          Qt::NoModifier);
    QApplication::sendEvent(cell, &enter);

    // 拖过棋盘时给出虚影，但数据不能被预演污染
    QCOMPARE(visualKind(board, Coord(2, 3)), static_cast<int>(ui::CellVisual::Kind::GhostHead));
    QCOMPARE(visualKind(board, Coord(2, 6)), static_cast<int>(ui::CellVisual::Kind::GhostBody));
    QCOMPARE(session.board(Side::Player1).placedCount(ShipType::Battleship), 0);

    QDragLeaveEvent leave;
    QApplication::sendEvent(cell, &leave);
    QCOMPARE(visualKind(board, Coord(2, 3)), static_cast<int>(ui::CellVisual::Kind::Empty));
}

void UiTest::placementBoardDragMovesShip()
{
    GameSession session;
    session.board(Side::Player1).place(ShipType::Battleship, Coord(0, 0), Direction::Right);

    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    auto* board = view.findChild<ui::BoardWidget*>();
    QVERIFY(board != nullptr);

    // 抓住船体第 2 格（0,1）拖到 (5,5)：船头应该落在 (5,4)，位置不能跳
    ui::HullPayload payload;
    payload.type = ShipType::Battleship;
    payload.direction = Direction::Right;
    payload.fromBoard = true;
    payload.sourceHead = Coord(0, 0);
    payload.grabOffset = 1;
    sendHullDrop(board, Coord(5, 5), payload);

    QVERIFY(!session.board(Side::Player1).hasShipAt(Coord(0, 0)));
    QVERIFY(session.board(Side::Player1).hasShipAt(Coord(5, 4)));
    QVERIFY(session.board(Side::Player1).hasShipAt(Coord(5, 7)));
    QCOMPARE(session.board(Side::Player1).placedCount(ShipType::Battleship), 1);
}

void UiTest::placementDropOnPaletteReturnsShip()
{
    GameSession session;
    session.board(Side::Player1).place(ShipType::Battleship, Coord(0, 0), Direction::Right);

    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    auto* returnZone = view.findChild<ui::HullReturnZone*>();
    QVERIFY(returnZone != nullptr);

    const QScopedPointer<QMimeData> mime(
        ui::makeHullMime({ShipType::Battleship, Direction::Right, true, Coord(0, 0), 0}));
    const QPoint pos = returnZone->rect().center();

    QDragEnterEvent enter(pos, Qt::MoveAction, mime.data(), Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(returnZone, &enter);
    QDropEvent drop(QPointF(pos), Qt::MoveAction, mime.data(), Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(returnZone, &drop);

    QVERIFY(!session.board(Side::Player1).hasShipAt(Coord(0, 0)));
    QCOMPARE(session.board(Side::Player1).placedCount(ShipType::Battleship), 0);
}

void UiTest::placementCellDragCanStart()
{
    // 棋盘上已放置的舰要能"按住拖动"。这里只验证按下 + 移动会发出拖动请求，
    // 不真的起 QDrag（它会进事件循环，测试里没法收敛）。
    ui::BoardWidget board;
    board.setGridLabelsVisible(false);
    board.setDropTargetEnabled(true);

    ui::CellVisual visual;
    visual.kind = ui::CellVisual::Kind::ShipHead;
    visual.ship = ShipType::Battleship;
    visual.direction = Direction::Right;
    board.setCell(0, 0, visual, false, true);

    ui::CellButton* cell = cellAt(&board, Coord(0, 0));
    QVERIFY(cell != nullptr);
    QVERIFY(cell->isDragSource());

    int requests = 0;
    QObject::connect(&board, &ui::BoardWidget::hullDragRequested,
                     [&requests](Coord) { ++requests; });

    QMouseEvent press(QEvent::MouseButtonPress, QPointF(10, 10), Qt::LeftButton, Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(cell, &press);

    QMouseEvent move(QEvent::MouseMove, QPointF(40, 40), Qt::LeftButton, Qt::LeftButton,
                     Qt::NoModifier);
    QApplication::sendEvent(cell, &move);

    QCOMPARE(requests, 1);
}

void UiTest::placementDirectionButtonCycles()
{
    GameSession session;
    ui::PlacementView view(&session);
    view.setSide(Side::Player1);

    auto* palette = view.findChild<ui::ShipPalette*>();
    QVERIFY(palette != nullptr);

    // 只有一个方向按钮，初始显示「↑ 上」（文案按"看起来船头朝哪"标，不是枚举名）
    QPushButton* rotate = firstByText<QPushButton>(palette, QStringLiteral("↑ 上"));
    QVERIFY(rotate != nullptr);

    // 点一下顺时针转一格：上 → 右 → 下 → 左 → 上
    const QStringList expected{QStringLiteral("→ 右"), QStringLiteral("↓ 下"),
                               QStringLiteral("← 左"), QStringLiteral("↑ 上")};
    for (const QString& text : expected) {
        rotate->click();
        QCOMPARE(rotate->text(), text);
        // 侧栏里的舰要跟着转，拖出去才是新朝向
        for (ui::ShipTile* tile : palette->findChildren<ui::ShipTile*>()) {
            QCOMPARE(static_cast<int>(tile->direction()),
                     static_cast<int>(palette->direction()));
        }
    }
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

void UiTest::volleyCellsAreCoveredInBattleView()
{
    GameSession session;
    session.board(Side::Player2).place(ShipType::Battleship, Coord(0, 0), Direction::Right);
    session.board(Side::Player2).place(ShipType::Submarine, Coord(9, 9), Direction::Down);
    session.board(Side::Player2).place(ShipType::Cruiser, Coord(3, 0), Direction::Down);

    ui::BattleView view(&session);
    view.startBattle();
    const auto boards = view.findChildren<ui::BoardWidget*>();
    QCOMPARE(boards.size(), 2);
    QWidget* defenderBoard = boards.at(1);

    // 直接走 session（不经过 BattleView::onFire）：只靠 cellChanged 信号刷新，
    // 贴图也必须是对的——这正是把遮盖标记放进 Board 的目的。
    session.fireAt(Side::Player1, Coord(9, 9)); // 击沉潜水艇，得一次扫射
    QVERIFY(session.armVolley(Side::Player1, 1));
    for (int col = 0; col < 5; ++col)
        QVERIFY(session.toggleSelection(Side::Player1, Coord(0, col)));
    session.fireVolley(Side::Player1);

    const int cover = static_cast<int>(ui::CellVisual::Kind::VolleyCover);
    for (int col = 0; col < 5; ++col) {
        // 命中、击沉、落空都盖 togeShoot.png，不显示舰型/红叉
        QCOMPARE(visualKind(defenderBoard, Coord(0, col)), cover);
    }
    // 单发命中的格子照旧显示舰型
    QCOMPARE(visualKind(defenderBoard, Coord(9, 9)),
             static_cast<int>(ui::CellVisual::Kind::Hit));

    // 重开一局后，遮盖标记随棋盘一起清掉
    view.startBattle();
    for (int col = 0; col < 5; ++col)
        QCOMPARE(visualKind(defenderBoard, Coord(0, col)),
                 static_cast<int>(ui::CellVisual::Kind::Empty));
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
