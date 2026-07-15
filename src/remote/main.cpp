#include <Arduino.h>
#include <WiFi.h>

#include <optional>

#include "../../lib/link/protocol.h"
#include "config.h"
#include "imu.h"
#include "link.h"

namespace {
std::optional<Imu> g_imu;
EspNowLink g_link;
uint8_t g_seq = 0;

int16_t clampDelta(const float v) {
  if (v > 127.0f) return 127;
  if (v < -127.0f) return -127;
  return static_cast<int16_t>(v);
}

bool pressed(const int pin) {
  return digitalRead(pin) == LOW;
}
}  // namespace

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }
  Serial.print("Remote MAC: ");
  Serial.println(WiFi.macAddress());

  pinMode(config::kPinLeft, INPUT_PULLUP);
  pinMode(config::kPinRight, INPUT_PULLUP);
  pinMode(config::kPinRecenter, INPUT_PULLUP);

  g_imu = createImu(config::kPinSda, config::kPinScl, 0x68);
  if (!g_imu) {
    Serial.println("MPU6050 not found; check wiring.");
  } else {
    Serial.println("Calibrating gyro (hold still)...");
    g_imu->calibrateGyroBias();
  }

  if (!g_link.begin(config::kWifiChannel)) {
    Serial.println("ESP-NOW init failed.");
  }
  if (!g_link.addPeer(config::kReceiverMac)) {
    Serial.println("Failed to add receiver peer; is kReceiverMac set?");
  }
  Serial.println("Remote ready.");
}

void loop() {
  static uint32_t last_tick = 0;
  const uint32_t now = millis();
  if (now - last_tick < config::kTickIntervalMs) {
    return;
  }
  last_tick = now;

  if (!g_imu || !g_imu->updateCurrentSample()) {
    return;
  }
  const auto& s = g_imu->currentSample();

  proto::MouseReport report = {};
  report.who_am_i = proto::kWhoAmI;
  report.version = proto::kVersion;
  report.seq = g_seq++;

  if (pressed(config::kPinRecenter)) {
    report.dx = 0;
    report.dy = 0;
  } else {
    report.dx = clampDelta(-s.gz * config::kMoveScale);
    report.dy = clampDelta(s.gx * config::kMoveScale);
  }

  if (pressed(config::kPinLeft))
    report.buttons |= 1 << static_cast<uint8_t>(proto::Button::kBtnLeft);
  if (pressed(config::kPinRight))
    report.buttons |= 1 << static_cast<uint8_t>(proto::Button::kBtnRight);

  g_link.send(reinterpret_cast<const uint8_t*>(&report), sizeof(report));
}
