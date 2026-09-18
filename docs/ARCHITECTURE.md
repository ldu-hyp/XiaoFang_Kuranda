# Software Architecture

固件采用 **ESP32-WROOM-32D + ESP-IDF + C/C++ 混合架构**。

```text
Application (C++)
    ├── UI (C++)
    ├── GameManager (C++)
    │    └── Game / GameResult
    └── c_api.hpp
         ├── display.c
         ├── imu.c / input.c
         ├── buzzer.c
         ├── storage.c
         ├── network.c / modem.c
         └── power.c
```

底层资源所有者保持 C；状态机和游戏对象使用 C++。游戏不直接操作 I2C、RMT、NVS 或 Wi-Fi。

## 输入模型

`input.c` 输出 `dir_pressed / dir_repeat / dir_released`、shake、pause、face_down 和 activity。实时移动游戏可使用自动重复；Sokoban 保持一步一动作。

## 游戏结果

`GameResult` 支持 Running、Success、Failure、Timeout、Disconnected。Application 根据结果选择不同反馈。

## 实时策略

- 基础 tick：50ms。
- WS2812：RMT。
- ESP-NOW：回调只入队。
- 蜂鸣器：独立 FreeRTOS Sound Task。
- Pong：Host 权威球物理，Client 发送 paddle。

## Deep Sleep

休眠前必须成功配置 MPU6050 Motion Interrupt、清中断、确认 GPIO33 为低并启用 EXT0；失败则取消休眠。