#include "webserver.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "config.h"
#include "game_state.h"
#include "leds.h"
#include "site.h"

namespace
{
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String scoreJson()
{
  return "{\"team1\":" + String(getScore(TEAM_BLUE)) +
         ",\"team2\":" + String(getScore(TEAM_RED)) + "}";
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (!(info->final && info->index == 0 && info->len == len &&
        info->opcode == WS_TEXT))
  {
    return;
  }

  data[len] = 0;
  // Length-aware: never relies on the byte past the frame buffer.
  const String message((const char *)data, len);

  if (message == "t1_add")
    addGoal(TEAM_BLUE);
  if (message == "t1_sub")
    removeGoal(TEAM_BLUE);
  if (message == "t2_add")
    addGoal(TEAM_RED);
  if (message == "t2_sub")
    removeGoal(TEAM_RED);

  if (message == "reset")
  {
    requestReset();
  }
  else
  {
    // Only loop() may talk to FastLED; this task just marks the score dirty
    // and loop() redraws it on its next iteration.
    requestLedUpdate();
    notifyClients();
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  (void)server;

  if (type == WS_EVT_CONNECT)
  {
    // Send current score immediately upon connection.
    client->text(scoreJson());
  }
  else if (type == WS_EVT_DATA)
  {
    handleWebSocketMessage(arg, data, len);
  }
}

void sendDashboard(AsyncWebServerRequest *request)
{
  request->send(200, "text/html", (const uint8_t *)index_html, index_html_len);
}
} // namespace

void setupWebServer()
{
  Serial.println("\nSetting up Access Point...");
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(
      IP); // Connect to this IP in your browser (usually 192.168.4.1)

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Always send the page together with its length.  With only a const char*
  // ESPAsyncWebServer copies the whole ~120 kB dashboard into a String on the
  // heap (AsyncBasicResponse).  With the WiFi AP running the largest free heap
  // block of an ESP32 is usually ~110 kB, so that allocation fails and the
  // browser gets a 200 response with no body -> blank white page.  The length
  // overload renders an AsyncProgmemResponse instead, which streams the page
  // in small chunks straight from flash and does no large allocation.
  server.on("/", HTTP_GET, sendDashboard);
  // Same dashboard, but with TV mode enabled (?tv=1 also works).
  server.on("/tv", HTTP_GET, sendDashboard);

  server.begin();
  Serial.println("Web Server started");
}

void notifyClients()
{
  ws.textAll(scoreJson());
}

void cleanupWebSockets()
{
  ws.cleanupClients(); // Keep WebSocket memory clean
}
