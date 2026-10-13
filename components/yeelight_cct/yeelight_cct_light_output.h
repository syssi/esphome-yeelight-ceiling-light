#pragma once

#include <algorithm>
#include <cmath>
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
  void set_min_duty(float min_duty) { this->min_duty_ = min_duty; }

  /// Report a colour-temperature-only light whose range is the first to the last table point.
  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
    // Mireds are 1e6 / kelvin, so the highest kelvin gives the lowest mired value.
    traits.set_min_mireds(1e6f / this->table_.back().kelvin);
    traits.set_max_mireds(1e6f / this->table_.front().kelvin);
    return traits;
  }

  /// Compute the warm and cold duty for the current colour temperature and brightness.
  /// ESPHome calls this on every state change and on every step of a transition.
  void write_state(light::LightState *state) override {
    // Brightness 0..1, already 0 when the light is off.
    float brightness;
    state->current_values_as_brightness(&brightness);
    if (brightness <= 0.0f) {
      this->warm_white_->set_level(0.0f);
      this->cold_white_->set_level(0.0f);
      return;
    }
    const auto &table = this->table_;
    const size_t last = table.size() - 1;
    const bool quadratic = this->interpolation_ == Interpolation::QUADRATIC;

    // The stock firmware works in whole kelvin. Rounding also keeps e.g. 3000 K, which arrives
    // as 2999.99 K after the mired round trip, from falling into the segment below.
    float t = std::round(1e6f / state->current_values.get_color_temperature());
    t = std::min(std::max(t, float(table[0].kelvin)), float(table[last].kelvin));
    // Quadratic mode: everything within 100 K of the last point is the last point.
    if (quadratic && t + 100.0f > table[last].kelvin - 1.0f)
      t = table[last].kelvin;

    // Segment i with K[i] <= t < K[i + 1]; the last point forms its own segment.
    size_t i = 0;
    while (i < last && t >= table[i + 1].kelvin)
      i++;
    // Without interpolation the lower point of the segment is the result.
    float w = table[i].warm;
    float c = table[i].cold;

    // Segments touching the first or last point are not interpolated: those points are pure
    // single-channel white, and between them and their neighbour one channel's share would lie
    // below the minimum share. The three-point window moves down one point so it never reaches
    // the last point.
    if (quadratic && i >= 1 && i + 1 < last) {
      const size_t u = (i + 2 == last) ? i - 1 : i;
      const MixPoint &p0 = table[u], &p1 = table[u + 1], &p2 = table[u + 2];
      float wl = std::min(lagrange(t, p0.kelvin, p0.warm, p1.kelvin, p1.warm, p2.kelvin, p2.warm), 1.0f);
      float cl = std::min(lagrange(t, p0.kelvin, p0.cold, p1.kelvin, p1.cold, p2.kelvin, p2.cold), 1.0f);
      // The stock firmware keeps the table point if the curve dips below the minimum share.
      if (wl >= this->min_share_ && cl >= this->min_share_) {
        w = wl;
        c = cl;
      }
    }
    if (this->interpolation_ == Interpolation::LINEAR && i < last) {
      const MixPoint &p0 = table[i], &p1 = table[i + 1];
      float a = (t - p0.kelvin) / (p1.kelvin - p0.kelvin);
      w = p0.warm + a * (p1.warm - p0.warm);
      c = p0.cold + a * (p1.cold - p0.cold);
    }

    // Brightness 1..100 % maps to min_brightness..100 % output. The stock firmware works in
    // whole percent; fractions are kept here for smooth transitions.
    float b = std::min(std::max(brightness, 0.01f), 1.0f);
    float f;
    if (this->brightness_curve_ == BrightnessCurve::QUADRATIC) {
      // Stock curve in percent: x = (b + k) / (100 + k) with k = max(m, 15), and x(1 %) maps to m.
      float m = this->min_brightness_ * 100.0f;
      float k = std::max(m, 15.0f);
      float x = (b * 100.0f + k) / (100.0f + k);
      float y = (1.0f + k) / (100.0f + k);
      f = (m + (100.0f - m) * (x * x - y * y) / (1.0f - y * y)) / 100.0f;
    } else {
      f = this->min_brightness_ + (b - 0.01f) * (1.0f - this->min_brightness_) / 0.99f;
    }
    // Share of each channel scaled by the brightness factor.
    float ow = w * f;
    float oc = c * f;
    // Every lit channel is offset by the stock minimum duty.
    this->warm_white_->set_level(ow > 0.0f ? this->min_duty_ + (1.0f - this->min_duty_) * ow : 0.0f);
    this->cold_white_->set_level(oc > 0.0f ? this->min_duty_ + (1.0f - this->min_duty_) * oc : 0.0f);
  }

 protected:
  /// One calibration point: warm and cold share of full scale (0..1) at a colour temperature.
  struct MixPoint {
    uint16_t kelvin;
    float warm;
    float cold;
  };

  /// Value at x of the parabola through (x0, y0), (x1, y1) and (x2, y2).
  static float lagrange(float x, float x0, float y0, float x1, float y1, float x2, float y2) {
    return y0 * (x - x1) * (x - x2) / ((x0 - x1) * (x0 - x2)) + y1 * (x - x0) * (x - x2) / ((x1 - x0) * (x1 - x2)) +
           y2 * (x - x0) * (x - x1) / ((x2 - x0) * (x2 - x1));
  }

  output::FloatOutput *warm_white_;
  output::FloatOutput *cold_white_;
  std::vector<MixPoint> table_;  // sorted by rising kelvin
  Interpolation interpolation_{Interpolation::QUADRATIC};
  BrightnessCurve brightness_curve_{BrightnessCurve::LINEAR};
  float min_share_{0.0f};       // quadratic: keep the table point if a share drops below this
  float min_brightness_{0.0f};  // output factor at 1 % brightness
  float min_duty_{0.0f};        // duty offset of every lit channel
};

}  // namespace esphome::yeelight_cct
