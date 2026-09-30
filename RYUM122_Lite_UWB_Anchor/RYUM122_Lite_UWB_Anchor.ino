// UWB Anchor — RYUW122 in RESPONDER mode
// ESP32 pin 17 (TX2) → RYUW122 RXD
// ESP32 pin 16 (RX2) ← RYUW122 TXD
// ESP32 pin 5         → RYUW122 NRST (reset pulse at boot)
// Author: Tipu / IoT Bhai

#include <Arduino.h>

#define UWB_TX 17   // board label TX2
#define UWB_RX 16   // board label RX2
#define UWB_NRST 5  // GPIO5 -> RYUW122 NRST (pin 2)
#define LED    2   // built-in LED on most ESP32 dev boards

HardwareSerial UWB(2);

// -------- Network settings (must match the tag) --------
// Per Reyax AT guide: NETWORKID and ADDRESS are exactly 8 ASCII chars,
// CPIN is a 32-char hex AES-128 key. Shorter values are rejected (+ERR=3).
const char* NET_ID  = "IOTBHAI1";
const char* NET_PIN = "10203040506070801020304050607080";
const char* MY_ADDR = "ANCHOR01";  // change to ANCHOR02, ANCHOR03 for extra anchors later

// ---- AT helpers ----
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

void blink(uint8_t n, uint16_t ms = 80) {
  for (uint8_t i = 0; i < n; i++) {
    digitalWrite(LED, HIGH); delay(ms);
    digitalWrite(LED, LOW);  delay(ms);
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
  Serial.print  ("  UWB Anchor  ["); Serial.print(MY_ADDR); Serial.println("]");
  Serial.println("======================================");

  // Sanity check
  sendAT("AT");

  // Responder mode. Note: Reyax calls MODE=0 "TAG" (the passive side),
  // so our fixed "anchor" is a Reyax TAG. Only naming differs.
  sendAT("AT+MODE=0");

  // Network + address + shared key
  sendAT(String("AT+NETWORKID=") + NET_ID);
  sendAT(String("AT+ADDRESS=") + MY_ADDR);
  sendAT(String("AT+CPIN=") + NET_PIN);

  // Preload the 1-byte reply payload. Reyax wants initiator/responder
  // payloads within 3 bytes of each other for the distance calc.
  sendAT("AT+TAG_SEND=1,P");

  // Read back what the module actually stored
  Serial.println("\n-- verify --");
  sendAT("AT+MODE?", 1);
  sendAT("AT+NETWORKID?", 1);
  sendAT("AT+ADDRESS?", 1);
  sendAT("AT+CPIN?", 1);

  Serial.println();
  Serial.println("Anchor ready. Waiting for ranging requests...");
  Serial.println("Any incoming +ANCHOR_RCV lines are exchanges in progress.");
  Serial.println();

  blink(3, 120);
}

void loop() {
  // Forward whatever the RYUW122 emits to USB serial for visibility,
  // and blink LED on every incoming byte so you can see it working.
  static uint32_t ledOn = 0;
  static String line;
  while (UWB.available()) {
    char c = UWB.read();
    Serial.write(c);
    digitalWrite(LED, HIGH);
    ledOn = millis();
    if (c == '\n') {
      // Each exchange consumes the stored payload, so re-arm it
      if (line.startsWith("+TAG_RCV=")) UWB.print("AT+TAG_SEND=1,P\r\n");
      line = "";
    } else if (c != '\r' && line.length() < 64) {
      line += c;
    }
  }
  if (ledOn && millis() - ledOn > 40) { digitalWrite(LED, LOW); ledOn = 0; }
}
