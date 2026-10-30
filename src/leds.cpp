#include "leds.h"

#include <Arduino.h>
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <atomic>

#include "config.h"

namespace
{
CRGB goals1[NUM_LEDS] = {};
CRGB goals2[NUM_LEDS] = {};

// Serialises FastLED.show().  Only loop() renders now, but the mutex stays the
// single gate in front of FastLED so a call from another task would still be
// safe.
SemaphoreHandle_t timingMutex = nullptr;

// ─────────────────────────────────────────────
// Animations are state machines driven by ledsTick() from loop().  loop() must
// never block for hundreds of milliseconds: that froze the firmware with the
// LEDs stuck in their last frame and ended in a watchdog/panic reboot.
// ─────────────────────────────────────────────
enum class Animation : uint8_t
{
  None,
  Celebration,
  Startup
};

Animation animation = Animation::None;
Team animationTeam = TEAM_BLUE;
uint8_t animationFrame = 0;
uint32_t animationNextFrameMs = 0;

// Set by the web server task when the score changed; loop() redraws it.
std::atomic<bool> scoreDirty{false};

constexpr uint8_t CELEBRATION_FRAMES = 8;
constexpr uint8_t STARTUP_SWEEPS = 2;
constexpr uint8_t STARTUP_FRAMES_PER_SWEEP = 2 * NUM_LEDS;

CRGB *stripFor(Team team) { return (team == TEAM_BLUE) ? goals1 : goals2; }

CRGB teamColor(Team team) { return (team == TEAM_BLUE) ? CRGB::Blue : CRGB::Red; }

void showLeds()
{
  if (timingMutex == nullptr)
  {
    return;
  }

  // Bounded wait.  With portMAX_DELAY a mutex that was ever left locked would
  // hang the caller forever (the LEDs keep their last frame and the 5 s task
  // watchdog reboots the board).  Dropping a single frame is harmless instead.
  if (xSemaphoreTake(timingMutex, pdMS_TO_TICKS(LED_MUTEX_TIMEOUT_MS)) != pdTRUE)
  {
    Serial.println("WARN: LED mutex timeout, frame dropped");
    return;
  }

  FastLED.show();
  xSemaphoreGive(timingMutex);
}

void drawScore()
{
  const uint8_t score1 = getScore(TEAM_BLUE);
  const uint8_t score2 = getScore(TEAM_RED);

  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    goals1[NUM_LEDS - 1 - i] = (score1 > i) ? CRGB::Blue : CRGB::Black;
    goals2[i] = (score2 > i) ? CRGB::Red : CRGB::Black;
  }
}

// Leave the animation state behind and always end on the current score, so the
// strips can never stay stuck in a transient frame.
void finishAnimation()
{
  animation = Animation::None;
  scoreDirty.store(false); // the score is redrawn right below
  drawScore();
  showLeds();
}
} // namespace

void setupLeds()
{
  FastLED.addLeds<WS2812B, LED_PIN_TEAM1, GRB>(goals1, NUM_LEDS);
  FastLED.addLeds<WS2812B, LED_PIN_TEAM2, GRB>(goals2, NUM_LEDS);
  // Full white on ten LEDs alone is ~600 mA, three times the load of the blue
  // or red score display.  Together with the Wi-Fi TX peaks that browns out a
  // weak 5 V supply, which is why the strip must stay inside this budget.
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);

  timingMutex = xSemaphoreCreateMutex();
  showLeds();
}

void updateLedScores()
{
  drawScore();
  showLeds();
}

void requestLedUpdate() { scoreDirty.store(true); }

void playCelebration(Team team)
{
  animation = Animation::Celebration;
  animationTeam = team;
  animationFrame = 0;
  animationNextFrameMs = millis(); // first frame on the next ledsTick()
}

void playStartupAnimation()
{
  animation = Animation::Startup;
  animationFrame = 0;
  animationNextFrameMs = millis();
}

void ledsTick()
{
  if (animation == Animation::None)
  {
    if (scoreDirty.exchange(false))
    {
      updateLedScores();
    }
    return;
  }

  if ((int32_t)(millis() - animationNextFrameMs) < 0)
  {
    return; // this frame is not due yet
  }

  bool finished = false;

  if (animation == Animation::Celebration)
  {
    // Strobe in the colour of the scoring team.  No white: it would triple the
    // current of the strip for no visual gain.
    const CRGB color = (animationFrame % 2 == 0) ? teamColor(animationTeam) : CRGB::Black;
    CRGB *strip = stripFor(animationTeam);

    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
      strip[i] = color;
    }

    finished = (++animationFrame >= CELEBRATION_FRAMES);
  }
  else
  {
    // Two sweeps: fill the bars from LED 0 upwards, then clear them again.
    const uint8_t step = animationFrame % STARTUP_FRAMES_PER_SWEEP;
    const bool filling = step < NUM_LEDS;

    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
      const bool lit = filling ? (i <= step) : (i > (step - NUM_LEDS));
      goals1[i] = lit ? CRGB::Green : CRGB::Black;
      goals2[i] = lit ? CRGB::Green : CRGB::Black;
    }

    finished = (++animationFrame >= STARTUP_SWEEPS * STARTUP_FRAMES_PER_SWEEP);
  }

  showLeds();

  if (finished)
  {
    finishAnimation();
    return;
  }

  animationNextFrameMs =
      millis() + ((animation == Animation::Celebration) ? CELEBRATE_FRAME_MS : STARTUP_FRAME_MS);
}
