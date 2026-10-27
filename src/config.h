#pragma once

#include <stdint.h>

// ─────────────────────────────────────────────
// Wi-Fi access point
// ─────────────────────────────────────────────
constexpr const char *WIFI_SSID = "Drehfussball";   // Hotspot name
constexpr const char *WIFI_PASSWORD = "Kennwort1!"; // Hotspot password (min 8 chars)

// ─────────────────────────────────────────────
// Stepper (ball return)
// ─────────────────────────────────────────────
constexpr uint8_t STEPPER_DIR_PIN = 18;
constexpr uint8_t STEPPER_ENABLE_PIN = 19;
constexpr uint8_t STEPPER_STEP_PIN = 21;

// Distance the return mechanism travels to release the ball.
constexpr int32_t BALL_RETURN_STEPS = 1200;
constexpr uint32_t BALL_RETURN_HOLD_MS = 1000;

// ─────────────────────────────────────────────
// LED strips (one per team)
// ─────────────────────────────────────────────
constexpr uint8_t LED_PIN_TEAM1 = 16;
constexpr uint8_t LED_PIN_TEAM2 = 17;
constexpr uint8_t NUM_LEDS = 10;
constexpr uint32_t CELEBRATE_FRAME_MS = 40;

// ─────────────────────────────────────────────
// Reset button
// ─────────────────────────────────────────────
constexpr uint8_t RESET_BUTTON_PIN = 23;

// ─────────────────────────────────────────────
// IR light barriers (PCB v1.0)
// ─────────────────────────────────────────────
// One beam per chute:
//   Tor 1 (blue) -> J6 / S1 -> GPIO27
//   Tor 2 (red)  -> J8 / S3 -> GPIO32
//
// J7 / S2 and J9 / S4 are spare inputs and are not used by this firmware.
//
// The 74HC14 on the board INVERTS the receiver output:
//   beam free      -> GPIO LOW
//   beam blocked   -> GPIO HIGH
//
// The ball is in a closed chute, so a single beam is enough.  Every beam
// interruption (the ball entering the chute) counts as a goal.
constexpr uint8_t SENSOR1_PIN = 22;
constexpr uint8_t SENSOR2_PIN = 4;

// After a goal BOTH beams are locked out for one second: the GPIO interrupts of
// both sensors are disabled, so a single ball can never score twice (e.g. on the
// blue and the red side one after another).
constexpr uint32_t SENSOR_LOCKOUT_MS = 1000;

// Set to true to print every accepted goal to Serial.
constexpr bool DEBUG_SENSORS = true;
