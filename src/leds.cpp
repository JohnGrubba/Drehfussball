#include "leds.h"

#include <Arduino.h>
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "config.h"

namespace
{
CRGB goals1[NUM_LEDS] = {};
CRGB goals2[NUM_LEDS] = {};

// Serialises FastLED.show() between loop() and the async web server task.
SemaphoreHandle_t timingMutex = nullptr;

// FastLED.show() is not thread-safe; take the mutex when it has been created.
void showLeds()
{
  if (timingMutex != nullptr && xSemaphoreTake(timingMutex, portMAX_DELAY))
  {
    FastLED.show();
    xSemaphoreGive(timingMutex);
  }
}

CRGB *stripFor(Team team)
{
  return (team == TEAM_BLUE) ? goals1 : goals2;
}
} // namespace

void setupLeds()
{
  FastLED.addLeds<WS2812B, LED_PIN_TEAM1, GRB>(goals1, NUM_LEDS);
  FastLED.addLeds<WS2812B, LED_PIN_TEAM2, GRB>(goals2, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.show();

  timingMutex = xSemaphoreCreateMutex();
}

void updateLedScores()
{
  const uint8_t score1 = getScore(TEAM_BLUE);
  const uint8_t score2 = getScore(TEAM_RED);

  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    goals1[NUM_LEDS - 1 - i] = (score1 > i) ? CRGB::Blue : CRGB::Black;
    goals2[i] = (score2 > i) ? CRGB::Red : CRGB::Black;
  }

  showLeds();
}

void playCelebration(Team team)
{
  CRGB *strip = stripFor(team);

  for (uint8_t flash = 0; flash < 8; flash++)
  {
    for (uint8_t i = 0; i < NUM_LEDS; i++)
      strip[i] = (flash % 2 == 0) ? CRGB::White : CRGB::Black;

    showLeds();
    delay(CELEBRATE_FRAME_MS);
  }

  updateLedScores();
}

void playStartupAnimation()
{
  for (uint8_t x = 0; x < 2; x++)
  {
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
      goals1[i] = CRGB::Green;
      goals2[i] = CRGB::Green;
      showLeds();
      delay(25);
    }

    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
      goals1[i] = CRGB::Black;
      goals2[i] = CRGB::Black;
      showLeds();
      delay(25);
    }
  }

  updateLedScores();
}
