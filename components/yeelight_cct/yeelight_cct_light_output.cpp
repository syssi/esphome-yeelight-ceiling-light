#include "yeelight_cct_light_output.h"

#include <algorithm>
#include <cmath>

namespace esphome::yeelight_cct {

light::LightTraits YeelightCCTLightOutput::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
  // Mireds are 1e6 / kelvin, so the highest kelvin gives the lowest mired value.
  traits.set_min_mireds(1e6f / this->table_.back().kelvin);
  traits.set_max_mireds(1e6f / this->table_.front().kelvin);
  return traits;
}

void YeelightCCTLightOutput::write_state(light::LightState *state) {
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
  float color_temperature = std::round(1e6f / state->current_values.get_color_temperature());
  color_temperature = std::min(std::max(color_temperature, float(table[0].kelvin)), float(table[last].kelvin));
  // Quadratic mode: everything within 100 K of the last point is the last point.
  if (quadratic && color_temperature + 100.0f > table[last].kelvin - 1.0f)
    color_temperature = table[last].kelvin;

  // Segment with K[segment] <= color_temperature < K[segment + 1]; the last point forms its own segment.
  size_t segment = 0;
  while (segment < last && color_temperature >= table[segment + 1].kelvin)
    segment++;
  // Without interpolation the lower point of the segment is the result.
  float warm = table[segment].warm;
  float cold = table[segment].cold;

  // Segments touching the first or last point are not interpolated: those points are pure
  // single-channel white, and between them and their neighbour one channel's share would lie
  // below the minimum share. The three-point window moves down one point so it never reaches
  // the last point.
  if (quadratic && segment >= 1 && segment + 1 < last) {
    const size_t first = (segment + 2 == last) ? segment - 1 : segment;
    const MixPoint &p0 = table[first], &p1 = table[first + 1], &p2 = table[first + 2];
    float curve_warm =
        std::min(lagrange(color_temperature, p0.kelvin, p0.warm, p1.kelvin, p1.warm, p2.kelvin, p2.warm), 1.0f);
    float curve_cold =
        std::min(lagrange(color_temperature, p0.kelvin, p0.cold, p1.kelvin, p1.cold, p2.kelvin, p2.cold), 1.0f);
    // The stock firmware keeps the table point if the curve dips below the minimum share.
    if (curve_warm >= this->min_share_ && curve_cold >= this->min_share_) {
      warm = curve_warm;
      cold = curve_cold;
    }
  }
  if (this->interpolation_ == Interpolation::LINEAR && segment < last) {
    const MixPoint &lower = table[segment], &upper = table[segment + 1];
    float fraction = (color_temperature - lower.kelvin) / (upper.kelvin - lower.kelvin);
    warm = lower.warm + fraction * (upper.warm - lower.warm);
    cold = lower.cold + fraction * (upper.cold - lower.cold);
  }

  // Brightness 1..100 % maps to min_brightness..100 % output. The stock firmware works in
  // whole percent; fractions are kept here for smooth transitions.
  brightness = std::min(std::max(brightness, 0.01f), 1.0f);
  float brightness_factor;
  if (this->brightness_curve_ == BrightnessCurve::QUADRATIC) {
    // Stock curve in percent: x = (brightness + offset) / (100 + offset) with
    // offset = max(min_brightness, 15), scaled so that 1 % maps to min_brightness.
    float min_brightness_percent = this->min_brightness_ * 100.0f;
    float curve_offset = std::max(min_brightness_percent, 15.0f);
    float x = (brightness * 100.0f + curve_offset) / (100.0f + curve_offset);
    float x_at_1_percent = (1.0f + curve_offset) / (100.0f + curve_offset);
    brightness_factor =
        (min_brightness_percent + (100.0f - min_brightness_percent) * (x * x - x_at_1_percent * x_at_1_percent) /
                                      (1.0f - x_at_1_percent * x_at_1_percent)) /
        100.0f;
  } else {
    brightness_factor = this->min_brightness_ + (brightness - 0.01f) * (1.0f - this->min_brightness_) / 0.99f;
  }
  // Share of each channel scaled by the brightness factor. The stock minimum duty of a lit
  // channel is min_power with zero_means_zero on the outputs.
  this->warm_white_->set_level(warm * brightness_factor);
  this->cold_white_->set_level(cold * brightness_factor);
}

float YeelightCCTLightOutput::lagrange(float x, float x0, float y0, float x1, float y1, float x2, float y2) {
  return y0 * (x - x1) * (x - x2) / ((x0 - x1) * (x0 - x2)) + y1 * (x - x0) * (x - x2) / ((x1 - x0) * (x1 - x2)) +
         y2 * (x - x0) * (x - x1) / ((x2 - x0) * (x2 - x1));
}

}  // namespace esphome::yeelight_cct
