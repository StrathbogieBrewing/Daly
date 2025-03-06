#!/bin/bash

set -e

arduino-cli compile --fqbn esp32:esp32:esp32 --libraries libraries Daly
# --export-binaries 

arduino-cli upload -p /dev/ttyESP_prog --fqbn esp32:esp32:esp32 Daly

arduino-cli monitor -c baudrate=115200 -p /dev/ttyESP_prog --fqbn esp32:esp32:esp32 Daly

# monitor with  ```socat  /dev/ttyESP_prog,b115200,raw,echo=0 -```

