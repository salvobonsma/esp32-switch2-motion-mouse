#pragma once

namespace proto {

constexpr uint8_t kWhoAmI = 0x77;
constexpr uint8_t kVersion = 1;

enum class Button : uint8_t {
  kBtnLeft,
  kBtnRight,
};

struct __attribute__((packed)) MouseReport {
  uint8_t who_am_i;
  uint8_t version;
  int16_t dx; // dx+ = right
  int16_t dy; // dy+ = down
  uint8_t buttons;
  uint8_t seq;
};

}  // namespace proto
