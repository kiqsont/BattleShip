#ifndef BS_CORE_GAMESESSION_H
#define BS_CORE_GAMESESSION_H

#include "core/Board.h"
#include "core/Types.h"

#include <QObject>

#include <array>
#include <vector>

namespace bs {

/// 对局状态机：谁在行动、还剩几次"一起扫射"、选中了哪些格子、胜负如何。
///
/// 规则依据（Battle Ship.docx）：
/// - "Player选择位置进攻" —— 每回合攻击的是**对方**海域；
/// - "每再击沉一艘舰，得一次一起扫射" —— 只有击沉才累积次数，命中不算；
/// - 一次"一起扫射"覆盖 5 格；被扫射的格子一律用 togeShoot.png 遮盖，
///   不显示命中/落空内容，也不暴露舰型（与初版一致，命中数只看行动条）。
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

    /// 用当前已布置的舰队开一局：清空命中状态、扫射次数与选中，回到 Player1 行动。
    void beginBattle();
    /// 离开对战（回主菜单），保留布置。
    void leaveBattle();

    /// 剩余"一起扫射"次数（本回合内有效，落空即清零）。
    int volleyCredits(Side side) const;
    bool isVolleyArmed(Side side) const;
    int volleyTarget(Side side) const;    ///< 本轮需要选中的总格数
    int volleyRemaining(Side side) const; ///< 还需要选中的格数
    int volleyCost(Side side) const;      ///< 发射将扣掉的次数
    int selectedCount(Side side) const;
    bool isSelected(Side side, Coord coord) const;
    bool isSelectable(Side side, Coord coord) const;
    std::vector<Coord> selectedCells(Side side) const;

    /// 单发：attacker 点击对方海域的一格。落空换手，击沉则累积一次扫射。
    ShotResult fireAt(Side attacker, Coord coord);

    /// 瞄准（一同扫射）：volleys 为要使用的扫射次数，内部按 kCellsPerVolley 换算格数。
    bool armVolley(Side attacker, int volleys);
    bool toggleSelection(Side attacker, Coord coord);
    void cancelVolley(Side attacker);

    struct VolleyOutcome {
        int hits = 0;
        int cost = 0;
    };
    VolleyOutcome fireVolley(Side attacker);

signals:
    void battleStarted();
    void turnChanged(bs::Side side);
    void cellChanged(bs::Side boardOwner, bs::Coord coord);
    void sideStateChanged(bs::Side side);
    void finished(bs::Side winner);

private:
    void resetRuntime(Side side);
    void resetAllRuntime();
    void switchTurn(Side to);
    bool checkVictory(Side attacker);

    std::array<Board, kSideCount> m_boards;
    std::array<int, kSideCount> m_volleyCredits;
    std::array<bool, kSideCount> m_armed;
    std::array<int, kSideCount> m_target;
    std::array<int, kSideCount> m_remaining;
    std::array<std::vector<Coord>, kSideCount> m_selected;

    Phase m_phase = Phase::Placement;
    Side m_current = Side::Player1;
};

} // namespace bs

#endif // BS_CORE_GAMESESSION_H
