#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include "../yeelight_front_panel.h"

namespace esphome::yeelight_front_panel {

/// Publishes the slider position as a level, counting from 1 at the end of the
/// slider closest to the power button, up to SLIDER_LEVELS.
class FrontPanelSliderSensor : public sensor::Sensor, public Component {
 public:
  void set_parent(YeelightFrontPanel *parent) { this->parent_ = parent; }

  void setup() override {
    this->parent_->add_on_event_callback([this](const FrontPanelEvent &event) {
      if (event.part == FrontPanelPart::SLIDER && event.slider_level > 0)
        this->publish_state(event.slider_level);
    });
  }

  void dump_config() override;

 protected:
  YeelightFrontPanel *parent_{nullptr};
};

}  // namespace esphome::yeelight_front_panel
