#include "yeelight_cct_mix.h"

#include <algorithm>
#include <cmath>

namespace esphome::yeelight_cct {

static float lagrange(float x, float x0, float y0, float x1, float y1, float x2, float y2) {
  return y0 * (x - x1) * (x - x2) / ((x0 - x1) * (x0 - x2)) + y1 * (x - x0) * (x - x2) / ((x1 - x0) * (x1 - x2)) +
         y2 * (x - x0) * (x - x1) / ((x2 - x0) * (x2 - x1));
}

void mix(const MixParams &params, float kelvin, float brightness, float *warm, float *cold) {
  *warm = 0.0f;
  *cold = 0.0f;
  const auto &table = params.table;
  if (brightness <= 0.0f || table.empty())
    return;
  const size_t last = table.size() - 1;

  // The stock firmware works in whole kelvin. Rounding also keeps e.g. 3000 K, which arrives
  // as 2999.99 K after the mired round trip, from falling into the segment below.
  float t = std::round(kelvin);
  t = std::min(std::max(t, float(table[0].kelvin)), float(table[last].kelvin));
  // Quadratic mode: everything within 100 K of the last point is the last point.
  if (params.interpolation == Interpolation::QUADRATIC && t + 100.0f > table[last].kelvin - 1.0f)
    t = table[last].kelvin;

  // Segment i with K[i] <= t < K[i + 1]; the last point forms its own segment.
  size_t i = 0;
  while (i < last && t >= table[i + 1].kelvin)
    i++;
  float w = table[i].warm;
  float c = table[i].cold;

  // Segments touching the first or last point are not interpolated: those points are pure
  // single-channel white, and between them and their neighbour one channel's share would lie
  // below the minimum share. The three-point window moves down one point so it never reaches
  // the last point.
  if (params.interpolation == Interpolation::QUADRATIC && i >= 1 && i + 1 < last) {
    const size_t u = (i + 2 == last) ? i - 1 : i;
    const MixPoint &p0 = table[u], &p1 = table[u + 1], &p2 = table[u + 2];
    float wl = std::min(lagrange(t, p0.kelvin, p0.warm, p1.kelvin, p1.warm, p2.kelvin, p2.warm), 1.0f);
    float cl = std::min(lagrange(t, p0.kelvin, p0.cold, p1.kelvin, p1.cold, p2.kelvin, p2.cold), 1.0f);
    // The stock firmware keeps the table point if the curve dips below the minimum share.
    if (wl >= params.min_share && cl >= params.min_share) {
      w = wl;
      c = cl;
    }
  }

  // Brightness 1..100 % maps linearly to min_brightness..100 % output. The stock firmware
  // works in whole percent; fractions are kept here for smooth transitions.
  float b = std::min(std::max(brightness, 0.01f), 1.0f);
  float f = params.min_brightness + (b - 0.01f) * (1.0f - params.min_brightness) / 0.99f;
  float ow = w * f;
  float oc = c * f;
  // Every lit channel is offset by the stock minimum duty.
  *warm = ow > 0.0f ? params.min_duty + (1.0f - params.min_duty) * ow : 0.0f;
  *cold = oc > 0.0f ? params.min_duty + (1.0f - params.min_duty) * oc : 0.0f;
}

}  // namespace esphome::yeelight_cct
