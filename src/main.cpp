#include <Arduino.h>
#include <esp_system.h>

#include "config.h"
#include "game_state.h"
#include "leds.h"
#include "sensors.h"
#include "stepper.h"
#include "webserver.h"

namespace
{
// The ball return is moved in two steps one second apart.  Doing that with
// delay() blocked loop() (and therefore ledsTick(), the reset button and the
// web sockets) for a full second, so it is a small state machine instead.
enum class BallReturnState : uint8_t
{
  Idle,
  Holding
};

BallReturnState ballReturnState = BallReturnState::Idle;
uint32_t ballReturnCloseAtMs = 0;

bool resetButtonWasLow = false;
uint32_t resetButtonLastChangeMs = 0;

const char *resetReasonText(esp_reset_reason_t reason)
{
  switch (reason)
  {
  case ESP_RST_POWERON:
    return "power-on";
  case ESP_RST_EXT:
    return "external pin";
  case ESP_RST_SW:
    return "software";
  case ESP_RST_PANIC:
    return "panic / crash";
  case ESP_RST_INT_WDT:
    return "interrupt watchdog";
  case ESP_RST_TASK_WDT:
    return "task watchdog";
  case ESP_RST_WDT:
    return "other watchdog";
  case ESP_RST_DEEPSLEEP:
    return "deep sleep wakeup";
  case ESP_RST_BROWNOUT:
    return "BROWNOUT - check the 5 V supply";
  case ESP_RST_SDIO:
    return "SDIO";
  default:
    return "unknown";
  }
}

// True once per physical button press (debounced falling edge).  Without the
// edge detection a held button would restart the reset sequence over and over.
bool resetButtonPressed()
{
  const bool isLow = (digitalRead(RESET_BUTTON_PIN) == LOW);
  const uint32_t now = millis();

  if (isLow == resetButtonWasLow || (now - resetButtonLastChangeMs) < RESET_BUTTON_DEBOUNCE_MS)
  {
    return false;
  }

  resetButtonWasLow = isLow;
  resetButtonLastChangeMs = now;
  return isLow;
}

void startReset()
{
  // Open the ball return.
  openBallReturn();

  clearPendingGoals();
  resetScores();
  notifyClients();
  playStartupAnimation(); // non-blocking, drawn by ledsTick()
  clearResetRequest();
  Serial.println("Game reset!");

  ballReturnState = BallReturnState::Holding;
  ballReturnCloseAtMs = millis() + BALL_RETURN_HOLD_MS;
}

// Close the ball return once it has been open for BALL_RETURN_HOLD_MS.
void serviceBallReturn()
{
  if (ballReturnState != BallReturnState::Holding)
  {
    return;
  }

  if ((int32_t)(millis() - ballReturnCloseAtMs) < 0)
  {
    return;
  }

  // Drop goals that were detected while the ball was rolling back.
  clearPendingGoals();
  closeBallReturn();
  ballReturnState = BallReturnState::Idle;
}
} // namespace

void setup()
{
  delay(1000);
  Serial.begin(9600);
  Serial.printf("\nBoot: reset reason = %s\n",
                resetReasonText(esp_reset_reason()));

  setupStepper();
  setupWebServer();

  setupLeds();
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  resetButtonWasLow = (digitalRead(RESET_BUTTON_PIN) == LOW);
  setupSensors();

  playStartupAnimation();
}

void loop()
{
  cleanupWebSockets();

  // Draw whatever animation or score update is due.  Never blocking.
  ledsTick();

  // Ignore goals while the ball return is open: the ball is rolling back and
  // must not score.
  const bool scoreGoals = (ballReturnState == BallReturnState::Idle);

  if (scoreGoals && consumeGoal(TEAM_BLUE))
  {
    addGoal(TEAM_BLUE);
    Serial.print("Goal: BLUE | Score: ");
    Serial.print(getScore(TEAM_BLUE));
    Serial.print(" : ");
    Serial.println(getScore(TEAM_RED));
    notifyClients();
    playCelebration(TEAM_BLUE);
  }

  if (scoreGoals && consumeGoal(TEAM_RED))
  {
    addGoal(TEAM_RED);
    Serial.print("Goal: RED | Score: ");
    Serial.print(getScore(TEAM_BLUE));
    Serial.print(" : ");
    Serial.println(getScore(TEAM_RED));
    notifyClients();
    playCelebration(TEAM_RED);
  }

  const bool resetPressed = resetButtonPressed();

  if (ballReturnState == BallReturnState::Idle &&
      (resetPressed || isResetRequested()))
  {
    startReset();
  }

  serviceBallReturn();
}
