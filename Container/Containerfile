FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
ENV USER=root


RUN apt-get clean && \
    apt-get update && \
    apt-get install -y curl python3 && \
    apt-get clean


RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/bin sh


RUN arduino-cli core update-index
RUN arduino-cli core install arduino:avr
RUN arduino-cli core --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json install esp32:esp32
