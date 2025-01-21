#!/bin/bash

set -e

arduino-cli compile --fqbn esp32:esp32:esp32 --libraries libraries --export-binaries Daly

arduino-cli upload -p /dev/ttyUSB1 --fqbn esp32:esp32:esp32 Daly

# docker run -it --privileged -v /dev/:/dev/ -v $PWD:/Arduino -w /Arduino arduino-cli
