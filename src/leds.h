#pragma once

#include "game_state.h"

// Initialise the two LED strips.  Must be called before the animations below.
void setupLeds();

// Show the current score on both strips.
void updateLedScores();

// Flash the strip of the scoring team and return to the score display.
void playCelebration(Team team);

// Green sweep shown at startup and after a reset.
void playStartupAnimation();
