#include "core/GameSession.h"

#include <QtTest>

using namespace bs;

class GameSessionTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void missSwitchesTurn();
    void hitIncreasesBudgetAndKeepsTurn();
    void volleyChargesBudgetAndSwitchesTurnWhenNothingHit();
    void cannotSelectBeyondTarget();
    void victoryEndsBattle();
    void beginBattleClearsPreviousShots();

private:
    static void placeBattleship(Board& board)
    {
        board.place(ShipType::Battleship, Coord(0, 0), Direction::Right);
    }
};

void GameSessionTest::initTestCase()
{
    qRegisterMetaType<bs::Side>("bs::Side");
    qRegisterMetaType<bs::Coord>("bs::Coord");
}

void GameSessionTest::missSwitchesTurn()
{
    GameSession session;
    session.board(Side::Player1).place(ShipType::Submarine, Coord(9, 9), Direction::Down);
    session.beginBattle();

    QCOMPARE(session.currentSide(), Side::Player1);
    QCOMPARE(session.fireAt(Side::Player1, Coord(5, 5)), ShotResult::Miss);
    QCOMPARE(session.currentSide(), Side::Player2);

    // 不是自己回合时点击应当被忽略
    QCOMPARE(session.fireAt(Side::Player1, Coord(5, 6)), ShotResult::AlreadyResolved);
}

void GameSessionTest::hitIncreasesBudgetAndKeepsTurn()
{
    GameSession session;
    placeBattleship(session.board(Side::Player1));
    session.beginBattle();

    QCOMPARE(session.fireAt(Side::Player1, Coord(0, 0)), ShotResult::Hit);
    QCOMPARE(session.budget(Side::Player1), 0);
    QCOMPARE(session.currentSide(), Side::Player1);

    QCOMPARE(session.fireAt(Side::Player1, Coord(0, 1)), ShotResult::Hit);
    QCOMPARE(session.budget(Side::Player1), 1);
}

void GameSessionTest::volleyChargesBudgetAndSwitchesTurnWhenNothingHit()
{
    GameSession session;
    placeBattleship(session.board(Side::Player1));
    session.beginBattle();

    session.fireAt(Side::Player1, Coord(0, 0));
    session.fireAt(Side::Player1, Coord(0, 1));
    QCOMPARE(session.budget(Side::Player1), 1);

    QVERIFY(session.armVolley(Side::Player1, 1));
    QCOMPARE(session.volleyTarget(Side::Player1), 5);
    QCOMPARE(session.volleyCost(Side::Player1), 1);

    for (int col = 0; col < 5; ++col)
        QVERIFY(session.toggleSelection(Side::Player1, Coord(5, col)));
    QCOMPARE(session.volleyRemaining(Side::Player1), 0);

    const auto outcome = session.fireVolley(Side::Player1);
    QCOMPARE(outcome.cost, 1);
    QCOMPARE(outcome.hits, 0);
    QCOMPARE(session.budget(Side::Player1), -1);
    QCOMPARE(session.currentSide(), Side::Player2);
    QVERIFY(!session.isVolleyArmed(Side::Player1));
}

void GameSessionTest::cannotSelectBeyondTarget()
{
    GameSession session;
    placeBattleship(session.board(Side::Player1));
    session.beginBattle();
    session.fireAt(Side::Player1, Coord(0, 0));
    session.fireAt(Side::Player1, Coord(0, 1));

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

void GameSessionTest::victoryEndsBattle()
{
    GameSession session;
    session.board(Side::Player1).place(ShipType::Submarine, Coord(0, 0), Direction::Down);
    session.board(Side::Player2).place(ShipType::Submarine, Coord(9, 9), Direction::Down);
    session.beginBattle();

    QSignalSpy spy(&session, &GameSession::finished);
    QCOMPARE(session.fireAt(Side::Player1, Coord(0, 0)), ShotResult::Sunk);
    QCOMPARE(session.phase(), GameSession::Phase::Finished);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<bs::Side>(), Side::Player1);

    // 结束后不再接受操作
    QCOMPARE(session.fireAt(Side::Player2, Coord(9, 9)), ShotResult::AlreadyResolved);
}

void GameSessionTest::beginBattleClearsPreviousShots()
{
    GameSession session;
    placeBattleship(session.board(Side::Player1));
    session.beginBattle();
    session.fireAt(Side::Player1, Coord(0, 0));
    QVERIFY(session.board(Side::Player1).isResolved(Coord(0, 0)));

    session.beginBattle();
    QVERIFY(!session.board(Side::Player1).isResolved(Coord(0, 0)));
    QCOMPARE(session.currentSide(), Side::Player1);
    QCOMPARE(session.budget(Side::Player1), -1);
    // 舰船本身保留
    QVERIFY(session.board(Side::Player1).hasShipAt(Coord(0, 0)));
}

QTEST_GUILESS_MAIN(GameSessionTest)

#include "tst_gamesession.moc"
