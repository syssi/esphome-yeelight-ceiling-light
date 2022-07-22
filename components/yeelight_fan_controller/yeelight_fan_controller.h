#pragma once

#include "esphome/components/fan/fan.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace yeelight_fan_controller {

class YeelightFanController : public uart::UARTDevice, public Component {
public:
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void send_command(uint8_t function, uint8_t speed);

protected:
  std::vector<uint8_t> rx_buffer_;
  uint32_t last_byte_{0};
  uint32_t last_send_{0};

  void on_yeelight_fan_controller_data_(const std::vector<uint8_t> &data);
  bool parse_yeelight_fan_controller_byte_(uint8_t byte);
};

} // namespace yeelight_fan_controller
} // namespace esphome
