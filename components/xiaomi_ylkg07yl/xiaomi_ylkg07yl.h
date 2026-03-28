#pragma once

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/xiaomi_ble/xiaomi_ble.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

#ifdef USE_ESP32

static const uint8_t KEYCODE_BUTTON = 0;

static const uint8_t ACTION_TYPE_PRESS = 3;
static const uint8_t ACTION_TYPE_ROTATE = 4;

namespace esphome {
namespace xiaomi_ylkg07yl {

class XiaomiYLKG07YL : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void set_address(uint64_t address) { address_ = address; }
  void set_bindkey(const std::string &bindkey);

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_keycode_sensor(sensor::Sensor *keycode_sensor) { keycode_sensor_ = keycode_sensor; }
  void set_encoder_value_sensor(sensor::Sensor *encoder_value_sensor) { encoder_value_sensor_ = encoder_value_sensor; }
  void set_action_type_sensor(sensor::Sensor *action_type_sensor) { action_type_sensor_ = action_type_sensor; }
  void add_on_press_callback(std::function<void(int)> &&callback) { this->press_callback_.add(std::move(callback)); }
  void add_on_rotate_callback(std::function<void(int)> &&callback) { this->rotate_callback_.add(std::move(callback)); }
  void add_on_press_and_rotate_callback(std::function<void(int)> &&callback) {
    this->press_and_rotate_callback_.add(std::move(callback));
  }

 protected:
  uint64_t address_;
  uint8_t bindkey_[12];
  sensor::Sensor *keycode_sensor_{nullptr};
  sensor::Sensor *encoder_value_sensor_{nullptr};
  sensor::Sensor *action_type_sensor_{nullptr};
  CallbackManager<void(int)> press_callback_{};
  CallbackManager<void(int)> rotate_callback_{};
  CallbackManager<void(int)> press_and_rotate_callback_{};

  bool decrypt_mibeacon_v23_(std::vector<uint8_t> &raw, const uint8_t *bindkey, const uint64_t &address);
};

}  // namespace xiaomi_ylkg07yl
}  // namespace esphome

#endif
