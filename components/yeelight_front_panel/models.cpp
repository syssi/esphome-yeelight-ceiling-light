#include "models.h"

#include <cstring>

namespace esphome {
namespace yeelight_front_panel {

// --- Bedside Lamp 2 --------------------------------------------------------

const uint8_t *Bslamp2Model::event_request() const {
  static const uint8_t REQUEST[MESSAGE_LENGTH] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  return REQUEST;
}

bool Bslamp2Model::parse_event(const uint8_t *message, FrontPanelEvent *event) const {
  if (message[0] != 0x04 || message[1] != 0x04 || message[2] != 0x01 || message[3] != 0x00)
    return false;

  switch (message[4]) {
    case 0x01:
    case 0x02:
      event->part = message[4] == 0x01 ? FrontPanelPart::POWER_BUTTON : FrontPanelPart::COLOR_BUTTON;
      if (message[5] == 0x01 && message[6] == 0x02 + message[4]) {
        event->action = FrontPanelAction::TOUCH;
      } else if (message[5] == 0x02 && message[6] == 0x03 + message[4]) {
        event->action = FrontPanelAction::RELEASE;
      } else {
        return false;
      }
      return true;

    case 0x03:
    case 0x04:
      event->part = FrontPanelPart::SLIDER;
      event->action = message[4] == 0x03 ? FrontPanelAction::TOUCH : FrontPanelAction::RELEASE;
      // The last byte is a checksum over the part id and the position.
      if (message[6] != message[5] + message[4] + 0x01)
        return false;
      if (message[5] < 0x01 || message[5] > SLIDER_LEVELS)
        return false;
      // The panel counts down from the far end of the slider; flip it so that
      // level 1 is the position closest to the power button.
      event->slider_level = SLIDER_LEVELS + 1 - message[5];
      return true;

    default:
      return false;
  }
}

void Bslamp2Model::encode_leds(uint16_t leds, uint8_t *message) const {
  // Bits 10 and 11 must be set for the panel to accept the message.
  uint16_t wire = 0x0C00;
  // The panel numbers its slider LEDs in the opposite direction.
  for (uint8_t i = 0; i < SLIDER_LEDS; i++) {
    if (leds & (1 << i))
      wire |= 1 << (SLIDER_LEDS - 1 - i);
  }
  if (leds & LED_POWER_BUTTON)
    wire |= 1 << 14;
  if (leds & LED_COLOR_BUTTON)
    wire |= 1 << 12;

  message[0] = 0x02;
  message[1] = 0x03;
  message[2] = wire >> 8;
  message[3] = wire & 0xFF;
  message[4] = 0x64;
  message[5] = 0x00;
  message[6] = 0x00;
}

// --- Staria Floor Lamp -----------------------------------------------------

bool Lamp10Model::parse_event(const uint8_t *message, FrontPanelEvent *event) const {
  if (message[0] != 0x0A || message[5] != 0x00 || message[4] >= SLIDER_LEVELS)
    return false;

  // The panel reports its whole state on every read and never clears stale
  // fields: after a button press the slider still shows its last state and
  // position. Only a field that changed since the previous message is an event.
  // Where several changed, the original touch_event_handler's precedence
  // applies: colour button over power button over slider.
  //
  // A button can also drop from touch or held straight to 00 without the 03
  // ever being read (seen when the panel released the trigger line early).
  // That drop counts as the release.
  const auto button_changed = [this, message](uint8_t i) {
    if (message[i] == this->last_[i])
      return false;
    return message[i] != 0x00 || this->last_[i] == 0x01 || this->last_[i] == 0x02;
  };
  const bool color = button_changed(2);
  const bool power = button_changed(1);
  const bool slider = (message[3] != this->last_[3] || message[4] != this->last_[4]) && message[3] != 0x00;
  std::memcpy(this->last_, message, EVENT_LENGTH);

  if (color || power) {
    event->part = color ? FrontPanelPart::COLOR_BUTTON : FrontPanelPart::POWER_BUTTON;
    switch (color ? message[2] : message[1]) {
      case 0x01:
        event->action = FrontPanelAction::TOUCH;
        return true;
      case 0x02:
        // Held. Sent once on the change, then repeated unchanged.
        event->action = FrontPanelAction::UNKNOWN;
        return true;
      case 0x00:
      case 0x03:
        event->action = FrontPanelAction::RELEASE;
        return true;
      default:
        return false;
    }
  }

  if (slider) {
    event->part = FrontPanelPart::SLIDER;
    // 01 on touch and while moving, 02 on release.
    switch (message[3]) {
      case 0x01:
        event->action = FrontPanelAction::TOUCH;
        break;
      case 0x02:
        event->action = FrontPanelAction::RELEASE;
        break;
      default:
        return false;
    }
    // Position 0 is the "-" end, next to the power button - already level 1.
    // The position sent with a release is not where the finger left: captures
    // show 06, 0B or 0C regardless. Only touches carry a level.
    if (event->action == FrontPanelAction::TOUCH)
      event->slider_level = message[4] + 1;
    return true;
  }

  // Nothing changed: a repeat, or the idle answer while the line is still low.
  return true;
}

void Lamp10Model::encode_leds(uint16_t leds, uint8_t *message) const {
  message[0] = LED_COMMAND;
  message[1] = leds >> 8;
  message[2] = leds & 0xFF;
}

// --- Registry --------------------------------------------------------------

const FrontPanelModel *get_model(Model model) {
  static const Bslamp2Model BSLAMP2;
  static const Lamp10Model LAMP10;

  switch (model) {
    case Model::LAMP10:
      return &LAMP10;
    case Model::BSLAMP2:
    default:
      return &BSLAMP2;
  }
}

}  // namespace yeelight_front_panel
}  // namespace esphome
