#pragma once

#include <stdint.h>

// The two sides of the table.  The numeric value doubles as the sensor index.
enum Team : uint8_t
{
  TEAM_BLUE = 0,
  TEAM_RED = 1
};

// ─────────────────────────────────────────────
// Score
// ─────────────────────────────────────────────
uint8_t getScore(Team team);
void addGoal(Team team);
void removeGoal(Team team); // never goes below zero
void resetScores();

// ─────────────────────────────────────────────
// Events between tasks
// ─────────────────────────────────────────────

// Goal detected by the IR sensor task, consumed once by loop().
void reportGoal(Team team);
bool consumeGoal(Team team);
void clearPendingGoals();

// Reset requested by the web UI or the reset button.
void requestReset();
bool isResetRequested();
void clearResetRequest();
