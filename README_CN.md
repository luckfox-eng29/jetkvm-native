![luckfox](https://github.com/LuckfoxTECH/luckfox-pico/assets/144299491/cec5c4a5-22b9-4a9a-abb1-704b11651e88)
[English](./README.md)
# Luckfox PicoKVM
Luckfox PicoKVM 是一款轻量级 IP KVM 运维工具，支持通过网络远程获取目标设备画面并模拟 HID 输入，实现对开发板、电脑及服务器等系统的无接触运维管理。该产品具备稳定、低延迟的视频采集和远程控制能力，广泛适用于远程电脑控制和服务器维护等场景。软件基于 JetKVM 二次开发。

## 特性
* **Micro SD 卡支持**:可用于软件启动设置或存储拓展
* **USB 多功能配置**:支持模拟 USB 声卡或 MTP 设备，在 MTP 共享目录下可通过 HTTP 上传或下载文件 
* **串口控制**:可连接受控设备的串口，实现对受控设备的串口控制和调试
* **IO 电平配置**:控制拓展 IO 输出高低电平
* **1.54 寸触控屏幕**:显示 IP 地址、连接状态及系统运行状态
* **多种远程访问方案**:使用 WebRTC 通过异域组网或 FRP 端口反向代理进行远程管理

## 编译
* 设置 luckfox-pico SDK 地址
    ```bash
    export LUCKFOX_SDK_PATH=$SDK_PATH
    ```
* 完整编译
    ```bash
    make
    ```
* 编译生成 **build/bin/kvm_video**，通过 ssh 或 MTP 将文件上传到 Luckfox PicoKVM，替换 **/userdata/picokvm/bin/kvm_video**

## 详细使用说明
[Luckfox PicoKVM](https://wiki.luckfox.com/zh/Luckfox-PicoKVM/)
