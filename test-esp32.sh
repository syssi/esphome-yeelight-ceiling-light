#!/bin/bash

if [[ $2 == tests/* ]]; then
  C="../components"
else
  C="components"
fi

esphome -s external_components_source $C ${1:-run} ${2:-yeelight_light_fancl5.yaml}
