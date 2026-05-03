#include <FastLED.h>
#include <Ultrasonic.h>
#include <RunningMedian.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "site.cpp"

// --- Wi-Fi & Web Server Settings ---
const char *ssid = "Drehfussball";   // Hotspot Name
const char *password = "Kennwort1!"; // Hotspot Password (min 8 chars)

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Rechte Seite Sensor: Echo 12, Trig 13
// Linke Seite Sensor: Echo 10, Trig 11

#define NUM_LEDS 10
#define RESET_BUTTON_PIN 32

// --- Calibration ---
#define INITIAL_AVERAGE_MEASURE_COUNT 50   // Samples taken at startup
#define ACTIVATION_DISTANCE_FROM_AVERAGE 1 // cm below idle = "object present"

// After a goal is registered, ignore that sensor for this many ms.
#define GOAL_COOLDOWN_MS 2000

// During the cooldown the celebration animation runs.
#define CELEBRATE_FRAME_MS 80

CRGB goals1[NUM_LEDS] = {};
CRGB goals2[NUM_LEDS] = {};

Ultrasonic goal2Sensor(12, 13, 10000UL);
Ultrasonic goal1Sensor(27, 14, 10000UL);

volatile uint8_t team1 = 0;
volatile uint8_t team2 = 0;

volatile bool resetRequested = false;

float idleDistance1 = 0;
float idleDistance2 = 0;

volatile uint8_t hits1 = 0;
volatile uint8_t hits2 = 0;

volatile unsigned long cooldownUntil1 = 0;
volatile unsigned long cooldownUntil2 = 0;

// ─────────────────────────────────────────────
// Helpers & WebSockets
// ─────────────────────────────────────────────

void notifyClients()
{
  String json = "{\"team1\":" + String(team1) + ",\"team2\":" + String(team2) + "}";
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
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
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

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    // Send current score immediately upon connection
    client->text("{\"team1\":" + String(team1) + ",\"team2\":" + String(team2) + "}");
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

void ResetGoals()
{
  resetRequested = true;
}

// ─────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────

void setup()
{
  delay(1000);
  Serial.begin(9600);

  // --- Start Wi-Fi Access Point ---
  Serial.println("\nSetting up Access Point...");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP); // Connect to this IP in your browser (usually 192.168.4.1)

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

  RunningMedian median1(INITIAL_AVERAGE_MEASURE_COUNT);
  RunningMedian median2(INITIAL_AVERAGE_MEASURE_COUNT);

  for (int i = 0; i < INITIAL_AVERAGE_MEASURE_COUNT; i++)
  {
    unsigned int r1 = goal1Sensor.read(CM);
    if (r1 > 0)
      median1.add(r1);
    unsigned int r2 = goal2Sensor.read(CM);
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
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON_PIN), ResetGoals, FALLING);
  StartupAnimationLEDS();
}

void loop()
{
  ws.cleanupClients(); // Keep WebSocket memory clean

  if (resetRequested)
  {
    team1 = 0;
    team2 = 0;
    hits1 = 0;
    hits2 = 0;
    cooldownUntil1 = 0;
    cooldownUntil2 = 0;
    StartupAnimationLEDS();
    notifyClients(); // Broadcast the 0:0 reset to the webpage
    resetRequested = false;
    return;
  }

  unsigned long now = millis();
  unsigned int d1 = goal1Sensor.read(CM);
  unsigned int d2 = goal2Sensor.read(CM);

  // ── Sensor 1 (Team 1 - Blue) ──────────────────────
  if (now >= cooldownUntil1)
  {
    if (d1 > 0 && d1 < (idleDistance1 - ACTIVATION_DISTANCE_FROM_AVERAGE))
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

  // ── Sensor 2 (Team 2 - Red) ──────────────────────
  if (now >= cooldownUntil2)
  {
    if (d2 > 0 && d2 < (idleDistance2 - ACTIVATION_DISTANCE_FROM_AVERAGE))
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