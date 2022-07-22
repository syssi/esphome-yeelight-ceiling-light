#pragma once

#include "../yeelight_fan_controller.h"
#include "esphome/components/fan/fan.h"

namespace esphome {
namespace yeelight_fan_controller {

class YeelightFanController;
class YeelightFan : public fan::Fan, public Component {
public:
  YeelightFan(YeelightFanController *parent) : parent_(parent) {}
  void setup() override;
  void dump_config() override;
  fan::FanTraits get_traits() override;

protected:
  void control(const fan::FanCall &call) override;
  void write_state_();

  bool last_state_{false};
  YeelightFanController *parent_;
};

} // namespace yeelight_fan_controller
} // namespace esphome
