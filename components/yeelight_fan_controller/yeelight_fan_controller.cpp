#include "yeelight_fan_controller.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace yeelight_fan_controller {

static const char *const TAG = "yeelight_fan_controller";

static const uint8_t FAN_PKT_START = 0x01;
static const uint8_t FAN_PKT_END = 0x03;
static const uint8_t FAN_PKT_RESPONSE = 0xF3;
static const uint8_t FAN_PKT_INVALID = 0xA0;

void YeelightFanController::loop() {
  const uint32_t now = millis();

  if (now - this->last_byte_ > 50) {
    this->rx_buffer_.clear();
    this->last_byte_ = now;
  }

  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    if (this->parse_yeelight_fan_controller_byte_(byte)) {
      this->last_byte_ = now;
    } else {
      this->rx_buffer_.clear();
    }
  }
}

bool YeelightFanController::parse_yeelight_fan_controller_byte_(uint8_t byte) {
  size_t at = this->rx_buffer_.size();
  this->rx_buffer_.push_back(byte);
  const uint8_t *raw = &this->rx_buffer_[0];
  uint8_t frame_len = 6;

  // Example traffic
  //
  // turn on
  // >>> 01:04:01:18:13:03
  // <<< 01:F3:01:07:13:03

  // turn off
  // >>> 01:01:01:13:11:03
  // <<< 01:F3:01:05:11:03

  // 1 level / fan speed 1%
  // >>> 01:03:01:05:01:03
  // <<< 01:F3:01:F5:01:03

  // 2 level / fan speed 50%
  // >>> 01:03:01:36:32:03
  // <<< 01:F3:01:26:32:03

  // 3 level / fan speed 100%
  // >>> 01:03:01:68:64:03
  // <<< 01:F3:01:58:64:03

  // Byte Len  Payload                Content              Coeff.      Unit
  // Example value 0     1   0x01                   Start of frame 1     1 0xF3
  // Frame type 2     1   0x01                   Unknown 3     1   0x07 Checksum
  // 4     1   0x13                   Fan speed
  // 5     1   0x03                   End of frame

  if (at == 0)
    return true;

  if (raw[0] != FAN_PKT_START) {
    ESP_LOGW(TAG, "Invalid header");

    // return false to reset buffer
    return false;
  }

  if (at < frame_len - 1)
    return true;

  uint8_t computed_crc = raw[1] + raw[2] + raw[4];
  uint8_t remote_crc = raw[3];
  if (computed_crc != remote_crc) {
    ESP_LOGW(TAG, "YeelightFanController CRC Check failed! %04X != %04X", computed_crc, remote_crc);
    return false;
  }

  ESP_LOGVV(TAG, "RX <- %s", format_hex_pretty(raw, at + 1).c_str());

  std::vector<uint8_t> data(this->rx_buffer_.begin(), this->rx_buffer_.begin() + frame_len);

  this->on_yeelight_fan_controller_data_(data);

  // return false to reset buffer
  return false;
}

void YeelightFanController::on_yeelight_fan_controller_data_(const std::vector<uint8_t> &data) {
  if (data[1] == FAN_PKT_RESPONSE) {
    if (data[4] == FAN_PKT_INVALID) {
      ESP_LOGE(TAG, "Command failed");
      return;
    }

    ESP_LOGI(TAG, "Command successful");
    return;
  }

  ESP_LOGW(TAG, "Unhandled response received: %s", format_hex_pretty(&data.front(), data.size()).c_str());
}

void YeelightFanController::dump_config() {  // NOLINT(google-readability-function-size,readability-function-size)
  ESP_LOGCONFIG(TAG, "YeelightFanController:");
}

float YeelightFanController::get_setup_priority() const {
  // After UART bus
  return setup_priority::BUS - 1.0f;
}

void YeelightFanController::send_command(uint8_t function, uint8_t speed) {
  uint8_t frame[6];

  // Payload                          Description
  // 0x01 0x04 0x01 0x18 0x13 0x03    Turn on
  // 0x01 0x01 0x01 0x13 0x11 0x03    Turn off
  // 0x01 0x03 0x01 0x05 0x01 0x03    Fan speed 1%
  // 0x01 0x03 0x01 0x36 0x32 0x03    Fan speed 50%
  // 0x01 0x03 0x01 0x68 0x64 0x03    Fan speed 100%

  frame[0] = FAN_PKT_START;
  frame[1] = function;
  frame[2] = 0x01;
  frame[3] = 0x00;
  frame[4] = speed;
  frame[5] = FAN_PKT_END;

  frame[3] = frame[1] + frame[2] + frame[4];

  this->write_array(frame, 6);
  this->flush();

  delay(55);
}

}  // namespace yeelight_fan_controller
}  // namespace esphome
