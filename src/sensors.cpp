#include "sensors.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "config.h"
#include "game_state.h"

namespace
{
// ─────────────────────────────────────────────
// Sensor event queue
// ─────────────────────────────────────────────
struct BeamEvent
{
  uint8_t goal; // 0 = blue, 1 = red
  bool blocked;
};

QueueHandle_t sensorQueue = nullptr;

IRAM_ATTR void pushBeamEvent(uint8_t goal, uint8_t pin)
{
  BeamEvent ev;
  ev.goal = goal;
  ev.blocked = (gpio_get_level((gpio_num_t)pin) != 0); // 74HC14 inverts

  BaseType_t woken = pdFALSE;
  xQueueSendFromISR(sensorQueue, &ev, &woken);
  (void)woken;
  // The queue wakes sensorTask; an explicit portYIELD_FROM_ISR() is not
  // required here, the task runs on the next scheduler slot (<= 1 ms).
}

void IRAM_ATTR isrGoal1() { pushBeamEvent(0, SENSOR1_PIN); }
void IRAM_ATTR isrGoal2() { pushBeamEvent(1, SENSOR2_PIN); }

// Enable/disable the edge interrupts of BOTH beams.  While disabled no ISR can
// run and therefore no event can enter the queue.
void setBeamInterrupts(bool enable)
{
  if (enable)
  {
    gpio_intr_enable((gpio_num_t)SENSOR1_PIN);
    gpio_intr_enable((gpio_num_t)SENSOR2_PIN);
  }
  else
  {
    gpio_intr_disable((gpio_num_t)SENSOR1_PIN);
    gpio_intr_disable((gpio_num_t)SENSOR2_PIN);
  }
}

// Re-read the beam levels after the lockout, because edges that happened while
// the interrupts were disabled were never delivered.
void resyncBeams(bool blocked[2])
{
  const uint8_t pins[2] = {SENSOR1_PIN, SENSOR2_PIN};

  for (uint8_t g = 0; g < 2; g++)
  {
    // 74HC14 inverts: HIGH means the beam is blocked.
    blocked[g] = (gpio_get_level((gpio_num_t)pins[g]) != 0);
  }
}

void sensorTask(void *)
{
  BeamEvent ev;
  bool blocked[2] = {false, false};

  for (;;)
  {
    if (xQueueReceive(sensorQueue, &ev, portMAX_DELAY) != pdTRUE)
    {
      continue;
    }

    const uint8_t g = ev.goal;

    if (!ev.blocked)
    {
      // Trailing edge, the ball left the beam.  Nothing to score.
      blocked[g] = false;
      continue;
    }

    if (blocked[g])
    {
      // Duplicate of a beam that was already blocked when the lockout ended.
      continue;
    }

    blocked[g] = true;

    // Any beam interruption counts as a goal.
    reportGoal((Team)g);

    if (DEBUG_SENSORS)
    {
      Serial.printf("S%u GOAL -> lockout %u ms\n", g + 1,
                    static_cast<unsigned>(SENSOR_LOCKOUT_MS));
    }

    // Lock out BOTH sensors for one second.  The other side cannot fire while
    // the ball is still rolling through, and the same ball cannot be counted
    // twice.
    setBeamInterrupts(false);
    vTaskDelay(pdMS_TO_TICKS(SENSOR_LOCKOUT_MS));
    // Drop anything an in-flight ISR may have queued before the lockout.
    xQueueReset(sensorQueue);
    resyncBeams(blocked);
    setBeamInterrupts(true);
  }
}
} // namespace

void setupSensors()
{
  sensorQueue = xQueueCreate(32, sizeof(BeamEvent));
  configASSERT(sensorQueue != nullptr);

  // The 74HC14 output is push-pull; no internal pull-up is needed.
  pinMode(SENSOR1_PIN, INPUT);
  pinMode(SENSOR2_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(SENSOR1_PIN), isrGoal1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SENSOR2_PIN), isrGoal2, CHANGE);

  xTaskCreatePinnedToCore(
      sensorTask, // Function
      "Sensors",  // Name
      4096,       // Stack size in words
      NULL,       // Parameter
      6,          // Priority
      NULL,       // Handle
      1           // Core 1
  );
}
