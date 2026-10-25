#include <AsyncTCP.h>
#include <atomic>
#include <ESPAsyncWebServer.h>
#include <FastAccelStepper.h>
#include <FastLED.h>
#include <WiFi.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "site.cpp"

// --- Wi-Fi & Web Server Settings ---
const char *ssid = "Drehfussball";   // Hotspot Name
const char *password = "Kennwort1!"; // Hotspot Password (min 8 chars)

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

#define dirPinStepper 18
#define enablePinStepper 19
#define stepPinStepper 21

#define LED_1 16
#define LED_2 17

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;

#define NUM_LEDS 10
#define RESET_BUTTON_PIN 23
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
// The ball is in a closed chute, so a single beam is enough.  Every beam
// interruption (the ball entering the chute) counts as a goal.
#define SENSOR1_PIN 22
#define SENSOR2_PIN 4

// After a goal BOTH beams are locked out for one second: the GPIO interrupts of
// both sensors are disabled, so a single ball can never score twice (e.g. on the
// blue and the red side one after another).
#define SENSOR_LOCKOUT_MS 1000

// Set to 1 to print every accepted goal to Serial.
#define DEBUG_SENSORS 1

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
// IR sensor ISRs + task
// ─────────────────────────────────────────────
static inline void IRAM_ATTR pushBeamEvent(uint8_t goal, uint8_t pin)
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
static void setBeamInterrupts(bool enable)
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
static void resyncBeams(bool blocked[2])
{
  const uint8_t pins[2] = {SENSOR1_PIN, SENSOR2_PIN};

  for (uint8_t g = 0; g < 2; g++)
  {
    // 74HC14 inverts: HIGH means the beam is blocked.
    blocked[g] = (gpio_get_level((gpio_num_t)pins[g]) != 0);
  }
}

static void sensorTask(void *)
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
    if (g == 0)
      goal1Scored = true;
    else
      goal2Scored = true;

#if DEBUG_SENSORS
    Serial.printf("S%u GOAL -> lockout %u ms\n", g + 1, SENSOR_LOCKOUT_MS);
#endif

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
  // Always send the page together with its length.  With only a const char*
  // ESPAsyncWebServer copies the whole ~120 kB dashboard into a String on the
  // heap (AsyncBasicResponse).  With the WiFi AP running the largest free heap
  // block of an ESP32 is usually ~110 kB, so that allocation fails and the
  // browser gets a 200 response with no body -> blank white page.  The length
  // overload renders an AsyncProgmemResponse instead, which streams the page
  // in small chunks straight from flash and does no large allocation.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", (const uint8_t *)index_html,
                            sizeof(index_html) - 1); });
  // Same dashboard, but with TV mode enabled (?tv=1 also works).
  server.on("/tv", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", (const uint8_t *)index_html,
                            sizeof(index_html) - 1); });
  server.begin();
  Serial.println("Web Server started");

  FastLED.addLeds<WS2812B, LED_1, GRB>(goals1, NUM_LEDS);
  FastLED.addLeds<WS2812B, LED_2, GRB>(goals2, NUM_LEDS);
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
