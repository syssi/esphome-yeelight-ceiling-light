#pragma once

#include <cstdint>
#include <vector>

namespace esphome::yeelight_cct {

/// One point of the stock calibration table: warm and cold share of full scale (0..1) at a colour temperature.
struct MixPoint {
  uint16_t kelvin;
  float warm;
  float cold;
};

/// How the stock firmware mixes between two calibration points.
enum class Interpolation : uint8_t {
  /// Use the lower point of the segment (stock firmware 1.3.x).
  NONE,
  /// Quadratic Lagrange interpolation with the stock edge rules (stock firmware 2.x).
  QUADRATIC,
};

struct MixParams {
  std::vector<MixPoint> table;  // sorted by rising kelvin
  Interpolation interpolation{Interpolation::QUADRATIC};
  float min_share{0.0f};       // QUADRATIC: fall back to the table point if a share drops below this
  float min_brightness{0.0f};  // output factor at 1 % brightness
  float min_duty{0.0f};        // duty offset of every lit channel
};

/// Compute the warm and cold duty (0..1) for a colour temperature in kelvin and a brightness (0..1).
void mix(const MixParams &params, float kelvin, float brightness, float *warm, float *cold);

}  // namespace esphome::yeelight_cct
