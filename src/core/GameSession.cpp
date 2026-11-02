#include "core/GameSession.h"

#include <algorithm>

namespace bs {

GameSession::GameSession(QObject* parent)
    : QObject(parent)
{
    resetAllRuntime();
}

Board& GameSession::board(Side side)
{
    return m_boards[static_cast<size_t>(index(side))];
}

const Board& GameSession::board(Side side) const
{
    return m_boards[static_cast<size_t>(index(side))];
}

void GameSession::resetRuntime(Side side)
{
    const size_t i = static_cast<size_t>(index(side));
    m_volleyCredits[i] = 0;
    m_armed[i] = false;
    m_target[i] = 0;
    m_remaining[i] = 0;
    m_selected[i].clear();
}

void GameSession::resetAllRuntime()
{
    for (int i = 0; i < kSideCount; ++i)
        resetRuntime(sideOf(i));
}

void GameSession::beginBattle()
{
    for (int i = 0; i < kSideCount; ++i) {
        m_boards[static_cast<size_t>(i)].clearResolutions();
        resetRuntime(sideOf(i));
    }
    m_current = Side::Player1;
    m_phase = Phase::Battle;

    emit battleStarted();
    emit turnChanged(m_current);
    for (int i = 0; i < kSideCount; ++i)
        emit sideStateChanged(sideOf(i));
}

void GameSession::leaveBattle()
{
    resetAllRuntime();
    m_phase = Phase::Placement;
}

int GameSession::volleyCredits(Side side) const
{
    return m_volleyCredits[static_cast<size_t>(index(side))];
}

bool GameSession::isVolleyArmed(Side side) const
{
    return m_armed[static_cast<size_t>(index(side))];
}

int GameSession::volleyTarget(Side side) const
{
    return m_target[static_cast<size_t>(index(side))];
}

int GameSession::volleyRemaining(Side side) const
{
    return m_remaining[static_cast<size_t>(index(side))];
}

int GameSession::volleyCost(Side side) const
{
    return m_target[static_cast<size_t>(index(side))] / rules::kCellsPerVolley;
}

int GameSession::selectedCount(Side side) const
{
    return static_cast<int>(m_selected[static_cast<size_t>(index(side))].size());
}

std::vector<Coord> GameSession::selectedCells(Side side) const
{
    return m_selected[static_cast<size_t>(index(side))];
}

bool GameSession::isSelected(Side side, Coord coord) const
{
    const std::vector<Coord>& selected = m_selected[static_cast<size_t>(index(side))];
    return std::find(selected.begin(), selected.end(), coord) != selected.end();
}

bool GameSession::isSelectable(Side attacker, Coord coord) const
{
    const size_t i = static_cast<size_t>(index(attacker));
    if (m_phase != Phase::Battle || attacker != m_current || !m_armed[i])
        return false;

    // 选中的是对方海域的格子
    const Board& target = m_boards[static_cast<size_t>(index(otherSide(attacker)))];
    if (!coord.isValid() || target.isResolved(coord))
        return false;
    if (isSelected(attacker, coord))
        return true;
    // 目标格数选满后，其余格子不可再选
    return m_target[i] <= 0 || m_remaining[i] > 0;
}

ShotResult GameSession::fireAt(Side attacker, Coord coord)
{
    if (m_phase != Phase::Battle || attacker != m_current || !coord.isValid())
        return ShotResult::AlreadyResolved;

    const Side defender = otherSide(attacker);
    Board& target = m_boards[static_cast<size_t>(index(defender))];
    if (target.isResolved(coord))
        return ShotResult::AlreadyResolved;

    const ShotResult result = target.shoot(coord);
    emit cellChanged(defender, coord);

    const size_t i = static_cast<size_t>(index(attacker));
    if (result == ShotResult::Miss) {
        // 落空换手，本回合累积的扫射次数作废
        m_volleyCredits[i] = 0;
        switchTurn(defender);
    } else if (result == ShotResult::Sunk) {
        // "每再击沉一艘舰，得一次一起扫射"
        ++m_volleyCredits[i];
    }

    emit sideStateChanged(attacker);

    if (result != ShotResult::Miss)
        checkVictory(attacker);
    return result;
}

bool GameSession::armVolley(Side attacker, int volleys)
{
    const size_t i = static_cast<size_t>(index(attacker));
    if (m_phase != Phase::Battle || attacker != m_current)
        return false;
    if (m_volleyCredits[i] <= 0 || volleys <= 0)
        return false;

    int target = volleys * rules::kCellsPerVolley;
    const int maxCells = m_volleyCredits[i] * rules::kCellsPerVolley;
    if (target > maxCells)
        target = maxCells;

    m_armed[i] = true;
    m_target[i] = target;
    m_remaining[i] = target;
    m_selected[i].clear();

    emit sideStateChanged(attacker);
    return true;
}

bool GameSession::toggleSelection(Side attacker, Coord coord)
{
    const size_t i = static_cast<size_t>(index(attacker));
    if (m_phase != Phase::Battle || attacker != m_current || !m_armed[i])
        return false;

    const Board& target = m_boards[static_cast<size_t>(index(otherSide(attacker)))];
    if (!coord.isValid() || target.isResolved(coord))
        return false;

    std::vector<Coord>& selected = m_selected[i];
    const auto it = std::find(selected.begin(), selected.end(), coord);
    if (it != selected.end()) {
        selected.erase(it);
        ++m_remaining[i];
    } else {
        if (!isSelectable(attacker, coord))
            return false;
        selected.push_back(coord);
        --m_remaining[i];
    }

    emit cellChanged(otherSide(attacker), coord);
    emit sideStateChanged(attacker);
    return true;
}

void GameSession::cancelVolley(Side attacker)
{
    const size_t i = static_cast<size_t>(index(attacker));
    if (!m_armed[i])
        return;

    m_armed[i] = false;
    m_target[i] = 0;
    m_remaining[i] = 0;
    m_selected[i].clear();

    emit sideStateChanged(attacker);
}

GameSession::VolleyOutcome GameSession::fireVolley(Side attacker)
{
    VolleyOutcome outcome;
    const size_t i = static_cast<size_t>(index(attacker));
    if (m_phase != Phase::Battle || attacker != m_current || !m_armed[i])
        return outcome;

    const Side defender = otherSide(attacker);
    outcome.cost = volleyCost(attacker);
    m_volleyCredits[i] -= outcome.cost;

    Board& target = m_boards[static_cast<size_t>(index(defender))];
    const std::vector<Coord> selected = m_selected[i];
    for (const Coord& coord : selected) {
        if (target.shoot(coord) != ShotResult::Miss)
            ++outcome.hits;
        emit cellChanged(defender, coord);
    }

    m_armed[i] = false;
    m_target[i] = 0;
    m_remaining[i] = 0;
    m_selected[i].clear();
    emit sideStateChanged(attacker);

    if (outcome.hits == 0) {
        m_volleyCredits[i] = 0;
        switchTurn(defender);
    }

    checkVictory(attacker);
    return outcome;
}

void GameSession::switchTurn(Side to)
{
    m_current = to;
    emit turnChanged(m_current);
}

bool GameSession::checkVictory(Side attacker)
{
    if (m_phase != Phase::Battle)
        return false;

    // 打光对方全部舰船即获胜
    if (!m_boards[static_cast<size_t>(index(otherSide(attacker)))].allSunk())
        return false;

    m_phase = Phase::Finished;
    emit finished(attacker);
    return true;
}

} // namespace bs
