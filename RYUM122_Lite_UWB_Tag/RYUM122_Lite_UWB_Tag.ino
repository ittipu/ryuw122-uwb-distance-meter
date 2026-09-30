// UWB Tag — RYUW122 in INITIATOR mode, hosts the dashboard over Wi-Fi
// ESP32 pin 17 (TX2) → RYUW122 RXD
// ESP32 pin 16 (RX2) ← RYUW122 TXD
// ESP32 pin 5         → RYUW122 NRST (reset pulse at boot)
// Author: Tipu / IoT Bhai
//
// Boot → creates Wi-Fi AP "UWB-Tag" (pass: iotbhai123)
// Phone → connects to that AP, opens http://192.168.4.1/
// Dashboard loads, WebSocket streams live distance at ~15 Hz.

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#define UWB_TX 17   // board label TX2
#define UWB_RX 16   // board label RX2
#define UWB_NRST 5  // GPIO5 -> RYUW122 NRST (pin 2)
#define LED    2

HardwareSerial UWB(2);
WebServer      http(80);
WebSocketsServer ws(81);

// -------- Network settings (must match the anchor) --------
// Per Reyax AT guide: NETWORKID and ADDRESS are exactly 8 ASCII chars,
// CPIN is a 32-char hex AES-128 key. Shorter values are rejected (+ERR=3).
const char* NET_ID     = "IOTBHAI1";
const char* NET_PIN    = "10203040506070801020304050607080";
const char* MY_ADDR    = "TAG00001";
const char* PEER_ADDR  = "ANCHOR01";

// -------- Wi-Fi AP --------
const char* AP_SSID = "UWB-Tag";
const char* AP_PASS = "iotbhai123";

// Ranging cadence
const uint32_t RANGE_INTERVAL_MS = 67;   // ~15 Hz

// Per-module calibration offset in metres (measure at a known 3 m distance once)
const float CAL_OFFSET_M = 0.00f;

// ---- AT helper ----
// The module drops characters if commands arrive back-to-back (it's busy
// writing flash), so: pause before each command, pace the bytes, retry.
String sendAT(const String& cmd, uint8_t tries = 3, uint32_t timeout = 400) {
  String resp;
  for (uint8_t i = 0; i < tries; i++) {
    delay(100);
    while (UWB.available()) UWB.read();        // flush
    Serial.print(">> "); Serial.println(cmd);
    for (size_t k = 0; k < cmd.length(); k++) { UWB.write(cmd[k]); delayMicroseconds(500); }
    UWB.print("\r\n");

    resp = "";
    uint32_t t0 = millis();
    while (millis() - t0 < timeout) {
      if (!UWB.available()) continue;
      resp += (char)UWB.read();
      if (resp.endsWith("\r\n") && resp.indexOf('+') >= 0) break;   // +OK, +ERR=n, +XXX=value
    }
    Serial.print("<< "); Serial.print(resp);
    if (!resp.endsWith("\r\n")) Serial.println("(timeout)");
    if (resp.indexOf('+') >= 0 && resp.indexOf("+ERR") < 0) return resp;
  }
  return resp;
}

// Perform one ranging exchange with PEER_ADDR. Returns metres, or NAN on failure.
float rangeToPeer() {
  while (UWB.available()) UWB.read();

  // AT+ANCHOR_SEND=<addr>,<len>,<data>
  String cmd = String("AT+ANCHOR_SEND=") + PEER_ADDR + ",1,P";
  UWB.print(cmd); UWB.print("\r\n");

  String resp;
  uint32_t t0 = millis();
  while (millis() - t0 < 150) {                 // 150 ms max per exchange
    while (UWB.available()) {
      char c = UWB.read();
      resp += c;
      // Look for +ANCHOR_RCV=<addr>,<len>,<data>,<distance> cm[,<rssi>]
      // Distance is the 4th field; RSSI is appended if AT+RSSI=1.
      int p = resp.indexOf("+ANCHOR_RCV=");
      if (p >= 0) {
        int eol = resp.indexOf("\r\n", p);
        if (eol > 0) {
          String line = resp.substring(p, eol);
          int c1 = line.indexOf(',');
          int c2 = line.indexOf(',', c1 + 1);
          int c3 = line.indexOf(',', c2 + 1);
          if (c1 > 0 && c2 > 0 && c3 > 0) {
            String cm = line.substring(c3 + 1);   // "40 cm" or "40 cm,-80"
            cm.trim();
            if (cm.length() && isDigit(cm[0])) return cm.toInt() / 100.0f + CAL_OFFSET_M;
          }
          return NAN;
        }
      }
      if (resp.indexOf("+ERR=") >= 0) return NAN;
    }
  }
  return NAN;
}

#include "dashboard.h"

// ---- Web + WebSocket handlers ----
void handleRoot() {
  http.send_P(200, "text/html", INDEX_HTML);
}
void wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
  if (type == WStype_CONNECTED) {
    Serial.printf("[ws] client %u connected\n", num);
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("[ws] client %u disconnected\n", num);
  }
}

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  delay(200);

  UWB.begin(115200, SERIAL_8N1, UWB_RX, UWB_TX);

  // On ESP32 the module doesn't start cleanly on its own —
  // pulse NRST once after the UART is up, then wait for +READY.
  pinMode(UWB_NRST, OUTPUT);
  digitalWrite(UWB_NRST, LOW);
  delay(100);
  digitalWrite(UWB_NRST, HIGH);
  delay(1000);

  Serial.println();
  Serial.println("======================================");
  Serial.print  ("  UWB Tag  ["); Serial.print(MY_ADDR);
  Serial.print  (" -> "); Serial.print(PEER_ADDR); Serial.println("]");
  Serial.println("======================================");

  // Configure RYUW122 as initiator. Note: Reyax calls MODE=1 "ANCHOR"
  // (the side that reports distance), so our moving tag is a Reyax ANCHOR.
  sendAT("AT");
  sendAT("AT+MODE=1");
  sendAT(String("AT+NETWORKID=") + NET_ID);
  sendAT(String("AT+ADDRESS=") + MY_ADDR);
  sendAT(String("AT+CPIN=") + NET_PIN);

  // Read back what the module actually stored
  Serial.println("\n-- verify --");
  sendAT("AT+MODE?", 1);
  sendAT("AT+NETWORKID?", 1);
  sendAT("AT+ADDRESS?", 1);
  sendAT("AT+CPIN?", 1);
  Serial.println();

  // Bring up Wi-Fi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP  SSID: "); Serial.println(AP_SSID);
  Serial.print("    PASS: "); Serial.println(AP_PASS);
  Serial.print("    URL : http://"); Serial.println(ip);

  // Servers
  http.on("/", handleRoot);
  http.begin();
  ws.begin();
  ws.onEvent(wsEvent);

  Serial.println("Ready. Ranging starts now.");
  digitalWrite(LED, HIGH);
}

void loop() {
  http.handleClient();
  ws.loop();

  static uint32_t last = 0;
  if (millis() - last >= RANGE_INTERVAL_MS) {
    last = millis();

    float d = rangeToPeer();
    if (!isnan(d)) {
      char buf[32];
      snprintf(buf, sizeof(buf), "{\"d\":%.2f}", d);
      ws.broadcastTXT(buf);
      Serial.printf("d = %.2f m\n", d);
      digitalWrite(LED, HIGH);
    } else {
      digitalWrite(LED, LOW);
      Serial.println("range: no reply");
    }
  }
}
