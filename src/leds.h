#pragma once

#include "game_state.h"

// Initialise the two LED strips.  Must be called before the animations below.
void setupLeds();

// Draw the current score on both strips once (blocking, single frame).
void updateLedScores();

// Ask loop() to redraw the score.  Safe to call from any task, e.g. from the
// web server, because only loop() ever talks to FastLED.
void requestLedUpdate();

// Advance the running animation.  Must be called once per loop() iteration.
void ledsTick();

// Start the celebration animation for the scoring team.  Returns immediately;
// the frames are drawn by ledsTick().
void playCelebration(Team team);

// Start the green sweep shown at startup and after a reset.  Returns
// immediately; the frames are drawn by ledsTick().
void playStartupAnimation();
