#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FastAccelStepper.h>
#include <FastLED.h>
#include <RunningMedian.h>
#include <WiFi.h>

#include "site.cpp"

// --- Wi-Fi & Web Server Settings ---
const char *ssid = "Drehfussball";   // Hotspot Name
const char *password = "Kennwort1!"; // Hotspot Password (min 8 chars)

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

#define dirPinStepper 21
#define enablePinStepper 17
#define stepPinStepper 16

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;

#define NUM_LEDS 10
#define RESET_BUTTON_PIN 33

// --- Calibration ---
#define ACTIVATION_DISTANCE_FROM_AVERAGE 0.3 // cm below idle = "object present" (increased to filter out noise)

// After a goal is registered, ignore that sensor for this many ms.
#define GOAL_COOLDOWN_MS 2000

// During the cooldown the celebration animation runs.
#define CELEBRATE_FRAME_MS 80

CRGB goals1[NUM_LEDS] = {};
CRGB goals2[NUM_LEDS] = {};

#define ECHO2_PIN 13
#define TRIG2_PIN 12
#define ECHO1_PIN 14
#define TRIG1_PIN 27

volatile uint8_t team1 = 0;
volatile uint8_t team2 = 0;

volatile bool resetRequested = false;

float idleDistance1 = 3.3;
float idleDistance2 = 3.3;

TaskHandle_t SensorTask;

volatile unsigned long cooldownUntil1 = 0;
volatile unsigned long cooldownUntil2 = 0;

volatile bool goal1Scored = false;
volatile bool goal2Scored = false;

SemaphoreHandle_t timingMutex;

// ─────────────────────────────────────────────
// Helpers & WebSockets
// ─────────────────────────────────────────────

void notifyClients()
{
  String json =
      "{\"team1\":" + String(team1) + ",\"team2\":" + String(team2) + "}";
  ws.textAll(json);
}

void updateLEDSGoals()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    goals1[9 - i] = (team1 > i) ? CRGB::Blue : CRGB::Black;
    goals2[i] = (team2 > i) ? CRGB::Red : CRGB::Black;
  }

  if (xSemaphoreTake(timingMutex, portMAX_DELAY))
  {
    FastLED.show();
    xSemaphoreGive(timingMutex);
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len &&
      info->opcode == WS_TEXT)
  {
    data[len] = 0;
    String message = (char *)data;

    if (message == "t1_add")
      team1++;
    if (message == "t1_sub" && team1 > 0)
      team1--;
    if (message == "t2_add")
      team2++;
    if (message == "t2_sub" && team2 > 0)
      team2--;

    if (message == "reset")
    {
      resetRequested = true;
    }
    else
    {
      updateLEDSGoals();
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    // Send current score immediately upon connection
    client->text("{\"team1\":" + String(team1) + ",\"team2\":" + String(team2) +
                 "}");
  }
  else if (type == WS_EVT_DATA)
  {
    handleWebSocketMessage(arg, data, len);
  }
}

void celebrateGoal(CRGB *strip)
{
  for (int flash = 0; flash < 4; flash++)
  {
    for (int i = 0; i < NUM_LEDS; i++)
      strip[i] = (flash % 2 == 0) ? CRGB::White : CRGB::Black;

    if (timingMutex != NULL && xSemaphoreTake(timingMutex, portMAX_DELAY))
    {
      FastLED.show();
      xSemaphoreGive(timingMutex);
    }

    delay(CELEBRATE_FRAME_MS);
  }
  updateLEDSGoals();
}

void StartupAnimationLEDS()
{
  for (int x = 0; x < 2; x++)
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      goals1[i] = CRGB::Green;
      goals2[i] = CRGB::Green;
      if (timingMutex != NULL && xSemaphoreTake(timingMutex, portMAX_DELAY))
      {
        FastLED.show();
        xSemaphoreGive(timingMutex);
      }
      delay(25);
    }
    for (int i = 0; i < NUM_LEDS; i++)
    {
      goals1[i] = CRGB::Black;
      goals2[i] = CRGB::Black;
      if (timingMutex != NULL && xSemaphoreTake(timingMutex, portMAX_DELAY))
      {
        FastLED.show();
        xSemaphoreGive(timingMutex);
      }
      delay(25);
    }
  }
  updateLEDSGoals();
}

// ─────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────

void SensorLoop(void *pvParameters)
{
  for (;;)
  {
    // 1. Trigger the sensor (10us pulse)
    digitalWrite(TRIG1_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG1_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG1_PIN, LOW);

    // Turn off FreeRTOS task preemption so we aren't interrupted by other tasks
    // while we measure the pulse width. Hardware interrupts (like Wi-Fi) can still fire
    // occasionally, but won't crash the WDT.
    unsigned long distance1 = 0;
    if (xSemaphoreTake(timingMutex, portMAX_DELAY))
    {
      vTaskSuspendAll();
      distance1 = pulseIn(ECHO1_PIN, HIGH, 3000UL);
      xTaskResumeAll();
      xSemaphoreGive(timingMutex);
    }
    float d1 = (distance1 == 0) ? 0 : (distance1 * 0.0343 / 2.0);

    // Trigger second sensor with a slight delay to avoid crosstalk
    delayMicroseconds(100);
    digitalWrite(TRIG2_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG2_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG2_PIN, LOW);

    unsigned long distance2 = 0;
    if (xSemaphoreTake(timingMutex, portMAX_DELAY))
    {
      vTaskSuspendAll();
      distance2 = pulseIn(ECHO2_PIN, HIGH, 3000UL);
      xTaskResumeAll();
      xSemaphoreGive(timingMutex);
    }
    float d2 = (distance2 == 0) ? 0 : (distance2 * 0.0343 / 2.0);

    // 3. Process data here
    unsigned long now = millis();
    // ── Sensor 1 (Team 1 - Blue) ──────────────────────
    if (now >= cooldownUntil1)
    {
      if (d1 > 0 && d1 <= (idleDistance1 - ACTIVATION_DISTANCE_FROM_AVERAGE) && team1 < NUM_LEDS)
      {
        team1++;
        cooldownUntil1 = now + GOAL_COOLDOWN_MS;
        goal1Scored = true;
      }
    }

    // ── Sensor 2 (Team 2 - Red) ──────────────────────
    if (now >= cooldownUntil2)
    {
      if (d2 > 0 && d2 <= (idleDistance2 - ACTIVATION_DISTANCE_FROM_AVERAGE) && team2 < NUM_LEDS)
      {
        team2++;
        cooldownUntil2 = now + GOAL_COOLDOWN_MS;
        goal2Scored = true;
      }
    }

    // Yield control to prevent watchdog starvation on Core 1
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void setup()
{
  delay(1000);
  Serial.begin(9600);

  // Initialize Stepper
  engine.init();
  stepper = engine.stepperConnectToPin(stepPinStepper);
  if (stepper)
  {
    stepper->setDirectionPin(dirPinStepper, true, 5000);
    stepper->setEnablePin(enablePinStepper, true);
    // Stepper should always block (even if not moving)
    stepper->setAutoEnable(true);
    stepper->setDelayToDisable(10000);

    stepper->setSpeedInHz(30000);
    stepper->setAcceleration(5000);
  }
  else
  {
    Serial.println("Failed to initialize stepper!");
  }

  // --- Start Wi-Fi Access Point ---
  Serial.println("\nSetting up Access Point...");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(
      IP); // Connect to this IP in your browser (usually 192.168.4.1)

  // --- Start Web Server & WebSockets ---
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", index_html); });
  server.begin();
  Serial.println("Web Server started");

  FastLED.addLeds<WS2812B, 26, GRB>(goals1, NUM_LEDS);
  FastLED.addLeds<WS2812B, 25, GRB>(goals2, NUM_LEDS);
  FastLED.show();

  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);
  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ECHO2_PIN, INPUT);

  Serial.print("Calibration 1: ");
  Serial.println(idleDistance1);
  Serial.print("Calibration 2: ");
  Serial.println(idleDistance2);

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  timingMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
      SensorLoop,   // Function to implement the task
      "SensorTask", // Name of the task
      10000,        // Stack size in words
      NULL,         // Task input parameter
      10,           // Priority of the task (High priority)
      &SensorTask,  // Task handle
      1             // Core ID (Core 1 is isolated from Core 0 Wi-Fi)
  );

  StartupAnimationLEDS();
}

void loop()
{
  ws.cleanupClients(); // Keep WebSocket memory clean

  if (goal1Scored)
  {
    goal1Scored = false;
    Serial.print("Goal: BLUE | Score: ");
    Serial.print(team1);
    Serial.print(" : ");
    Serial.println(team2);
    notifyClients(); // Broadcast to web interface
    celebrateGoal(goals1);
  }

  if (goal2Scored)
  {
    goal2Scored = false;
    Serial.print("Goal: RED | Score: ");
    Serial.print(team1);
    Serial.print(" : ");
    Serial.println(team2);
    notifyClients(); // Broadcast to web interface
    celebrateGoal(goals2);
  }

  if (digitalRead(RESET_BUTTON_PIN) == LOW || resetRequested)
  {
    // Open Ball Return
    stepper->move(1200);
    team1 = 0;
    team2 = 0;
    notifyClients(); // Broadcast the 0:0 reset to the webpage
    StartupAnimationLEDS();
    resetRequested = false;
    Serial.println("Game reset!");
    delay(1000); // Keep the return open for 2 seconds
    stepper->move(-1200);
    return;
  }
}