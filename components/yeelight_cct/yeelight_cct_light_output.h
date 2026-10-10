#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/output/float_output.h"
#include "yeelight_cct_mix.h"

namespace esphome::yeelight_cct {

/// Colour temperature light that drives the warm and cold channel like the Yeelight stock firmware.
class YeelightCCTLightOutput : public light::LightOutput {
 public:
  void set_warm_white(output::FloatOutput *warm_white) { this->warm_white_ = warm_white; }
  void set_cold_white(output::FloatOutput *cold_white) { this->cold_white_ = cold_white; }
  void add_point(uint16_t kelvin, float warm, float cold) { this->params_.table.push_back({kelvin, warm, cold}); }
  void set_interpolation(Interpolation interpolation) { this->params_.interpolation = interpolation; }
  void set_min_share(float min_share) { this->params_.min_share = min_share; }
  void set_min_brightness(float min_brightness) { this->params_.min_brightness = min_brightness; }
  void set_min_duty(float min_duty) { this->params_.min_duty = min_duty; }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
    traits.set_min_mireds(1e6f / this->params_.table.back().kelvin);
    traits.set_max_mireds(1e6f / this->params_.table.front().kelvin);
    return traits;
  }

  void write_state(light::LightState *state) override {
    float brightness;
    state->current_values_as_brightness(&brightness);
    float kelvin = 1e6f / state->current_values.get_color_temperature();
    float warm, cold;
    mix(this->params_, kelvin, brightness, &warm, &cold);
    this->warm_white_->set_level(warm);
    this->cold_white_->set_level(cold);
  }

 protected:
  output::FloatOutput *warm_white_;
  output::FloatOutput *cold_white_;
  MixParams params_;
};

}  // namespace esphome::yeelight_cct
