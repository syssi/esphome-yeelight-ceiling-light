#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/component.h"

#include "../yeelight_front_panel.h"

namespace esphome {
namespace yeelight_front_panel {

/// Reports whether one element of the front panel is currently being touched.
class FrontPanelBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_parent(YeelightFrontPanel *parent) { this->parent_ = parent; }
  void set_part(FrontPanelPart part) { this->part_ = part; }

  void setup() override {
    this->publish_initial_state(false);
    this->parent_->add_on_event_callback([this](const FrontPanelEvent &event) {
      if (event.part != this->part_)
        return;
      if (event.action == FrontPanelAction::TOUCH) {
        this->publish_state(true);
      } else if (event.action == FrontPanelAction::RELEASE) {
        this->publish_state(false);
      }
    });
  }

  void dump_config() override;

 protected:
  YeelightFrontPanel *parent_{nullptr};
  FrontPanelPart part_{FrontPanelPart::UNKNOWN};
};

}  // namespace yeelight_front_panel
}  // namespace esphome
