#include <Arduino.h>

#include "config.h"
#include "game_state.h"
#include "leds.h"
#include "sensors.h"
#include "stepper.h"
#include "webserver.h"

void setup()
{
  delay(1000);
  Serial.begin(9600);

  setupStepper();
  setupWebServer();

  setupLeds();
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  setupSensors();

  playStartupAnimation();
}

void loop()
{
  cleanupWebSockets();

  if (consumeGoal(TEAM_BLUE))
  {
    addGoal(TEAM_BLUE);
    Serial.print("Goal: BLUE | Score: ");
    Serial.print(getScore(TEAM_BLUE));
    Serial.print(" : ");
    Serial.println(getScore(TEAM_RED));
    notifyClients();
    playCelebration(TEAM_BLUE);
  }

  if (consumeGoal(TEAM_RED))
  {
    addGoal(TEAM_RED);
    Serial.print("Goal: RED | Score: ");
    Serial.print(getScore(TEAM_BLUE));
    Serial.print(" : ");
    Serial.println(getScore(TEAM_RED));
    notifyClients();
    playCelebration(TEAM_RED);
  }

  if (digitalRead(RESET_BUTTON_PIN) == LOW || isResetRequested())
  {
    // Open the ball return.
    openBallReturn();

    clearPendingGoals();
    resetScores();
    notifyClients();
    playStartupAnimation();

    clearResetRequest();
    Serial.println("Game reset!");
    delay(BALL_RETURN_HOLD_MS); // Keep the return open for one second

    clearPendingGoals();
    closeBallReturn();
  }
}
