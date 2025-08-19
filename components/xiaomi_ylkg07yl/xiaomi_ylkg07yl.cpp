#include "xiaomi_ylkg07yl.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "mbedtls/ccm.h"
#include <vector>

#ifdef USE_ESP32

namespace esphome {
namespace xiaomi_ylkg07yl {

static const char *const TAG = "xiaomi_ylkg07yl";

void XiaomiYLKG07YL::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi YLKG07YL");
  ESP_LOGCONFIG(TAG, "  Bindkey: %s", format_hex_pretty(this->bindkey_, 12).c_str());
  LOG_SENSOR("  ", "Keycode", this->keycode_sensor_);
  LOG_SENSOR("  ", "Encoder value", this->encoder_value_sensor_);
  LOG_SENSOR("  ", "Action type", this->action_type_sensor_);
}

bool XiaomiYLKG07YL::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.address_uint64() != this->address_) {
    ESP_LOGVV(TAG, "parse_device(): unknown MAC address.");
    return false;
  }
  ESP_LOGVV(TAG, "parse_device(): MAC address %s found.", device.address_str().c_str());

  bool success = false;
  for (auto &service_data : device.get_service_datas()) {
    auto res = xiaomi_ble::parse_xiaomi_header(service_data);
    if (!res.has_value()) {
      continue;
    }
    if (res->is_duplicate) {
      continue;
    }
    if (res->has_encryption) {
      if (!(this->decrypt_mibeacon_v23_(const_cast<std::vector<uint8_t> &>(service_data.data), this->bindkey_,
                                        this->address_))) {
        // Try v4/v5 decryption if legacy fails - pad 12-byte key with 0xFF to 16 bytes
        uint8_t padded_key[16];
        memcpy(padded_key, this->bindkey_, 12);
        memset(padded_key + 12, 0xFF, 4);
        if (!(xiaomi_ble::decrypt_xiaomi_payload(const_cast<std::vector<uint8_t> &>(service_data.data), padded_key,
                                                 this->address_))) {
          continue;
        }
        // Adjust raw_offset for V4 frame parsing - encrypted payload starts at byte 5
        res->raw_offset = 5;
      }
    }
    if (!(xiaomi_ble::parse_xiaomi_message(service_data.data, *res))) {
      continue;
    }
    if (!(xiaomi_ble::report_xiaomi_results(res, device.address_str()))) {
      continue;
    }
    if (res->keycode.has_value()) {
      if(res->encoder_value.has_value())
          res->encoder_value = ((res->encoder_value.value() + 128) % 256 - 128);

      if (this->keycode_sensor_ != nullptr)
        this->keycode_sensor_->publish_state(*res->keycode);

      if (this->encoder_value_sensor_ != nullptr)
        this->encoder_value_sensor_->publish_state(*res->encoder_value);

      if (this->action_type_sensor_ != nullptr)
        this->action_type_sensor_->publish_state(*res->action_type);

      this->receive_callback_.call(*res->keycode, *res->encoder_value, *res->action_type);
    }
    success = true;
  }

  return success;
}

void XiaomiYLKG07YL::set_bindkey(const std::string &bindkey) {
  memset(bindkey_, 0, 12);
  if (bindkey.size() != 24) {
    return;
  }
  char temp[3] = {0};
  for (int i = 0; i < 12; i++) {
    strncpy(temp, &(bindkey.c_str()[i * 2]), 2);
    bindkey_[i] = std::strtoul(temp, nullptr, 16);
  }
}

void XiaomiYLKG07YL::add_on_receive_callback(std::function<void(int, int, int)> &&callback) {
  this->receive_callback_.add(std::move(callback));
}

// Decrypt MiBeacon V2/V3 payload
bool XiaomiYLKG07YL::decrypt_mibeacon_v23_(std::vector<uint8_t> &raw, const uint8_t *bindkey, const uint64_t &address) {
  if (raw.size() != 21) {
    ESP_LOGVV(TAG, "decrypt_mibeacon_v23_(): data packet has wrong size (%d)!", raw.size());
    ESP_LOGVV(TAG, "  Packet : %s", format_hex_pretty(raw.data(), raw.size()).c_str());
    return false;
  }

  uint8_t mac_reverse[6] = {0};
  mac_reverse[5] = (uint8_t) (address >> 40);
  mac_reverse[4] = (uint8_t) (address >> 32);
  mac_reverse[3] = (uint8_t) (address >> 24);
  mac_reverse[2] = (uint8_t) (address >> 16);
  mac_reverse[1] = (uint8_t) (address >> 8);
  mac_reverse[0] = (uint8_t) (address >> 0);

  xiaomi_ble::XiaomiAESVector vector{.key = {0},
                                     .plaintext = {0},
                                     .ciphertext = {0},
                                     .authdata = {0x11},
                                     .iv = {0},
                                     .tag = {0},
                                     .keysize = 16,
                                     .authsize = 1,
                                     .datasize = 0,
                                     .tagsize = 4,
                                     .ivsize = 13};

  vector.datasize = raw.size() - 15;
  int cipher_pos = 11;

  const uint8_t *v = raw.data();

  // key format is: bindkey[:6] "8d3d3c97" bindkey[6:]
  uint8_t key[16] = {0, 0, 0, 0, 0, 0, 0x8d, 0x3d, 0x3c, 0x97, 0, 0, 0, 0, 0, 0};
  memcpy(key + 0x0, bindkey + 0, 6);
  memcpy(key + 0xA, bindkey + 6, 6);

  memcpy(vector.key, key, vector.keysize);
  memcpy(vector.ciphertext, v + cipher_pos, vector.datasize);

  // 58.30.B6.03.01.BC.7B.C4.41.24.F8.5A.B8.4A.60.93.B8.01.00.00.21 (21)
  //                                  ^^^^^^^^^^^^^^^^^^^^^^^^^^
  //
  // 58.30.               Frame control
  // B6.03.               Device type
  // 01.                  Frame count
  // BC.7B.C4.41.24.F8.   MAC address
  // 5A.B8.4A.60.93.B8.   Cipher
  // 01.00.00.            Ext. count
  // 21

  // nonce = b"".join(
  //     [
  //         self.framectrl_data,
  //         self.device_type,
  //         payload_counter,
  //         self.xiaomi_mac_reversed[:-1]
  //    ]
  // )

  // 58.30.B6.03.01.BC.7B.C4.41.24.F8.5A.B8.4A.60.93.B8.01.00.00.21 (21)
  // ^^^^^^^^^^^^^^
  memcpy(vector.iv, v, 5);  // frame control (2 bytes) + device type (2 bytes) +
                            // frame count (1 byte)

  // 58.30.B6.03.01.BC.7B.C4.41.24.F8.5A.B8.4A.60.93.B8.01.00.00.21 (21)
  //                                                    ^^^^^^^^
  memcpy(vector.iv + 5, v + raw.size() - 4, 3);  // ext. count

  // 58.30.B6.03.01.BC.7B.C4.41.24.F8.5A.B8.4A.60.93.B8.01.00.00.21 (21)
  //                ^^^^^^^^^^^^^^
  memcpy(vector.iv + 8, mac_reverse, 5);  // 5 bytes of the reversed MAC address

  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);

  int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, vector.key, vector.keysize * 8);
  if (ret) {
    ESP_LOGVV(TAG, "decrypt_mibeacon_v23_(): mbedtls_ccm_setkey() failed.");
    mbedtls_ccm_free(&ctx);
    return false;
  }

  ret = mbedtls_ccm_star_auth_decrypt(&ctx, vector.datasize, vector.iv, vector.ivsize, vector.authdata, vector.authsize,
                                      vector.ciphertext, vector.plaintext, nullptr, 0);
  if (ret) {
    uint8_t mac_address[6] = {0};
    memcpy(mac_address, mac_reverse + 5, 1);
    memcpy(mac_address + 1, mac_reverse + 4, 1);
    memcpy(mac_address + 2, mac_reverse + 3, 1);
    memcpy(mac_address + 3, mac_reverse + 2, 1);
    memcpy(mac_address + 4, mac_reverse + 1, 1);
    memcpy(mac_address + 5, mac_reverse, 1);
    ESP_LOGVV(TAG, "decrypt_mibeacon_v23_(): mbedtls_ccm_star_auth_decrypt failed.");
    ESP_LOGVV(TAG, "  MAC address : %s", format_hex_pretty(mac_address, 6).c_str());
    ESP_LOGVV(TAG, "       Packet : %s", format_hex_pretty(raw.data(), raw.size()).c_str());
    ESP_LOGVV(TAG, "          Key : %s", format_hex_pretty(vector.key, vector.keysize).c_str());
    ESP_LOGVV(TAG, "           Iv : %s", format_hex_pretty(vector.iv, vector.ivsize).c_str());
    ESP_LOGVV(TAG, "       Cipher : %s", format_hex_pretty(vector.ciphertext, vector.datasize).c_str());
    mbedtls_ccm_free(&ctx);
    return false;
  }

  // replace encrypted payload with plaintext
  uint8_t *p = vector.plaintext;
  for (std::vector<uint8_t>::iterator it = raw.begin() + cipher_pos; it != raw.begin() + cipher_pos + vector.datasize;
       ++it) {
    *it = *(p++);
  }

  // clear encrypted flag
  raw[0] &= ~0x08;

  ESP_LOGVV(TAG, "decrypt_mibeacon_v23_(): authenticated decryption passed.");
  ESP_LOGVV(TAG, "  Plaintext : %s, Packet : %d", format_hex_pretty(raw.data() + cipher_pos, vector.datasize).c_str(),
            static_cast<int>(raw[4]));

  mbedtls_ccm_free(&ctx);
  return true;
}

}  // namespace xiaomi_ylkg07yl
}  // namespace esphome

#endif
