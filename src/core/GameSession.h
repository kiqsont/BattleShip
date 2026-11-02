#ifndef BS_CORE_GAMESESSION_H
#define BS_CORE_GAMESESSION_H

#include "core/Board.h"
#include "core/Types.h"

#include <QObject>

#include <array>
#include <vector>

namespace bs {

/// 对局状态机：谁在行动、还剩多少扫射预算、选中了哪些格子、胜负如何。
/// 旧版把这些状态散成 turn / togeShoot1 / togeShoot2 / choic1 / choic2 /
/// Shoot1 / Shoot2 / time1 / time2 九个字段外加两个"只用来发信号"的 label。
class GameSession : public QObject
{
    Q_OBJECT

public:
    enum class Phase { Placement, Battle, Finished };

    explicit GameSession(QObject* parent = nullptr);

    Board& board(Side side);
    const Board& board(Side side) const;

    Phase phase() const { return m_phase; }
    Side currentSide() const { return m_current; }

    /// 用当前已布置的舰队开一局：清空命中状态、预算与选中，回到 Player1 行动。
    void beginBattle();
    /// 离开对战（回主菜单），保留布置。
    void leaveBattle();

    int budget(Side side) const;             ///< 剩余扫射预算，-1 表示本回合未累积
    bool isVolleyArmed(Side side) const;
    int volleyTarget(Side side) const;       ///< 本轮需要选中的格数
    int volleyRemaining(Side side) const;    ///< 还需要选中的格数
    int volleyCost(Side side) const;         ///< 发射将扣掉的预算
    int selectedCount(Side side) const;
    bool isSelected(Side side, Coord coord) const;
    bool isSelectable(Side side, Coord coord) const;
    std::vector<Coord> selectedCells(Side side) const;

    /// 单发：点自己海域的一格。命中则预算 +1 并继续行动，落空则换手。
    ShotResult fireAt(Side side, Coord coord);

    /// 瞄准（一同扫射）：units 为界面输入的"格数单位"，内部按 kCellsPerVolleyUnit 换算。
    bool armVolley(Side side, int units);
    bool toggleSelection(Side side, Coord coord);
    void cancelVolley(Side side);

    struct VolleyOutcome {
        int hits = 0;
        int cost = 0;
    };
    VolleyOutcome fireVolley(Side side);

signals:
    void battleStarted();
    void turnChanged(bs::Side side);
    void cellChanged(bs::Side side, bs::Coord coord);
    void sideStateChanged(bs::Side side);
    void finished(bs::Side winner);

private:
    void resetRuntime(Side side);
    void resetAllRuntime();
    void switchTurn(Side to);
    bool checkVictory(Side side);

    std::array<Board, kSideCount> m_boards;
    std::array<int, kSideCount> m_budget;
    std::array<bool, kSideCount> m_armed;
    std::array<int, kSideCount> m_target;
    std::array<int, kSideCount> m_remaining;
    std::array<std::vector<Coord>, kSideCount> m_selected;

    Phase m_phase = Phase::Placement;
    Side m_current = Side::Player1;
};

} // namespace bs

#endif // BS_CORE_GAMESESSION_H
