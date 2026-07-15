#pragma once

namespace config {
constexpr uint8_t kWifiChannel = 1;
constexpr uint8_t kReceiverMac[6] = {0x48, 0xF6, 0xEE, 0x79, 0xDB, 0x36};

// Remote GPIO
constexpr int kPinSda = 21;
constexpr int kPinScl = 22;
constexpr int kPinLeft = 23;
constexpr int kPinRight = 4;
constexpr int kPinRecenter = 0;

// Remote gyro tuning
constexpr float kMoveScale = 0.6f;

constexpr uint32_t kTickIntervalMs = 8;

}  // namespace config
