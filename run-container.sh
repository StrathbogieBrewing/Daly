#!/bin/bash
set -e

cd Arduino
podman run -ti -v /dev/ttyESP_prog:/dev/ttyESP_prog --group-add keep-groups -v $(pwd):/Arduino localhost/arduino-cli
