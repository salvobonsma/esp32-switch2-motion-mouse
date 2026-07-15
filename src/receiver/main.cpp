#include <Arduino.h>
#include <WiFi.h>

#include "../../lib/link/protocol.h"
#include "USB.h"
#include "USBHIDMouse.h"
#include "config.h"
#include "link.h"

namespace {
USBHIDMouse g_mouse;
EspNowLink g_link;

volatile bool g_fresh = false;
volatile uint32_t g_last_rx_ms = 0;
proto::MouseReport g_latest;

uint8_t g_prev_buttons = 0;

constexpr uint8_t kBtnLeftMask =
    (1 << static_cast<uint8_t>(proto::Button::kBtnLeft));
constexpr uint8_t kBtnRightMask =
    (1 << static_cast<uint8_t>(proto::Button::kBtnRight));

int8_t clamp8(const int16_t v) {
  if (v > 127) return 127;
  if (v < -127) return -127;
  return static_cast<int8_t>(v);
}

void onFrame(const uint8_t* data, const size_t len) {
  if (len != sizeof(proto::MouseReport)) {
    return;
  }
  proto::MouseReport r;
  memcpy(&r, data, sizeof(r));
  if (r.who_am_i != proto::kWhoAmI || r.version != proto::kVersion) {
    return;
  }
  g_latest = r;
  g_last_rx_ms = millis();
  g_fresh = true;
}

void applyButton(uint8_t buttons, uint8_t mask, uint8_t hid_button) {
  const bool now_down = buttons & mask;
  const bool was_down = g_prev_buttons & mask;
  if (now_down && !was_down) {
    g_mouse.press(hid_button);
  } else if (!now_down && was_down) {
    g_mouse.release(hid_button);
  }
}
}  // namespace

void setLed(bool on) {
  neopixelWrite(LED_BUILTIN, on ? 10 : 0, on ? 10 : 0, 0);
}

void setup() {
  Serial.begin(115200);
  setLed(false);

  g_link.begin(config::kWifiChannel);
  g_link.onReceive(onFrame);

  Serial.print("Receiver MAC (put this in config.h kReceiverMac): ");
  Serial.println(WiFi.macAddress());

  g_mouse.begin();
  USB.begin();
  Serial.println("Receiver ready (USB HID mouse).");
}

void loop() {
  static uint32_t last_mac_print = 0;
  if (g_last_rx_ms == 0 && millis() - last_mac_print > 1000) {
    last_mac_print = millis();
    Serial.print("Receiver MAC (put in config.h kReceiverMac): ");
    Serial.println(WiFi.macAddress());
  }

  const bool connected = g_last_rx_ms != 0 && millis() - g_last_rx_ms < 500;
  setLed(connected);

  if (g_fresh) {
    g_fresh = false;
    const proto::MouseReport r = g_latest;  // snapshot

    if (r.dx != 0 || r.dy != 0) {
      g_mouse.move(clamp8(r.dx), clamp8(r.dy));
    }
    applyButton(r.buttons, kBtnLeftMask, MOUSE_LEFT);
    applyButton(r.buttons, kBtnRightMask, MOUSE_RIGHT);
    g_prev_buttons = r.buttons;
  }

  // If the remote goes silent, release any held button so a click never sticks.
  if (g_prev_buttons != 0 && millis() - g_last_rx_ms > 500) {
    if (g_prev_buttons & kBtnLeftMask) g_mouse.release(MOUSE_LEFT);
    if (g_prev_buttons & kBtnRightMask) g_mouse.release(MOUSE_RIGHT);
    g_prev_buttons = 0;
  }
}
