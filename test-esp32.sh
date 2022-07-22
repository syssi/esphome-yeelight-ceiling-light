#!/bin/bash

esphome -s external_components_source components ${1:-run} ${2:-yeelight_light_fancl5.yaml}
