![luckfox](https://github.com/LuckfoxTECH/luckfox-pico/assets/144299491/cec5c4a5-22b9-4a9a-abb1-704b11651e88)
[中文](./README_CN.md)
# Luckfox PicoKVM
Luckfox PicoKVM is a lightweight IP KVM operation and maintenance tool. It allows remote access to a target device’s display and simulates HID input over the network, enabling contactless operation and management of development boards, PCs, and servers. The product delivers stable, low-latency video capture and remote control, making it well-suited for scenarios such as remote computer management and server maintenance. The software is based on a secondary development of JetKVM.

## Features
* **Micro SD card support**: Used for software boot settings or storage expansion
* **USB multifunction configuration**: Supports emulating a USB sound card or MTP device. Files in the shared MTP directory can be uploaded or downloaded via HTTP
* **Serial port control**: Connects to the serial port of the controlled device for control and debugging
* **IO level configuration**: Controls expansion IO output (high/low level)
* **1.54-inch touchscreen**: Displays IP address, connection status, and system runtime status
* **Multiple remote access methods**: Remote management via WebRTC through cross-network connections or FRP reverse proxy

## Compilation
* Set the Luckfox Pico SDK path
  ```bash
  export LUCKFOX_SDK_PATH=$SDK_PATH
  ```
* Full Compilation
  ```bash
  make
  ```
* Compile to generate **build/bin/kvm_video**, then upload the file to Luckfox PicoKVM via SSH or MTP, replacing **/userdata/picokvm/bin/kvm_video**

## Detailed Usage Instructions
[Luckfox PicoKVM](https://wiki.luckfox.com/Luckfox-PicoKVM/)
