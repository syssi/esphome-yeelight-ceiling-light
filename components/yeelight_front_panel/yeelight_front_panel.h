#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

namespace esphome::yeelight_front_panel {

/// Longest message any supported panel uses. Bump when adding a model.
static const uint8_t MAX_MESSAGE_LENGTH = 8;

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

enum class Model : uint8_t {
  BSLAMP2 = 0,
  LAMP10 = 1,
};

/// Model-independent LED bitmask.
///
/// Slider LED 1 is the one closest to the power button. Each model maps these
/// bits onto its own wire format, so configurations stay portable between
/// lamps with a different number of LEDs or a different bit order.
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
  /// 1 .. FrontPanelModel::slider_level_count(), or 0 when not applicable.
  uint8_t slider_level{0};
};

/// Describes one lamp's front panel wire protocol.
///
/// Everything that differs between lamps lives behind this interface; the hub
/// component and the sensor platforms above it are model independent.
class FrontPanelModel {
 public:
  virtual ~FrontPanelModel() = default;

  virtual const char *name() const = 0;

  /// Number of bytes in every message exchanged with this panel.
  virtual uint8_t message_length() const = 0;

  /// Number of bytes read for an event. Defaults to message_length().
  virtual uint8_t event_length() const { return this->message_length(); }

  /// Number of LEDs illuminating the slider.
  virtual uint8_t slider_led_count() const = 0;

  /// Number of distinct positions the slider reports.
  virtual uint8_t slider_level_count() const = 0;

  /// Message that must be written before the pending event can be read,
  /// or nullptr when the panel can be read directly.
  virtual const uint8_t *event_request() const { return nullptr; }

  /// Decode a message into an event. Returns false when the message is not
  /// recognised, in which case the caller logs the raw bytes.
  virtual bool parse_event(const uint8_t *message, FrontPanelEvent *event) const = 0;

  /// Render an LED bitmask into an outgoing message of message_length() bytes.
  virtual void encode_leds(uint16_t leds, uint8_t *message) const = 0;
};

struct FrontPanelTriggerStore {
  volatile uint32_t event_count{0};
  static void gpio_intr(FrontPanelTriggerStore *store);
};

/// Hub component for the I2C touch panel found on several Yeelight lamps.
///
/// The panel pulls an interrupt line low when an event is waiting. The ISR only
/// counts those pulses; the main loop does the I2C work and dispatches decoded
/// events to the binary_sensor and sensor platforms.
class YeelightFrontPanel : public Component, public i2c::I2CDevice {
 public:
  void set_model(Model model) { this->model_id_ = model; }
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

  uint8_t slider_level_count() const;

 protected:
  void read_event_();
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
  const FrontPanelModel *model_{nullptr};
  Model model_id_{Model::BSLAMP2};
  bool debug_{false};

  uint32_t last_event_count_{0};
  uint32_t last_read_ms_{0};
  bool last_trigger_level_{true};
  uint16_t led_state_{LED_NONE};
  bool leds_dirty_{true};

  CallbackManager<void(const FrontPanelEvent &)> event_callback_{};
};

}  // namespace esphome::yeelight_front_panel
