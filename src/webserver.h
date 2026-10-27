#pragma once

// Start the Wi-Fi access point, the HTTP server and the WebSocket handler.
void setupWebServer();

// Broadcast the current score to all connected dashboards.
void notifyClients();

// Free memory of disconnected WebSocket clients; call regularly from loop().
void cleanupWebSockets();
