#include "core/Board.h"
#include "core/Rules.h"

#include <array>

#include <QtTest>

using namespace bs;

class BoardTest : public QObject
{
    Q_OBJECT

private slots:
    void placesShipAndExposesCells();
    void rejectsOutOfBoundsPlacement();
    void rejectsOverlappingPlacement();
    void removesWholeShipWhenClickingBodyCell();
    void shootReportsMissHitSunk();
    void volleyCellsAreMarkedAndCleared();
    void allSunkOnlyAfterEveryCellHit();
    void remainingToPlaceFollowsFleetTable();
    void randomizeFleetPlacesWholeValidFleet();
    void placingTheSameShipTwiceKeepsOccupancyConsistent();
};

void BoardTest::placesShipAndExposesCells()
{
    Board board;
    QCOMPARE(board.place(ShipType::Battleship, Coord(1, 1), Direction::Down), true);

    for (int row = 1; row <= 4; ++row)
        QVERIFY(board.hasShipAt(Coord(row, 1)));

    QVERIFY(!board.hasShipAt(Coord(5, 1)));

    const CellView head = board.view(Coord(1, 1));
    QVERIFY(head.occupied);
    QVERIFY(head.isHead);
    QCOMPARE(head.type, ShipType::Battleship);
    QCOMPARE(head.direction, Direction::Down);

    const CellView body = board.view(Coord(2, 1));
    QVERIFY(body.occupied);
    QVERIFY(!body.isHead);
}

void BoardTest::rejectsOutOfBoundsPlacement()
{
    Board board;
    // 4 格舰从第 8 行往下放会超出边界
    QCOMPARE(board.canPlace(ShipType::Battleship, Coord(8, 0), Direction::Down),
             PlacementError::OutOfBounds);
    QCOMPARE(board.place(ShipType::Battleship, Coord(8, 0), Direction::Down), false);
    QCOMPARE(board.placedCount(ShipType::Battleship), 0);

    // 列号越界同样拒绝（旧版没有校验，会写到数组外面）
    QCOMPARE(board.canPlace(ShipType::Submarine, Coord(0, 10), Direction::Down),
             PlacementError::OutOfBounds);
}

void BoardTest::rejectsOverlappingPlacement()
{
    Board board;
    QVERIFY(board.place(ShipType::Cruiser, Coord(0, 0), Direction::Down));

    QCOMPARE(board.canPlace(ShipType::Destroyer, Coord(1, 0), Direction::Right),
             PlacementError::Overlap);
    QCOMPARE(board.place(ShipType::Destroyer, Coord(1, 0), Direction::Right), false);
    QCOMPARE(board.placedCount(ShipType::Destroyer), 0);

    // 校验失败不能留下半截船体
    QVERIFY(!board.hasShipAt(Coord(0, 1)));
}

void BoardTest::removesWholeShipWhenClickingBodyCell()
{
    Board board;
    QVERIFY(board.place(ShipType::Cruiser, Coord(0, 0), Direction::Down));

    // 点在船体（不是船头）上也要整艘删掉
    QVERIFY(board.removeShipAt(Coord(2, 0)));
    QCOMPARE(board.placedCount(ShipType::Cruiser), 0);
    QVERIFY(!board.hasShipAt(Coord(0, 0)));
    QVERIFY(!board.hasShipAt(Coord(1, 0)));
    QVERIFY(!board.hasShipAt(Coord(2, 0)));
    QVERIFY(!board.removeShipAt(Coord(2, 0)));
}

void BoardTest::shootReportsMissHitSunk()
{
    Board board;
    QVERIFY(board.place(ShipType::Destroyer, Coord(0, 0), Direction::Right));

    QCOMPARE(board.shoot(Coord(0, 0)), ShotResult::Hit);
    QCOMPARE(board.shoot(Coord(0, 0)), ShotResult::AlreadyResolved);
    QCOMPARE(board.shoot(Coord(0, 1)), ShotResult::Sunk);
    QCOMPARE(board.shoot(Coord(5, 5)), ShotResult::Miss);
    QCOMPARE(board.shoot(Coord(5, 5)), ShotResult::AlreadyResolved);
}

void BoardTest::volleyCellsAreMarkedAndCleared()
{
    Board board;
    QVERIFY(board.place(ShipType::Destroyer, Coord(0, 0), Direction::Right));

    // 单发落空：不遮盖
    QCOMPARE(board.shoot(Coord(4, 4)), ShotResult::Miss);
    QVERIFY(board.view(Coord(4, 4)).resolved);
    QVERIFY(!board.view(Coord(4, 4)).volley);

    // 齐射的命中、击沉、落空三种结果都打上同一个"遮盖"标记
    QCOMPARE(board.shootVolley(Coord(0, 0)), ShotResult::Hit);
    QCOMPARE(board.shootVolley(Coord(0, 1)), ShotResult::Sunk);
    QCOMPARE(board.shootVolley(Coord(5, 5)), ShotResult::Miss);
    QVERIFY(board.view(Coord(0, 0)).volley);
    QVERIFY(board.view(Coord(0, 1)).volley);
    QVERIFY(board.view(Coord(5, 5)).volley);
    QVERIFY(board.view(Coord(0, 1)).sunk); // 状态照常记录，只是界面不再据此显示舰型

    // 已揭开的格子不会被重复标记成齐射格
    QCOMPARE(board.shootVolley(Coord(4, 4)), ShotResult::AlreadyResolved);
    QVERIFY(!board.view(Coord(4, 4)).volley);

    // 重新布阵时也把旧标记清掉
    QVERIFY(board.place(ShipType::Submarine, Coord(2, 2), Direction::Down));
    QCOMPARE(board.shootVolley(Coord(6, 6)), ShotResult::Miss);
    QVERIFY(board.view(Coord(6, 6)).volley);
    board.clear();
    QVERIFY(!board.view(Coord(6, 6)).volley);
    QVERIFY(!board.view(Coord(0, 0)).volley);

    // 只重置战斗结果，舰队保留
    QVERIFY(board.place(ShipType::Destroyer, Coord(0, 0), Direction::Right));
    QCOMPARE(board.shootVolley(Coord(0, 0)), ShotResult::Hit);
    QVERIFY(board.view(Coord(0, 0)).volley);
    board.clearResolutions();
    QVERIFY(!board.view(Coord(0, 0)).volley);
    QVERIFY(board.hasShipAt(Coord(0, 0)));
}

void BoardTest::allSunkOnlyAfterEveryCellHit()
{
    Board board;
    QVERIFY(board.place(ShipType::Cruiser, Coord(0, 0), Direction::Down));
    QVERIFY(!board.allSunk());

    board.shoot(Coord(0, 0));
    board.shoot(Coord(1, 0));
    QVERIFY(!board.allSunk());

    board.shoot(Coord(2, 0));
    QVERIFY(board.allSunk());
    QCOMPARE(board.sunkCount(), 1);
}

void BoardTest::remainingToPlaceFollowsFleetTable()
{
    Board board;
    QCOMPARE(board.remainingToPlace(ShipType::Battleship), 1);
    QCOMPARE(board.remainingToPlace(ShipType::Cruiser), 2);
    QCOMPARE(board.remainingToPlace(ShipType::Destroyer), 4);
    QCOMPARE(board.remainingToPlace(ShipType::Submarine), 2);

    QVERIFY(board.place(ShipType::Cruiser, Coord(0, 0), Direction::Down));
    QCOMPARE(board.remainingToPlace(ShipType::Cruiser), 1);
}

void BoardTest::randomizeFleetPlacesWholeValidFleet()
{
    Board board;
    QVERIFY(board.place(ShipType::Battleship, Coord(0, 0), Direction::Right));

    int expectedCells = 0;
    for (const rules::FleetEntry& entry : rules::kFleet)
        expectedCells += entry.count * lengthOf(entry.type);

    // 反复随机：每一轮都必须是满编、不越界、不重叠，且旧布置被清掉
    for (int round = 0; round < 20; ++round) {
        QVERIFY(board.randomizeFleet());

        const std::vector<Ship>& ships = board.ships();
        QCOMPARE(static_cast<int>(ships.size()), rules::totalShipCount());
        for (const rules::FleetEntry& entry : rules::kFleet)
            QCOMPARE(board.placedCount(entry.type), entry.count);

        std::array<bool, kCellCount> seen{};
        seen.fill(false);
        int occupied = 0;
        for (const Ship& ship : ships) {
            const std::vector<Coord> cells = ship.cells();
            QCOMPARE(static_cast<int>(cells.size()), ship.length());
            for (const Coord& cell : cells) {
                QVERIFY(cell.isValid());
                const size_t flat = static_cast<size_t>(cell.flatIndex());
                QVERIFY2(!seen[flat], "随机布阵不能有重叠格");
                seen[flat] = true;
                ++occupied;
                QVERIFY(board.hasShipAt(cell));
                QCOMPARE(board.view(cell).type, ship.type());
                QVERIFY(!board.view(cell).resolved); // 布置阶段不该带战斗痕迹
            }
        }
        QCOMPARE(occupied, expectedCells);
    }
}

void BoardTest::placingTheSameShipTwiceKeepsOccupancyConsistent()
{
    Board board;
    QVERIFY(board.place(ShipType::Destroyer, Coord(0, 0), Direction::Down));
    QVERIFY(board.removeShipAt(Coord(0, 0)));
    QVERIFY(board.place(ShipType::Destroyer, Coord(0, 0), Direction::Down));
    QVERIFY(board.hasShipAt(Coord(1, 0)));
    QCOMPARE(board.placedCount(ShipType::Destroyer), 1);
}

QTEST_APPLESS_MAIN(BoardTest)

#include "tst_board.moc"
