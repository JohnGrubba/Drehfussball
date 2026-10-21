#include <AsyncTCP.h>
#include <atomic>
#include <ESPAsyncWebServer.h>
#include <FastAccelStepper.h>
#include <FastLED.h>
#include <WiFi.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

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
#define CELEBRATE_FRAME_MS 40

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
// The ball is in a closed chute, so a single beam is enough.  A valid goal is
// a beam occlusion between MIN_BLOCK_US and MAX_BLOCK_US.  Everything shorter
// is noise/dirt, everything longer is a stuck ball or a broken sensor.
#define SENSOR1_PIN 27
#define SENSOR2_PIN 32

#define MIN_BLOCK_US 2000UL        // 2 ms
#define MAX_BLOCK_US 500000UL      // 500 ms
#define GOAL_COOLDOWN_US 1500000UL // 1.5 s per side

// Set to 1 to print every beam edge / duration to Serial.
#define DEBUG_SENSORS 0

CRGB goals1[NUM_LEDS] = {};
CRGB goals2[NUM_LEDS] = {};

std::atomic<uint8_t> team1{0};
std::atomic<uint8_t> team2{0};

volatile bool resetRequested = false;
volatile bool goal1Scored = false;
volatile bool goal2Scored = false;

SemaphoreHandle_t timingMutex;

// ─────────────────────────────────────────────
// Sensor event queue
// ─────────────────────────────────────────────
struct BeamEvent
{
  uint8_t goal; // 0 = blue, 1 = red
  bool blocked;
  uint32_t us;
};

static QueueHandle_t sensorQueue = nullptr;

// ─────────────────────────────────────────────
// Helpers & WebSockets
// ─────────────────────────────────────────────
void notifyClients()
{
  String json =
      "{\"team1\":" + String(team1.load()) + ",\"team2\":" + String(team2.load()) + "}";
  ws.textAll(json);
}

void updateLEDSGoals()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    goals1[9 - i] = (team1.load() > i) ? CRGB::Blue : CRGB::Black;
    goals2[i] = (team2.load() > i) ? CRGB::Red : CRGB::Black;
  }

  if (timingMutex != NULL && xSemaphoreTake(timingMutex, portMAX_DELAY))
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
      team1.fetch_add(1);
    if (message == "t1_sub" && team1.load() > 0)
      team1.fetch_sub(1);
    if (message == "t2_add")
      team2.fetch_add(1);
    if (message == "t2_sub" && team2.load() > 0)
      team2.fetch_sub(1);

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
    client->text("{\"team1\":" + String(team1.load()) + ",\"team2\":" + String(team2.load()) +
                 "}");
  }
  else if (type == WS_EVT_DATA)
  {
    handleWebSocketMessage(arg, data, len);
  }
}

void celebrateGoal(CRGB *strip)
{
  for (int flash = 0; flash < 8; flash++)
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
// IR sensor ISR + task
// ─────────────────────────────────────────────
static inline void IRAM_ATTR pushBeamEvent(uint8_t goal, uint8_t pin)
{
  BeamEvent ev;
  ev.goal = goal;
  ev.blocked = (gpio_get_level((gpio_num_t)pin) != 0); // 74HC14 inverts
  ev.us = (uint32_t)esp_timer_get_time();

  BaseType_t woken = pdFALSE;
  xQueueSendFromISR(sensorQueue, &ev, &woken);
  (void)woken;
  // The queue wakes sensorTask; an explicit portYIELD_FROM_ISR() is not
  // required here, the task runs on the next scheduler slot (<= 1 ms).
}

void IRAM_ATTR isrGoal1() { pushBeamEvent(0, SENSOR1_PIN); }
void IRAM_ATTR isrGoal2() { pushBeamEvent(1, SENSOR2_PIN); }

static void sensorTask(void *)
{
  BeamEvent ev;

  bool blocked[2] = {false, false};
  uint32_t blockUs[2] = {0, 0};
  bool cooldownActive[2] = {false, false};
  uint32_t cooldownUntil[2] = {0, 0};

  for (;;)
  {
    if (xQueueReceive(sensorQueue, &ev, portMAX_DELAY) != pdTRUE)
    {
      continue;
    }

    const uint8_t g = ev.goal;

    if (ev.blocked)
    {
      if (!blocked[g])
      {
        blocked[g] = true;
        blockUs[g] = ev.us;
      }
      continue;
    }

    if (!blocked[g])
    {
      continue;
    }

    blocked[g] = false;
    const uint32_t duration = ev.us - blockUs[g];
    const bool valid = (duration >= MIN_BLOCK_US && duration <= MAX_BLOCK_US);
    // Wrap-safe cooldown check (works across the 32-bit micros() wrap).
    const bool ready =
        !cooldownActive[g] || ((int32_t)(ev.us - cooldownUntil[g]) >= 0);

#if DEBUG_SENSORS
    Serial.printf("S%u %s %lu us\n", g + 1, (valid && ready) ? "GOAL" : "reject",
                  (unsigned long)duration);
#endif

    if (valid && ready)
    {
      if (g == 0)
        goal1Scored = true;
      else
        goal2Scored = true;
      cooldownActive[g] = true;
      cooldownUntil[g] = ev.us + GOAL_COOLDOWN_US;
    }
  }
}

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
      sensorTask,   // Function
      "Sensors",    // Name
      4096,         // Stack size in words
      NULL,         // Parameter
      6,            // Priority
      NULL,         // Handle
      1             // Core 1
  );
}

// ─────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────
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
  // Same dashboard, but with TV mode enabled (?tv=1 also works).
  server.on("/tv", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", index_html); });
  server.begin();
  Serial.println("Web Server started");

  FastLED.addLeds<WS2812B, 26, GRB>(goals1, NUM_LEDS);
  FastLED.addLeds<WS2812B, 25, GRB>(goals2, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.show();

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  timingMutex = xSemaphoreCreateMutex();

  // --- Start IR goal detection ---
  setupSensors();

  StartupAnimationLEDS();
}

void loop()
{
  ws.cleanupClients(); // Keep WebSocket memory clean

  if (goal1Scored)
  {
    goal1Scored = false;
    team1.fetch_add(1);
    Serial.print("Goal: BLUE | Score: ");
    Serial.print(team1.load());
    Serial.print(" : ");
    Serial.println(team2.load());
    notifyClients(); // Broadcast to web interface
    celebrateGoal(goals1);
  }

  if (goal2Scored)
  {
    goal2Scored = false;
    team2.fetch_add(1);
    Serial.print("Goal: RED | Score: ");
    Serial.print(team1.load());
    Serial.print(" : ");
    Serial.println(team2.load());
    notifyClients(); // Broadcast to web interface
    celebrateGoal(goals2);
  }

  if (digitalRead(RESET_BUTTON_PIN) == LOW || resetRequested)
  {
    // Open Ball Return
    if (stepper)
      stepper->move(1200);
    goal1Scored = false;
    goal2Scored = false;
    team1.store(0);
    team2.store(0);
    notifyClients(); // Broadcast the 0:0 reset to the webpage
    StartupAnimationLEDS();
    resetRequested = false;
    Serial.println("Game reset!");
    delay(1000); // Keep the return open for 1 second
    goal1Scored = false;
    goal2Scored = false;
    if (stepper)
      stepper->move(-1200);
    return;
  }
}
