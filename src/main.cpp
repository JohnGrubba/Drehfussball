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
#define INITIAL_AVERAGE_MEASURE_COUNT 50   // Samples taken at startup
#define ACTIVATION_DISTANCE_FROM_AVERAGE 1 // cm below idle = "object present"

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

float getFastDistance(int trigPin, int echoPin)
{
  // Single, fast ping to avoid missing fast-moving balls
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Timeout of 4000us max (~68 cm max distance). This keeps the loop extremely
  // fast.
  long duration = pulseIn(echoPin, HIGH, 4000UL);
  return (duration == 0) ? 0 : (duration * 0.0343 / 2.0);
}

volatile uint8_t team1 = 0;
volatile uint8_t team2 = 0;

volatile bool resetRequested = false;

float idleDistance1 = 0;
float idleDistance2 = 0;

volatile unsigned long cooldownUntil1 = 0;
volatile unsigned long cooldownUntil2 = 0;

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
  FastLED.show();
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
    FastLED.show();
    delay(CELEBRATE_FRAME_MS);
  }
  updateLEDSGoals();
}

void StartupAnimationLEDS()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    goals1[i] = CRGB::Green;
    goals2[i] = CRGB::Green;
    FastLED.show();
    delay(50);
  }
  for (int i = 0; i < NUM_LEDS; i++)
  {
    goals1[i] = CRGB::Black;
    goals2[i] = CRGB::Black;
    FastLED.show();
    delay(50);
  }
  updateLEDSGoals();
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
    stepper->setDelayToDisable(5000);

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

  RunningMedian median1(INITIAL_AVERAGE_MEASURE_COUNT);
  RunningMedian median2(INITIAL_AVERAGE_MEASURE_COUNT);

  for (int i = 0; i < INITIAL_AVERAGE_MEASURE_COUNT; i++)
  {
    float r1 = getFastDistance(TRIG1_PIN, ECHO1_PIN);
    if (r1 > 0)
      median1.add(r1);
    float r2 = getFastDistance(TRIG2_PIN, ECHO2_PIN);
    if (r2 > 0)
      median2.add(r2);
  }
  idleDistance1 = median1.getAverage();
  idleDistance2 = median2.getAverage();

  Serial.print("Calibration 1: ");
  Serial.println(idleDistance1);
  Serial.print("Calibration 2: ");
  Serial.println(idleDistance2);

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  StartupAnimationLEDS();
}

void loop()
{
  ws.cleanupClients(); // Keep WebSocket memory clean

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

  unsigned long now = millis();
  float d1 = getFastDistance(TRIG1_PIN, ECHO1_PIN);
  float d2 = getFastDistance(TRIG2_PIN, ECHO2_PIN);

  // ── Sensor 1 (Team 1 - Blue) ──────────────────────
  if (now >= cooldownUntil1)
  {
    if (d1 > 0 && d1 <= (idleDistance1 - ACTIVATION_DISTANCE_FROM_AVERAGE))
    {
      delay(8); // very brief delay
      float d1_verify = getFastDistance(TRIG1_PIN, ECHO1_PIN);
      if (d1_verify > 0 &&
          d1_verify <= (idleDistance1 - ACTIVATION_DISTANCE_FROM_AVERAGE))
      {
        team1++;
        cooldownUntil1 = now + GOAL_COOLDOWN_MS;
        Serial.print("Goal: BLUE | Score: ");
        Serial.print(team1);
        Serial.print(" : ");
        Serial.println(team2);
        notifyClients(); // Broadcast to web interface
        celebrateGoal(goals1);
      }
    }
  }

  // ── Sensor 2 (Team 2 - Red) ──────────────────────
  if (now >= cooldownUntil2)
  {
    if (d2 > 0 && d2 <= (idleDistance2 - ACTIVATION_DISTANCE_FROM_AVERAGE))
    {
      delay(8); // very brief delay
      float d2_verify = getFastDistance(TRIG2_PIN, ECHO2_PIN);
      if (d2_verify > 0 &&
          d2_verify <= (idleDistance2 - ACTIVATION_DISTANCE_FROM_AVERAGE))
      {
        team2++;
        cooldownUntil2 = now + GOAL_COOLDOWN_MS;
        Serial.print("Goal: RED | Score: ");
        Serial.print(team1);
        Serial.print(" : ");
        Serial.println(team2);
        notifyClients(); // Broadcast to web interface
        celebrateGoal(goals2);
      }
    }
  }
}