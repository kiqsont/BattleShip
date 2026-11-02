#include "core/GameSession.h"

#include <QtTest>

using namespace bs;

class GameSessionTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void attacksOpponentBoard();
    void missSwitchesTurn();
    void hitDoesNotGrantVolleyCredit();
    void sinkingGrantsOneVolleyCredit();
    void volleyCoversFiveCellsPerCredit();
    void cannotSelectBeyondTarget();
    void victoryGoesToAttacker();
    void beginBattleClearsPreviousShots();

private:
    /// Player2 固定摆一艘战舰（(0,0) 起向右 4 格）+ 一艘潜水艇（(9,9)）。
    static void placeDefenderFleet(Board& board)
    {
        board.place(ShipType::Battleship, Coord(0, 0), Direction::Right);
        board.place(ShipType::Submarine, Coord(9, 9), Direction::Down);
    }
};

void GameSessionTest::initTestCase()
{
    qRegisterMetaType<bs::Side>("bs::Side");
    qRegisterMetaType<bs::Coord>("bs::Coord");
}

void GameSessionTest::attacksOpponentBoard()
{
    GameSession session;
    session.board(Side::Player1).place(ShipType::Battleship, Coord(0, 0), Direction::Right);
    placeDefenderFleet(session.board(Side::Player2));
    session.beginBattle();

    // Player1 打的是 Player2 的海域
    QCOMPARE(session.fireAt(Side::Player1, Coord(0, 0)), ShotResult::Hit);
    QVERIFY(session.board(Side::Player2).isResolved(Coord(0, 0)));
    QVERIFY(!session.board(Side::Player1).isResolved(Coord(0, 0)));
}

void GameSessionTest::missSwitchesTurn()
{
    GameSession session;
    placeDefenderFleet(session.board(Side::Player2));
    session.beginBattle();

    QCOMPARE(session.currentSide(), Side::Player1);
    QCOMPARE(session.fireAt(Side::Player1, Coord(5, 5)), ShotResult::Miss);
    QCOMPARE(session.currentSide(), Side::Player2);

    // 不是自己回合时点击应当被忽略
    QCOMPARE(session.fireAt(Side::Player1, Coord(5, 6)), ShotResult::AlreadyResolved);
}

void GameSessionTest::hitDoesNotGrantVolleyCredit()
{
    GameSession session;
    placeDefenderFleet(session.board(Side::Player2));
    session.beginBattle();

    // 命中但没击沉：不累积扫射次数，也不换手
    QCOMPARE(session.fireAt(Side::Player1, Coord(0, 0)), ShotResult::Hit);
    QCOMPARE(session.volleyCredits(Side::Player1), 0);
    QCOMPARE(session.currentSide(), Side::Player1);
}

void GameSessionTest::sinkingGrantsOneVolleyCredit()
{
    GameSession session;
    placeDefenderFleet(session.board(Side::Player2));
    session.beginBattle();

    QCOMPARE(session.fireAt(Side::Player1, Coord(9, 9)), ShotResult::Sunk);
    QCOMPARE(session.volleyCredits(Side::Player1), 1);
    QCOMPARE(session.currentSide(), Side::Player1);
}

void GameSessionTest::volleyCoversFiveCellsPerCredit()
{
    GameSession session;
    placeDefenderFleet(session.board(Side::Player2));
    session.beginBattle();

    session.fireAt(Side::Player1, Coord(9, 9)); // 击沉潜水艇，得一次扫射
    QCOMPARE(session.volleyCredits(Side::Player1), 1);

    QVERIFY(session.armVolley(Side::Player1, 1));
    QCOMPARE(session.volleyTarget(Side::Player1), 5); // 一次 = 5 格
    QCOMPARE(session.volleyCost(Side::Player1), 1);

    for (int col = 0; col < 5; ++col)
        QVERIFY(session.toggleSelection(Side::Player1, Coord(5, col)));
    QCOMPARE(session.volleyRemaining(Side::Player1), 0);

    const auto outcome = session.fireVolley(Side::Player1);
    QCOMPARE(outcome.cost, 1);
    QCOMPARE(outcome.hits, 0);
    QCOMPARE(session.volleyCredits(Side::Player1), 0);
    QCOMPARE(session.currentSide(), Side::Player2);
    QVERIFY(!session.isVolleyArmed(Side::Player1));
}

void GameSessionTest::cannotSelectBeyondTarget()
{
    GameSession session;
    placeDefenderFleet(session.board(Side::Player2));
    session.beginBattle();
    session.fireAt(Side::Player1, Coord(9, 9));

    QVERIFY(session.armVolley(Side::Player1, 1));
    for (int col = 0; col < 5; ++col)
        QVERIFY(session.toggleSelection(Side::Player1, Coord(5, col)));

    // 选满之后其余格子不可再选
    QVERIFY(!session.isSelectable(Side::Player1, Coord(6, 0)));
    QVERIFY(!session.toggleSelection(Side::Player1, Coord(6, 0)));

    // 取消一格后恢复可选
    QVERIFY(session.toggleSelection(Side::Player1, Coord(5, 0)));
    QVERIFY(session.isSelectable(Side::Player1, Coord(6, 0)));
}

void GameSessionTest::victoryGoesToAttacker()
{
    GameSession session;
    session.board(Side::Player1).place(ShipType::Battleship, Coord(0, 0), Direction::Right);
    session.board(Side::Player2).place(ShipType::Submarine, Coord(9, 9), Direction::Down);
    session.beginBattle();

    QSignalSpy spy(&session, &GameSession::finished);
    QCOMPARE(session.fireAt(Side::Player1, Coord(9, 9)), ShotResult::Sunk);
    QCOMPARE(session.phase(), GameSession::Phase::Finished);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<bs::Side>(), Side::Player1);

    // 结束后不再接受操作
    QCOMPARE(session.fireAt(Side::Player2, Coord(0, 0)), ShotResult::AlreadyResolved);
}

void GameSessionTest::beginBattleClearsPreviousShots()
{
    GameSession session;
    placeDefenderFleet(session.board(Side::Player2));
    session.beginBattle();
    session.fireAt(Side::Player1, Coord(0, 0));
    QVERIFY(session.board(Side::Player2).isResolved(Coord(0, 0)));

    session.beginBattle();
    QVERIFY(!session.board(Side::Player2).isResolved(Coord(0, 0)));
    QCOMPARE(session.currentSide(), Side::Player1);
    QCOMPARE(session.volleyCredits(Side::Player1), 0);
    // 舰船本身保留
    QVERIFY(session.board(Side::Player2).hasShipAt(Coord(0, 0)));
}

QTEST_GUILESS_MAIN(GameSessionTest)

#include "tst_gamesession.moc"
