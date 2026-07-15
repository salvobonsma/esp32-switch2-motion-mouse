#pragma once

#include <cstddef>
#include <cstdint>

// Thin wrapper around the ESP-NOW C API
class EspNowLink {
 public:
  using ReceiveCallback = void (*)(const uint8_t* data, size_t len);
  bool begin(uint8_t channel);
  bool addPeer(const uint8_t mac[6]) const;
  static bool send(const uint8_t* data, size_t len);
  static void onReceive(ReceiveCallback cb);

 private:
  uint8_t channel_ = 1;
};
