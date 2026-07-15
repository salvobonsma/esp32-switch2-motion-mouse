#include "link.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <cstring>

namespace {
EspNowLink::ReceiveCallback g_user_cb = nullptr;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void RecvTrampoline(const esp_now_recv_info_t*, const uint8_t* data, int len) {
#else
void RecvTrampoline(const uint8_t*, const uint8_t* data, int len) {
#endif
  if (g_user_cb != nullptr && len > 0) {
    g_user_cb(data, static_cast<size_t>(len));
  }
}
}  // namespace

bool EspNowLink::begin(const uint8_t channel) {
  channel_ = channel;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(channel_, WIFI_SECOND_CHAN_NONE);
  return esp_now_init() == ESP_OK;
}

bool EspNowLink::addPeer(const uint8_t mac[6]) const {
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = channel_;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  return esp_now_add_peer(&peer) == ESP_OK;
}

bool EspNowLink::send(const uint8_t* data, const size_t len) {
  return esp_now_send(nullptr, data, len) == ESP_OK;
}

void EspNowLink::onReceive(const ReceiveCallback cb) {
  g_user_cb = cb;
  esp_now_register_recv_cb(RecvTrampoline);
}
