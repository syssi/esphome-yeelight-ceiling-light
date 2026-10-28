#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/xiaomi_ble/xiaomi_ble.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome::xiaomi_ylkg07yl {

static const uint8_t KEYCODE_BUTTON = 0;

static const uint8_t ACTION_TYPE_PRESS = 3;
static const uint8_t ACTION_TYPE_ROTATE = 4;

class XiaomiYLKG07YL final : public Component, public xiaomi_ble::XiaomiEncryptedDevice {
 public:
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
  bool decrypt_payload(uint8_t *raw, size_t size, const uint8_t *bindkey) override;
  void publish_result(const xiaomi_ble::XiaomiParseResult &res) override;

  sensor::Sensor *keycode_sensor_{nullptr};
  sensor::Sensor *encoder_value_sensor_{nullptr};
  sensor::Sensor *action_type_sensor_{nullptr};
  CallbackManager<void(int)> press_callback_{};
  CallbackManager<void(int)> rotate_callback_{};
  CallbackManager<void(int)> press_and_rotate_callback_{};
};

}  // namespace esphome::xiaomi_ylkg07yl
