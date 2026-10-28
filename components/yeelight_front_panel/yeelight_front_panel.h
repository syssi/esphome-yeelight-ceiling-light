#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

namespace esphome::yeelight_front_panel {

/// LED updates are 3 bytes: a command byte followed by the 16 bit LED state.
/// Established by static analysis of the original firmware: the front panel
/// driver writes three bytes to I2C address 0x50 on I2C port 1, caching a 16 bit
/// LED state into the last two bytes first -- structurally the same LED update
/// the Bedside Lamp 2 performs.
static const uint8_t LED_MESSAGE_LENGTH = 3;
/// Unverified: the command byte that precedes the 16 bit LED state.
static const uint8_t LED_COMMAND = 0x02;

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
static const uint8_t EVENT_LENGTH = 6;

/// Unverified: taken from the number of slider LEDs visible on the device.
static const uint8_t SLIDER_LEDS = 10;
/// Verified: positions 00..15 were all observed.
static const uint8_t SLIDER_LEVELS = 22;

/// The element of the front panel that generated an event.
enum class FrontPanelPart : uint8_t {
  UNKNOWN = 0,
  POWER_BUTTON = 1,
  COLOR_BUTTON = 2,
  SLIDER = 3,
};

/// What happened to that element.
enum class FrontPanelAction : uint8_t {
  UNKNOWN = 0,
  TOUCH = 1,
  RELEASE = 2,
};

/// LED bitmask, written to the panel as is.
///
/// Slider LED 1 is the one closest to the power button.
enum FrontPanelLED : uint16_t {
  LED_NONE = 0,
  LED_SLIDER_1 = 1 << 0,
  LED_SLIDER_2 = 1 << 1,
  LED_SLIDER_3 = 1 << 2,
  LED_SLIDER_4 = 1 << 3,
  LED_SLIDER_5 = 1 << 4,
  LED_SLIDER_6 = 1 << 5,
  LED_SLIDER_7 = 1 << 6,
  LED_SLIDER_8 = 1 << 7,
  LED_SLIDER_9 = 1 << 8,
  LED_SLIDER_10 = 1 << 9,
  LED_POWER_BUTTON = 1 << 10,
  LED_COLOR_BUTTON = 1 << 11,
  LED_ALL_SLIDER = 0x03FF,
  LED_ALL = 0x0FFF,
};

struct FrontPanelEvent {
  FrontPanelPart part{FrontPanelPart::UNKNOWN};
  FrontPanelAction action{FrontPanelAction::UNKNOWN};
  /// 1 .. SLIDER_LEVELS, or 0 when not applicable.
  uint8_t slider_level{0};
};

struct FrontPanelTriggerStore {
  volatile uint32_t event_count{0};
  static void gpio_intr(FrontPanelTriggerStore *store);
};

/// Hub component for the I2C touch panel of the Yeelight Staria Floor Lamp
/// (yeelink.light.lamp10).
///
/// The panel pulls an interrupt line low when an event is waiting. The ISR only
/// counts those pulses; the main loop does the I2C work and dispatches decoded
/// events to the binary_sensor and sensor platforms.
class YeelightFrontPanel : public Component, public i2c::I2CDevice {
 public:
  void set_trigger_pin(InternalGPIOPin *pin) { this->trigger_pin_ = pin; }
  void set_debug(bool debug) { this->debug_ = debug; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void add_on_event_callback(std::function<void(const FrontPanelEvent &)> &&callback) {
    this->event_callback_.add(std::move(callback));
  }

  /// Turn the given LEDs on, leaving the others untouched.
  void turn_on_leds(uint16_t leds) { this->update_leds_(this->led_state_ | leds); }

  /// Turn the given LEDs off, leaving the others untouched.
  void turn_off_leds(uint16_t leds) { this->update_leds_(this->led_state_ & ~leds); }

  /// Turn on exactly the given LEDs and turn off all others.
  void set_leds(uint16_t leds) { this->update_leds_(leds); }

  /// Light the slider LEDs to represent a level between 0.0 and 1.0.
  /// 0.0 turns the slider illumination off entirely.
  void set_slider_level(float level);

  uint8_t slider_level_count() const { return SLIDER_LEVELS; }

 protected:
  void read_event_();
  bool parse_event_(const uint8_t *message, FrontPanelEvent *event);
  void flush_leds_();

  void update_leds_(uint16_t leds) {
    leds &= LED_ALL;
    if (leds == this->led_state_)
      return;
    this->led_state_ = leds;
    this->leds_dirty_ = true;
  }

  InternalGPIOPin *trigger_pin_{nullptr};
  FrontPanelTriggerStore store_{};
  bool debug_{false};

  uint32_t last_event_count_{0};
  uint32_t last_read_ms_{0};
  bool last_trigger_level_{true};
  uint16_t led_state_{LED_NONE};
  bool leds_dirty_{true};
  /// The panel reports its whole state on every read, so events are derived
  /// from what changed since the previous message.
  uint8_t last_message_[EVENT_LENGTH]{};

  CallbackManager<void(const FrontPanelEvent &)> event_callback_{};
};

}  // namespace esphome::yeelight_front_panel
