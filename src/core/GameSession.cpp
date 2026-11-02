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
    m_budget[i] = -1;
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

int GameSession::budget(Side side) const
{
    return m_budget[static_cast<size_t>(index(side))];
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
    return m_target[static_cast<size_t>(index(side))] / rules::kCellsPerVolleyUnit;
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

bool GameSession::isSelectable(Side side, Coord coord) const
{
    const size_t i = static_cast<size_t>(index(side));
    if (m_phase != Phase::Battle || side != m_current || !m_armed[i])
        return false;
    if (!coord.isValid() || m_boards[i].isResolved(coord))
        return false;
    if (isSelected(side, coord))
        return true;
    // 目标格数选满后，其余格子不可再选（旧版靠 CellButton::full 标记实现）
    return m_target[i] <= 0 || m_remaining[i] > 0;
}

ShotResult GameSession::fireAt(Side side, Coord coord)
{
    if (m_phase != Phase::Battle || side != m_current || !coord.isValid())
        return ShotResult::AlreadyResolved;

    Board& target = m_boards[static_cast<size_t>(index(side))];
    if (target.isResolved(coord))
        return ShotResult::AlreadyResolved;

    const ShotResult result = target.shoot(coord);
    emit cellChanged(side, coord);

    const size_t i = static_cast<size_t>(index(side));
    if (result == ShotResult::Miss) {
        m_budget[i] = -1;
        switchTurn(otherSide(side));
    } else {
        ++m_budget[i];
    }

    emit sideStateChanged(side);

    // 与旧版一致：只有命中才判定胜负（落空不可能击沉最后一艘，仅为保持语义一致）
    if (result != ShotResult::Miss)
        checkVictory(side);
    return result;
}

bool GameSession::armVolley(Side side, int units)
{
    const size_t i = static_cast<size_t>(index(side));
    if (m_phase != Phase::Battle || side != m_current)
        return false;
    if (m_budget[i] <= 0)
        return false;

    int target = units > 0 ? units * rules::kCellsPerVolleyUnit : 0;
    const int maxCells = m_budget[i] * rules::kCellsPerVolleyUnit;
    if (target > maxCells)
        target = maxCells;

    m_armed[i] = true;
    m_target[i] = target;
    m_remaining[i] = target;
    m_selected[i].clear();

    emit sideStateChanged(side);
    return true;
}

bool GameSession::toggleSelection(Side side, Coord coord)
{
    const size_t i = static_cast<size_t>(index(side));
    if (m_phase != Phase::Battle || side != m_current || !m_armed[i])
        return false;
    if (!coord.isValid() || m_boards[i].isResolved(coord))
        return false;

    std::vector<Coord>& selected = m_selected[i];
    const auto it = std::find(selected.begin(), selected.end(), coord);
    if (it != selected.end()) {
        selected.erase(it);
        ++m_remaining[i];
    } else {
        if (!isSelectable(side, coord))
            return false;
        selected.push_back(coord);
        --m_remaining[i];
    }

    emit cellChanged(side, coord);
    emit sideStateChanged(side);
    return true;
}

void GameSession::cancelVolley(Side side)
{
    const size_t i = static_cast<size_t>(index(side));
    if (!m_armed[i])
        return;

    m_armed[i] = false;
    m_target[i] = 0;
    m_remaining[i] = 0;
    m_selected[i].clear();

    emit sideStateChanged(side);
}

GameSession::VolleyOutcome GameSession::fireVolley(Side side)
{
    VolleyOutcome outcome;
    const size_t i = static_cast<size_t>(index(side));
    if (m_phase != Phase::Battle || side != m_current || !m_armed[i])
        return outcome;

    outcome.cost = volleyCost(side);
    m_budget[i] -= outcome.cost;

    Board& target = m_boards[i];
    const std::vector<Coord> selected = m_selected[i];
    for (const Coord& coord : selected) {
        if (target.shoot(coord) != ShotResult::Miss)
            ++outcome.hits;
        emit cellChanged(side, coord);
    }

    m_armed[i] = false;
    m_target[i] = 0;
    m_remaining[i] = 0;
    m_selected[i].clear();
    emit sideStateChanged(side);

    if (outcome.hits == 0) {
        m_budget[i] = -1;
        switchTurn(otherSide(side));
    }

    checkVictory(side);
    return outcome;
}

void GameSession::switchTurn(Side to)
{
    m_current = to;
    emit turnChanged(m_current);
}

bool GameSession::checkVictory(Side side)
{
    if (m_phase != Phase::Battle)
        return false;
    if (!m_boards[static_cast<size_t>(index(side))].allSunk())
        return false;

    m_phase = Phase::Finished;
    emit finished(side);
    return true;
}

} // namespace bs
