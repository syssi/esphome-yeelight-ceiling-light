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
  void add_on_receive_callback(std::function<void(int, int, int)> &&callback);

 protected:
  uint64_t address_;
  uint8_t bindkey_[12];
  sensor::Sensor *keycode_sensor_{nullptr};
  sensor::Sensor *encoder_value_sensor_{nullptr};
  sensor::Sensor *action_type_sensor_{nullptr};
  CallbackManager<void(int, int, int)> receive_callback_{};

  bool decrypt_mibeacon_v23_(std::vector<uint8_t> &raw, const uint8_t *bindkey, const uint64_t &address);
};

class OnPressTrigger : public Trigger<int> {
 public:
  OnPressTrigger(XiaomiYLKG07YL *a_remote) {
    a_remote->add_on_receive_callback([this](int keycode, int encoder_value, int action_type) {
      if (action_type == ACTION_TYPE_PRESS && keycode == KEYCODE_BUTTON) {
        this->trigger(encoder_value);
      }
    });
  }
};

class OnRotateTrigger : public Trigger<int> {
 public:
  OnRotateTrigger(XiaomiYLKG07YL *a_remote) {
    a_remote->add_on_receive_callback([this](int keycode, int encoder_value, int action_type) {
      if (action_type == ACTION_TYPE_ROTATE && keycode == KEYCODE_BUTTON) {

        this->trigger(encoder_value);
      }
    });
  }
};

class OnPressAndRotateTrigger : public Trigger<int> {
 public:
  OnPressAndRotateTrigger(XiaomiYLKG07YL *a_remote) {
    a_remote->add_on_receive_callback([this](int keycode, int encoder_value, int action_type) {
      if (action_type == ACTION_TYPE_ROTATE && keycode != KEYCODE_BUTTON) {

        this->trigger(keycode); //for press and rotate, encoder_value is a keycode
      }
    });
  }
};

}  // namespace xiaomi_ylkg07yl
}  // namespace esphome

#endif
