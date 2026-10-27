#include "game_state.h"

#include <atomic>

namespace
{
std::atomic<uint8_t> scores[2] = {{0}, {0}};
std::atomic<bool> goalPending[2] = {{false}, {false}};
std::atomic<bool> resetPending{false};

std::atomic<uint8_t> &scoreRef(Team team)
{
  return scores[static_cast<uint8_t>(team)];
}

std::atomic<bool> &goalRef(Team team)
{
  return goalPending[static_cast<uint8_t>(team)];
}
} // namespace

uint8_t getScore(Team team)
{
  return scoreRef(team).load();
}

void addGoal(Team team)
{
  scoreRef(team).fetch_add(1);
}

void removeGoal(Team team)
{
  std::atomic<uint8_t> &score = scoreRef(team);

  uint8_t current = score.load();
  while (current > 0 && !score.compare_exchange_weak(current, current - 1))
  {
    // current was reloaded by compare_exchange_weak, retry.
  }
}

void resetScores()
{
  for (auto &score : scores)
    score.store(0);
}

void reportGoal(Team team)
{
  goalRef(team).store(true);
}

bool consumeGoal(Team team)
{
  return goalRef(team).exchange(false);
}

void clearPendingGoals()
{
  for (auto &goal : goalPending)
    goal.store(false);
}

void requestReset()
{
  resetPending.store(true);
}

bool isResetRequested()
{
  return resetPending.load();
}

void clearResetRequest()
{
  resetPending.store(false);
}
