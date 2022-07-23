#pragma once

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/xiaomi_ble/xiaomi_ble.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

#ifdef USE_ESP32

namespace esphome {
namespace xiaomi_ylyk01yl {

// static const uint8_t BUTTON_ON = 0;
// static const uint8_t BUTTON_OFF = 1;
// static const uint8_t BUTTON_SUN = 2;
// static const uint8_t BUTTON_M = 4;
// static const uint8_t BUTTON_PLUS = 3;
// static const uint8_t BUTTON_MINUS = 5;

class XiaomiYLYK01YL : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void set_address(uint64_t address) { address_ = address; }

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_keycode(sensor::Sensor *keycode) { keycode_ = keycode; }
  void add_on_receive_callback(std::function<void(uint8_t, bool)> &&callback);

 protected:
  uint64_t address_;
  sensor::Sensor *keycode_{nullptr};
  CallbackManager<void(uint8_t, bool)> receive_callback_{};
};

class OnPressTrigger : public Trigger<uint8_t> {
 public:
  OnPressTrigger(XiaomiYLYK01YL *parent) {
    parent->add_on_receive_callback([this](uint8_t keycode, bool is_long_press) {
      if (!is_long_press) {
        if (this->keycode_ != 255) {
          if (this->keycode_ == keycode) {
            this->trigger(keycode);
          }
        } else {
          this->trigger(keycode);
        }
      }
    });
  }

  void set_keycode(uint8_t keycode) { this->keycode_ = keycode; }

 protected:
  uint8_t keycode_ = 255;
};

class OnLongPressTrigger : public Trigger<uint8_t> {
 public:
  OnLongPressTrigger(XiaomiYLYK01YL *parent) {
    parent->add_on_receive_callback([this](uint8_t keycode, bool is_long_press) {
      if (is_long_press) {
        if (this->keycode_ != 255) {
          if (this->keycode_ == keycode) {
            this->trigger(keycode);
          }
        } else {
          this->trigger(keycode);
        }
      }
    });
  }

  void set_keycode(uint8_t keycode) { this->keycode_ = keycode; }

 protected:
  uint8_t keycode_ = 255;
};

}  // namespace xiaomi_ylyk01yl
}  // namespace esphome

#endif
