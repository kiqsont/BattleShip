#include "core/Board.h"

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
    void allSunkOnlyAfterEveryCellHit();
    void remainingToPlaceFollowsFleetTable();
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
