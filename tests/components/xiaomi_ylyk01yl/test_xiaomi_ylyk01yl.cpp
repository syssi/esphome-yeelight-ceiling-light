#include "esphome/components/xiaomi_ylyk01yl/xiaomi_ylyk01yl.h"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace esphome::xiaomi_ylyk01yl::testing {

// Least significant byte first, as from_scan_result() takes them
static constexpr uint8_t MAC[6] = {0x2D, 0x23, 0x6C, 0x38, 0xC1, 0xA4};  // A4:C1:38:6C:23:2D

static std::vector<uint8_t> frame(uint16_t product_id, uint8_t frame_count, uint8_t keycode, uint8_t action_type) {
  return {0x50,
          0x30,
          static_cast<uint8_t>(product_id & 0xFF),
          static_cast<uint8_t>(product_id >> 8),
          frame_count,
          MAC[0],
          MAC[1],
          MAC[2],
          MAC[3],
          MAC[4],
          MAC[5],
          0x01,
          0x10,
          0x03,
          keycode,
          0x00,
          action_type};
}

static ble_device_base::ESPBTDevice device_from(const std::vector<uint8_t> &frame) {
  std::vector<uint8_t> adv = {static_cast<uint8_t>(frame.size() + 3), 0x16, 0x95, 0xFE};
  adv.insert(adv.end(), frame.begin(), frame.end());
  ble_device_base::ESPBTDevice device;
  device.from_scan_result(MAC, -60, 0, adv.data(), static_cast<uint16_t>(adv.size()));
  return device;
}

class XiaomiYLYK01YLTest : public ::testing::Test {
 protected:
  void SetUp() override {
    this->remote_.set_address(device_from(frame(0x0153, 0, 0, 0)).address_uint64());
    this->remote_.add_on_receive_callback(
        [this](uint8_t keycode, bool is_long_press) { this->events_.emplace_back(keycode, is_long_press); });
  }

  XiaomiYLYK01YL remote_;
  std::vector<std::pair<uint8_t, bool>> events_;
};

TEST_F(XiaomiYLYK01YLTest, ReportsPress) {
  EXPECT_TRUE(this->remote_.parse_device(device_from(frame(0x0153, 1, 3, 0))));
  ASSERT_EQ(this->events_.size(), 1u);
  EXPECT_EQ(this->events_[0], std::make_pair(uint8_t{3}, false));
}

TEST_F(XiaomiYLYK01YLTest, ReportsLongPress) {
  EXPECT_TRUE(this->remote_.parse_device(device_from(frame(0x0153, 1, 5, 2))));
  ASSERT_EQ(this->events_.size(), 1u);
  EXPECT_EQ(this->events_[0], std::make_pair(uint8_t{5}, true));
}

TEST_F(XiaomiYLYK01YLTest, ReportsCeilingFanRemote) {
  EXPECT_TRUE(this->remote_.parse_device(device_from(frame(0x068E, 1, 4, 0))));
  ASSERT_EQ(this->events_.size(), 1u);
  EXPECT_EQ(this->events_[0], std::make_pair(uint8_t{4}, false));
}

TEST_F(XiaomiYLYK01YLTest, ReportsFirstFrame) {
  EXPECT_TRUE(this->remote_.parse_device(device_from(frame(0x0153, 0, 1, 0))));
  EXPECT_EQ(this->events_.size(), 1u);
}

TEST_F(XiaomiYLYK01YLTest, DropsRepeatedFrameCount) {
  auto device = device_from(frame(0x0153, 7, 0, 0));
  EXPECT_TRUE(this->remote_.parse_device(device));
  EXPECT_FALSE(this->remote_.parse_device(device));
  EXPECT_EQ(this->events_.size(), 1u);
}

TEST_F(XiaomiYLYK01YLTest, IgnoresUnknownProduct) {
  EXPECT_FALSE(this->remote_.parse_device(device_from(frame(0x1234, 1, 0, 0))));
  EXPECT_TRUE(this->events_.empty());
}

}  // namespace esphome::xiaomi_ylyk01yl::testing
