#pragma once

#include "yeelight_front_panel.h"

namespace esphome {
namespace yeelight_front_panel {

/// Xiaomi Mijia Bedside Lamp 2 (yeelink.light.bslamp2).
///
/// I2C address 0x2C, 7 byte messages. An event is fetched by writing a fixed
/// request and reading the reply. Message layout:
///
///   [0..3] fixed prefix 04:04:01:00
///   [4]    part: 01 power button, 02 color button, 03 slider touch,
///          04 slider release
///   [5]    buttons: 01 touch, 02 release
///          slider:  position, counting down from 0x16 at slider LED 1
///   [6]    checksum
///
/// Protocol established by the esphome-xiaomi_bslamp2 project
/// (https://github.com/mmakaay/esphome-xiaomi_bslamp2). This is an independent
/// implementation of it.
class Bslamp2Model : public FrontPanelModel {
 public:
  static const uint8_t ADDRESS = 0x2C;
  static const uint8_t MESSAGE_LENGTH = 7;
  static const uint8_t SLIDER_LEDS = 10;
  static const uint8_t SLIDER_LEVELS = 22;

  const char *name() const override { return "bslamp2"; }
  uint8_t message_length() const override { return MESSAGE_LENGTH; }
  uint8_t slider_led_count() const override { return SLIDER_LEDS; }
  uint8_t slider_level_count() const override { return SLIDER_LEVELS; }
  const uint8_t *event_request() const override;
  bool parse_event(const uint8_t *message, FrontPanelEvent *event) const override;
  void encode_leds(uint16_t leds, uint8_t *message) const override;
};

/// Yeelight Staria Floor Lamp (yeelink.light.lamp10).
///
/// I2C address 0x50, 3 byte messages. Established by static analysis of the
/// original firmware: the front panel driver writes three bytes to slave 0x50
/// on I2C port 1, caching a 16 bit LED state into the last two bytes first --
/// structurally the same LED update the Bedside Lamp 2 performs.
///
/// Events are read as 6 bytes, captured on the device on 2026-09-12:
///
///   [0]  fixed 0A
///   [1]  power button:  01 touch, 02 held (repeats), 03 release
///   [2]  colour button: 01 touch, 02 held (repeats), 03 release
///   [3]  slider: 01 touch / moving, 02 release
///   [4]  slider position, 00 at the "-" end (power button) .. 15 at "+"
///   [5]  always 00
///
/// While events are pending the panel holds the trigger line low, and reads
/// then return all zeros once it has nothing more to report.
class Lamp10Model : public FrontPanelModel {
 public:
  static const uint8_t ADDRESS = 0x50;
  static const uint8_t MESSAGE_LENGTH = 3;
  static const uint8_t EVENT_LENGTH = 6;
  /// Unverified: taken from the number of slider LEDs visible on the device.
  static const uint8_t SLIDER_LEDS = 10;
  /// Verified: positions 00..15 were all observed.
  static const uint8_t SLIDER_LEVELS = 22;
  /// Unverified: the command byte that precedes the 16 bit LED state.
  static const uint8_t LED_COMMAND = 0x02;

  const char *name() const override { return "lamp10"; }
  uint8_t message_length() const override { return MESSAGE_LENGTH; }
  uint8_t event_length() const override { return EVENT_LENGTH; }
  uint8_t slider_led_count() const override { return SLIDER_LEDS; }
  uint8_t slider_level_count() const override { return SLIDER_LEVELS; }
  bool parse_event(const uint8_t *message, FrontPanelEvent *event) const override;
  void encode_leds(uint16_t leds, uint8_t *message) const override;

 protected:
  /// The panel reports its whole state on every read, so events are derived
  /// from what changed since the previous message.
  mutable uint8_t last_[EVENT_LENGTH]{};
};

const FrontPanelModel *get_model(Model model);

}  // namespace yeelight_front_panel
}  // namespace esphome
