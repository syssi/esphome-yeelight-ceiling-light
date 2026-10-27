#pragma once

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/optional.h"

#include <cstdint>
#include <string>
#include <vector>

#ifdef USE_ESP32

namespace esphome::yeelight_ble_remote {

struct RemoteParseResult {
  enum { TYPE_YLYK01YL, TYPE_YLYK01YL_FANCL, TYPE_YLKG07YL } type;
  const char *name;
  optional<uint8_t> keycode;
  optional<int> encoder_value;
  optional<int> action_type;
  optional<bool> is_long_press;
  bool has_encryption;  // 0x08
  uint8_t frame_count;
  int raw_offset;
};

struct AESVector {
  uint8_t key[16];
  uint8_t plaintext[16];
  uint8_t ciphertext[16];
  uint8_t authdata[16];
  uint8_t iv[16];
  uint8_t tag[16];
  size_t keysize;
  size_t authsize;
  size_t datasize;
  size_t tagsize;
  size_t ivsize;
};

/// Parses the MiBeacon frame control and product id. Returns nothing for frames of other devices.
optional<RemoteParseResult> parse_mibeacon_header(const esp32_ble_tracker::ServiceData &service_data);
/// Parses the (plaintext) data points of a MiBeacon frame. Returns true if a button event was found.
bool parse_mibeacon_message(const std::vector<uint8_t> &message, RemoteParseResult &result);
/// Decrypts a MiBeacon V4/V5 payload in place and clears its encryption flag.
bool decrypt_mibeacon_v45(std::vector<uint8_t> &raw, const uint8_t *bindkey, uint64_t address);
void report_results(const RemoteParseResult &result, const std::string &address);

}  // namespace esphome::yeelight_ble_remote

#endif
