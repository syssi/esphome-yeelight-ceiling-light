"""Schema structure tests for yeelight_ceiling_light ESPHome component modules."""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

import components.xiaomi_ylkg07yl as ylkg07yl  # noqa: E402
import components.xiaomi_ylyk01yl as ylyk01yl  # noqa: E402
import components.yeelight_fan_controller as hub  # noqa: E402


class TestHubConstants:
    def test_conf_yeelight_fan_controller_id_defined(self):
        assert hub.CONF_YEELIGHT_FAN_CONTROLLER_ID == "yeelight_fan_controller_id"


class TestYlkg07ylConstants:
    def test_sensors_list(self):
        assert ylkg07yl.CONF_KEYCODE in ylkg07yl.SENSORS
        assert ylkg07yl.CONF_ENCODER_VALUE in ylkg07yl.SENSORS
        assert ylkg07yl.CONF_ACTION_TYPE in ylkg07yl.SENSORS
        assert len(ylkg07yl.SENSORS) == 3

    def test_on_press_actions_list(self):
        assert ylkg07yl.CONF_ON_PRESS_AND_ROTATE in ylkg07yl.ON_PRESS_ACTIONS
        assert ylkg07yl.CONF_ON_ROTATE in ylkg07yl.ON_PRESS_ACTIONS
        assert len(ylkg07yl.ON_PRESS_ACTIONS) == 3


class TestYlyk01ylConstants:
    def test_on_press_actions_list(self):
        assert ylyk01yl.CONF_ON_LONG_PRESS in ylyk01yl.ON_PRESS_ACTIONS
        assert len(ylyk01yl.ON_PRESS_ACTIONS) == 2
