#include "yeelight_ble_remote.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

#include <cstring>
#include "mbedtls/ccm.h"

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::yeelight_ble_remote {

ESPHOME_LOG_TAG(TAG, "yeelight_ble_remote");

static constexpr size_t MAX_HEX_DUMP_BYTES = 100;

static constexpr uint16_t PRODUCT_ID_YLYK01YL = 0x0153;
static constexpr uint16_t PRODUCT_ID_YLYK01YL_FANCL = 0x068E;
static constexpr uint16_t PRODUCT_ID_YLKG07YL = 0x03B6;

bool parse_mibeacon_message(const std::vector<uint8_t> &message, RemoteParseResult &result) {
  result.has_encryption = message[0] & 0x08;  // update encryption status
  if (result.has_encryption) {
    ESP_LOGVV(TAG, "parse_mibeacon_message(): payload is encrypted, stop reading message.");
    return false;
  }

  if ((message[1] >> 1) & 1) {
    ESP_LOGVV(TAG, "parse_mibeacon_message(): Discard register and bind request.");
    return false;
  }

  // Data point specs
  // Byte 0: type
  // Byte 1: fixed 0x10
  // Byte 2: length
  // Byte 3..3+len-1: data point value

  if (result.raw_offset < 0 || static_cast<size_t>(result.raw_offset) >= message.size()) {
    ESP_LOGVV(TAG, "parse_mibeacon_message(): raw_offset (%d) exceeds message size (%zu)!", result.raw_offset,
              message.size());
    return false;
  }
  const uint8_t *payload = message.data() + result.raw_offset;
  uint8_t payload_length = message.size() - result.raw_offset;
  uint8_t payload_offset = 0;
  bool success = false;

  if (payload_length < 4) {
    ESP_LOGVV(TAG, "parse_mibeacon_message(): payload has wrong size (%d)!", payload_length);
    return false;
  }

  while (payload_length > 3) {
    if (payload[payload_offset + 1] != 0x10 && payload[payload_offset + 1] != 0x00 &&
        payload[payload_offset + 1] != 0x4C && payload[payload_offset + 1] != 0x48) {
      ESP_LOGVV(TAG, "parse_mibeacon_message(): fixed byte not found, stop parsing residual data.");
      break;
    }

    const uint8_t value_length = payload[payload_offset + 2];
    if ((value_length < 1) || (value_length > 4) || (payload_length < (3 + value_length))) {
      ESP_LOGVV(TAG, "parse_mibeacon_message(): value has wrong size (%d)!", value_length);
      break;
    }

    const uint16_t value_type = encode_uint16(payload[payload_offset + 1], payload[payload_offset + 0]);
    const uint8_t *data = &payload[payload_offset + 3];

    // button event, 3 bytes: keycode, encoder value, action type
    if ((value_type == 0x1001) && (value_length == 3)) {
      result.keycode = data[0];
      result.encoder_value = data[1];
      result.action_type = data[2];
      result.is_long_press = data[2] == 2;
      success = true;
    }

    payload_length -= 3 + value_length;
    payload_offset += 3 + value_length;
  }

  return success;
}

optional<RemoteParseResult> parse_mibeacon_header(const esp32_ble_tracker::ServiceData &service_data) {
  RemoteParseResult result{};
  if (!service_data.uuid.contains(0x95, 0xFE)) {
    ESP_LOGVV(TAG, "parse_mibeacon_header(): no service data UUID magic bytes.");
    return {};
  }

  const auto &raw = service_data.data;
  if (raw.size() < 5) {
    ESP_LOGVV(TAG, "parse_mibeacon_header(): service data too short (%zu).", raw.size());
    return {};
  }

  if (!(raw[0] & 0x40)) {
    ESP_LOGVV(TAG, "parse_mibeacon_header(): service data has no DATA flag.");
    return {};
  }

  result.has_encryption = raw[0] & 0x08;
  result.frame_count = raw[4];
  result.raw_offset = (raw[0] & 0x20) ? 12 : 11;

  const uint16_t product_id = encode_uint16(raw[3], raw[2]);
  if (product_id == PRODUCT_ID_YLYK01YL) {  // Yeelight Remote Control YLYK01YL
    result.type = RemoteParseResult::TYPE_YLYK01YL;
    result.name = "YLYK01YL";
  } else if (product_id == PRODUCT_ID_YLYK01YL_FANCL) {  // Yeelight Remote Control YLYK01YL Ceiling Fan
    result.type = RemoteParseResult::TYPE_YLYK01YL_FANCL;
    result.name = "YLYK01YL-FANCL";
  } else if (product_id == PRODUCT_ID_YLKG07YL) {  // Yeelight Wireless Smart Dimmer YLKG07YL/YLKG08YL
    result.type = RemoteParseResult::TYPE_YLKG07YL;
    result.name = "YLKG07YL";
  } else {
    ESP_LOGVV(TAG, "parse_mibeacon_header(): unknown product id 0x%04X.", product_id);
    return {};
  }

  return result;
}

// Decrypt MiBeacon V4/V5 payload
bool decrypt_mibeacon_v45(std::vector<uint8_t> &raw, const uint8_t *bindkey, uint64_t address) {
  // Determine cipher position based on packet structure
  size_t cipher_pos = 5;
  if (raw[0] & 0x10)  // has MAC address
    cipher_pos += 6;
  if (raw[0] & 0x20)  // has capability
    cipher_pos += 1;

  // ciphertext + payload counter (3 bytes) + tag (4 bytes)
  if (raw.size() < cipher_pos + 7 + 1 || raw.size() > cipher_pos + 7 + 16) {
    ESP_LOGVV(TAG, "decrypt_mibeacon_v45(): data packet has wrong size (%zu)!", raw.size());
    char hex_buf[format_hex_pretty_size(MAX_HEX_DUMP_BYTES)];
    ESP_LOGVV(TAG, "  Packet : %s", format_hex_pretty_to(hex_buf, raw, '.'));
    return false;
  }

  uint8_t mibeacon_version = (raw[0] >> 4) & 0x0F;
  ESP_LOGVV(TAG, "decrypt_mibeacon_v45(): detected MiBeacon V%d", mibeacon_version);

  uint8_t mac_reverse[6] = {0};
  mac_reverse[5] = (uint8_t) (address >> 40);
  mac_reverse[4] = (uint8_t) (address >> 32);
  mac_reverse[3] = (uint8_t) (address >> 24);
  mac_reverse[2] = (uint8_t) (address >> 16);
  mac_reverse[1] = (uint8_t) (address >> 8);
  mac_reverse[0] = (uint8_t) (address >> 0);

  AESVector vector{.key = {0},
                   .plaintext = {0},
                   .ciphertext = {0},
                   .authdata = {0x11},
                   .iv = {0},
                   .tag = {0},
                   .keysize = 16,
                   .authsize = 1,
                   .datasize = 0,
                   .tagsize = 4,
                   .ivsize = 12};

  vector.datasize = raw.size() - cipher_pos - 7;

  const uint8_t *v = raw.data();

  memcpy(vector.key, bindkey, vector.keysize);
  memcpy(vector.ciphertext, v + cipher_pos, vector.datasize);
  memcpy(vector.tag, v + raw.size() - vector.tagsize, vector.tagsize);
  memcpy(vector.iv, mac_reverse, 6);             // MAC address reverse
  memcpy(vector.iv + 6, v + 2, 3);               // sensor type (2) + packet id (1)
  memcpy(vector.iv + 9, v + raw.size() - 7, 3);  // payload counter

  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);

  int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, vector.key, vector.keysize * 8);
  if (ret) {
    ESP_LOGVV(TAG, "decrypt_mibeacon_v45(): mbedtls_ccm_setkey() failed.");
    mbedtls_ccm_free(&ctx);
    return false;
  }

  ret = mbedtls_ccm_auth_decrypt(&ctx, vector.datasize, vector.iv, vector.ivsize, vector.authdata, vector.authsize,
                                 vector.ciphertext, vector.plaintext, vector.tag, vector.tagsize);
  mbedtls_ccm_free(&ctx);
  if (ret) {
    ESP_LOGVV(TAG, "decrypt_mibeacon_v45(): authenticated decryption failed.");
    char hex_buf[format_hex_pretty_size(MAX_HEX_DUMP_BYTES)];
    ESP_LOGVV(TAG, "       Packet : %s", format_hex_pretty_to(hex_buf, raw, '.'));
    ESP_LOGVV(TAG, "           Iv : %s", format_hex_pretty_to(hex_buf, vector.iv, vector.ivsize, '.'));
    ESP_LOGVV(TAG, "       Cipher : %s", format_hex_pretty_to(hex_buf, vector.ciphertext, vector.datasize, '.'));
    ESP_LOGVV(TAG, "          Tag : %s", format_hex_pretty_to(hex_buf, vector.tag, vector.tagsize, '.'));
    return false;
  }

  // replace encrypted payload with plaintext
  memcpy(raw.data() + cipher_pos, vector.plaintext, vector.datasize);

  // clear encrypted flag
  raw[0] &= ~0x08;

  ESP_LOGVV(TAG, "decrypt_mibeacon_v45(): authenticated decryption passed.");
  char hex_buf[format_hex_pretty_size(MAX_HEX_DUMP_BYTES)];
  ESP_LOGVV(TAG, "  Plaintext : %s, Packet : %d",
            format_hex_pretty_to(hex_buf, raw.data() + cipher_pos, vector.datasize, '.'), static_cast<int>(raw[4]));

  return true;
}

void report_results(const RemoteParseResult &result, const std::string &address) {
  ESP_LOGD(TAG, "Got Yeelight %s (%s):", result.name, address.c_str());

  if (result.keycode.has_value()) {
    ESP_LOGD(TAG, "  Keycode: %u", *result.keycode);
  }
  if (result.encoder_value.has_value()) {
    ESP_LOGD(TAG, "  Encoder value: %d", *result.encoder_value);
  }
  if (result.action_type.has_value()) {
    ESP_LOGD(TAG, "  Action type: %d", *result.action_type);
  }
}

}  // namespace esphome::yeelight_ble_remote

#endif
