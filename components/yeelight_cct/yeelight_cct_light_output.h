#pragma once

#include <vector>

#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/output/float_output.h"

namespace esphome::yeelight_cct {

/// How the stock firmware mixes between two calibration points.
enum class Interpolation : uint8_t {
  /// Use the lower point of the segment (stock firmware 1.3.x).
  NONE,
  /// Quadratic Lagrange interpolation with the stock edge rules (stock firmware 2.x).
  QUADRATIC,
  /// Linear interpolation in kelvin between two points (ceil49).
  LINEAR,
};

/// How the stock firmware maps brightness 1..100 % to the output factor min_brightness..100 %.
enum class BrightnessCurve : uint8_t {
  /// Straight line (ceiling lights).
  LINEAR,
  /// Quadratic curve that stays darker at low brightness (lamp2, lamp4, lamp7, lamp9).
  QUADRATIC,
};

/// Colour temperature light that drives the warm and cold channel like the Yeelight stock firmware.
class YeelightCCTLightOutput : public light::LightOutput {
 public:
  // Setters called by the code generated from light.py.
  void set_warm_white(output::FloatOutput *warm_white) { this->warm_white_ = warm_white; }
  void set_cold_white(output::FloatOutput *cold_white) { this->cold_white_ = cold_white; }
  void add_point(uint16_t kelvin, float warm, float cold) { this->table_.push_back({kelvin, warm, cold}); }
  void set_interpolation(Interpolation interpolation) { this->interpolation_ = interpolation; }
  void set_brightness_curve(BrightnessCurve brightness_curve) { this->brightness_curve_ = brightness_curve; }
  void set_min_share(float min_share) { this->min_share_ = min_share; }
  void set_min_brightness(float min_brightness) { this->min_brightness_ = min_brightness; }

  /// Report a colour-temperature-only light whose range is the first to the last table point.
  light::LightTraits get_traits() override;

  /// Compute the warm and cold level for the current colour temperature and brightness.
  /// ESPHome calls this on every state change and on every step of a transition.
  void write_state(light::LightState *state) override;

 protected:
  /// One calibration point: warm and cold share of full scale (0..1) at a colour temperature.
  struct MixPoint {
    uint16_t kelvin;
    float warm;
    float cold;
  };

  /// Value at x of the parabola through (x0, y0), (x1, y1) and (x2, y2).
  static float lagrange(float x, float x0, float y0, float x1, float y1, float x2, float y2);

  output::FloatOutput *warm_white_;
  output::FloatOutput *cold_white_;
  std::vector<MixPoint> table_;  // sorted by rising kelvin
  Interpolation interpolation_{Interpolation::QUADRATIC};
  BrightnessCurve brightness_curve_{BrightnessCurve::LINEAR};
  float min_share_{0.0f};       // quadratic: keep the table point if a share drops below this
  float min_brightness_{0.0f};  // output factor at 1 % brightness
};

}  // namespace esphome::yeelight_cct
