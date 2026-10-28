#include "esphome/components/xiaomi_ylkg07yl/xiaomi_ylkg07yl.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <vector>

namespace esphome::xiaomi_ylkg07yl::testing {

// Least significant byte first, as from_scan_result() takes them
static constexpr uint8_t LEGACY_MAC[6] = {0xBC, 0x7B, 0xC4, 0x41, 0x24, 0xF8};  // F8:24:41:C4:7B:BC
static constexpr uint8_t MODERN_MAC[6] = {0x5C, 0xD3, 0x50, 0x12, 0xC9, 0x70};  // 70:C9:12:50:D3:5C

// MiBeacon V3, frame count 0x53: button pressed once
static const std::vector<uint8_t> LEGACY_PRESS_FRAME = {0x58, 0x30, 0xB6, 0x03, 0x53, 0xBC, 0x7B,
                                                        0xC4, 0x41, 0x24, 0xF8, 0xE2, 0xB6, 0xC4,
                                                        0xD5, 0x01, 0x3A, 0x00, 0x00, 0x00, 0x74};
// MiBeacon V3, frame count 0x56: knob rotated by 13 steps
static const std::vector<uint8_t> LEGACY_ROTATE_FRAME = {0x58, 0x30, 0xB6, 0x03, 0x56, 0xBC, 0x7B,
                                                         0xC4, 0x41, 0x24, 0xF8, 0xA4, 0xAB, 0x1D,
                                                         0x68, 0x0A, 0x05, 0x00, 0x00, 0x00, 0x5E};
// 12 byte bindkey ef3afd53f5c7853a080a3d52, padded with 0xFF as the code generator does
static constexpr std::initializer_list<uint8_t> LEGACY_BINDKEY = {0xEF, 0x3A, 0xFD, 0x53, 0xF5, 0xC7, 0x85, 0x3A,
                                                                  0x08, 0x0A, 0x3D, 0x52, 0xFF, 0xFF, 0xFF, 0xFF};

// MiBeacon V5 without MAC address, frame count 0x4F: button pressed once
static const std::vector<uint8_t> MODERN_FRAME = {0x48, 0x50, 0xB6, 0x03, 0x4F, 0x9E, 0xD6, 0x8B, 0x74,
                                                  0xBE, 0x06, 0x0B, 0x00, 0x00, 0x62, 0x42, 0x26, 0xBB};
// 12 byte bindkey 5ab9badac3b1becfc5ef5dcd, padded with 0xFF as the code generator does
static constexpr std::initializer_list<uint8_t> MODERN_BINDKEY = {0x5A, 0xB9, 0xBA, 0xDA, 0xC3, 0xB1, 0xBE, 0xCF,
                                                                  0xC5, 0xEF, 0x5D, 0xCD, 0xFF, 0xFF, 0xFF, 0xFF};

struct Events {
  std::vector<int> presses;
  std::vector<int> rotations;
  std::vector<int> press_and_rotations;
};

static ble_device_base::ESPBTDevice device_from(const uint8_t *mac, const std::vector<uint8_t> &frame) {
  std::vector<uint8_t> adv = {static_cast<uint8_t>(frame.size() + 3), 0x16, 0x95, 0xFE};
  adv.insert(adv.end(), frame.begin(), frame.end());
  ble_device_base::ESPBTDevice device;
  device.from_scan_result(mac, -60, 0, adv.data(), static_cast<uint16_t>(adv.size()));
  return device;
}

static void setup_dimmer(XiaomiYLKG07YL &dimmer, Events &events, const ble_device_base::ESPBTDevice &device,
                         std::initializer_list<uint8_t> bindkey) {
  dimmer.set_address(device.address_uint64());
  dimmer.set_bindkey(bindkey);
  dimmer.add_on_press_callback([&events](int value) { events.presses.push_back(value); });
  dimmer.add_on_rotate_callback([&events](int value) { events.rotations.push_back(value); });
  dimmer.add_on_press_and_rotate_callback([&events](int value) { events.press_and_rotations.push_back(value); });
}

TEST(XiaomiYLKG07YL, DecryptsLegacyPressFrame) {
  auto device = device_from(LEGACY_MAC, LEGACY_PRESS_FRAME);
  XiaomiYLKG07YL dimmer;
  Events events;
  setup_dimmer(dimmer, events, device, LEGACY_BINDKEY);
  EXPECT_TRUE(dimmer.parse_device(device));
  EXPECT_EQ(events.presses, std::vector<int>{1});
  EXPECT_TRUE(events.rotations.empty());
  EXPECT_TRUE(events.press_and_rotations.empty());
  EXPECT_EQ(device.get_service_datas()[0].data, LEGACY_PRESS_FRAME);
}

TEST(XiaomiYLKG07YL, DecryptsLegacyRotateFrame) {
  auto device = device_from(LEGACY_MAC, LEGACY_ROTATE_FRAME);
  XiaomiYLKG07YL dimmer;
  Events events;
  setup_dimmer(dimmer, events, device, LEGACY_BINDKEY);
  EXPECT_TRUE(dimmer.parse_device(device));
  EXPECT_TRUE(events.presses.empty());
  EXPECT_EQ(events.rotations, std::vector<int>{13});
  EXPECT_TRUE(events.press_and_rotations.empty());
}

TEST(XiaomiYLKG07YL, DecryptsModernFrame) {
  auto device = device_from(MODERN_MAC, MODERN_FRAME);
  XiaomiYLKG07YL dimmer;
  Events events;
  setup_dimmer(dimmer, events, device, MODERN_BINDKEY);
  EXPECT_TRUE(dimmer.parse_device(device));
  EXPECT_EQ(events.presses, std::vector<int>{1});
  EXPECT_TRUE(events.rotations.empty());
  EXPECT_EQ(device.get_service_datas()[0].data, MODERN_FRAME);
}

TEST(XiaomiYLKG07YL, RejectsModernFrameWithWrongBindkey) {
  auto device = device_from(MODERN_MAC, MODERN_FRAME);
  XiaomiYLKG07YL dimmer;
  Events events;
  setup_dimmer(dimmer, events, device, LEGACY_BINDKEY);
  EXPECT_FALSE(dimmer.parse_device(device));
}

TEST(XiaomiYLKG07YL, DropsRepeatedFrameCount) {
  auto device = device_from(LEGACY_MAC, LEGACY_PRESS_FRAME);
  XiaomiYLKG07YL dimmer;
  Events events;
  setup_dimmer(dimmer, events, device, LEGACY_BINDKEY);
  EXPECT_TRUE(dimmer.parse_device(device));
  EXPECT_FALSE(dimmer.parse_device(device));
  EXPECT_EQ(events.presses.size(), 1u);
}

}  // namespace esphome::xiaomi_ylkg07yl::testing
