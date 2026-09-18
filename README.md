# XiaoFang_Kuranda

“小方”彩色重构版固件。

当前正式架构：

- **MCU：ESP32-S3-WROOM-1**
- **SDK：ESP-IDF 5.5.x**
- **底层：C**
- **应用 / UI / Game：轻量 C++**
- **显示：8×8 WS2812B，Z 型蛇形排列**
- **姿态：MPU6050**
- **声音：无源压电片 / 无源蜂鸣器**
- **无线联机：ESP-NOW**
- **未来 4G：UART2，预留 RTS/CTS/PWRKEY/DTR/RI**
- **USB：ESP32-S3 原生 USB Serial/JTAG**

这不是把两个上游项目逐文件拼起来，而是以它们的玩法和实现经验为参考重新建立的软件架构。

## 为什么换到 ESP32-S3

经典 ESP32-WROOM-32D 性能足够，但已经不适合作为新的长期硬件平台。S3 对这个项目更合适：

- 双核 240 MHz；
- 原生 USB Serial/JTAG，PCB 可以减少一个 USB-UART 芯片；
- GPIO 资源更充裕；
- 后续音乐频谱、音频、BLE、OTA、4G 数据通道扩展空间更大；
- ESP-IDF 原生支持完整。

GPIO19/20 专门保留给 S3 原生 USB。

## 为什么使用 C + C++

驱动层继续使用 C：

```text
WS2812 / MPU6050 / UART / ESP-NOW / NVS / Deep Sleep / buzzer
```

因为这些模块直接对应 ESP-IDF C API，需要明确的资源和生命周期。

应用层使用轻量 C++：

```text
Application
  └── GameManager
        ├── SnakeGame
        ├── MazeGame
        ├── HourglassGame
        ├── ...
        └── PongGame
```

工程明确禁用 exceptions 和 RTTI，并且游戏层不使用 `new/delete`。因此不会为了面向对象引入不可控的堆分配。

## 当前功能

1. 彩色沙漏
2. 骰子
3. 八卦
4. 贪吃蛇
5. 随机迷宫
6. 是方块人就下一百层
7. 推箱子
8. 躲避方块
9. ESP-NOW 双机 Pong
10. MPU6050 温度
11. 声音开关
12. NVS 设置 / 最高分
13. Deep Sleep + MPU6050 Motion Interrupt 唤醒

暂未启用：

- 音乐频谱：GPIO5 已预留 ADC1 麦克风输入；
- 电池/充电 UI：GPIO4 已预留电池 ADC，等充电管理电路确定后再实现；
- 4G 协议：串口资源已经完整预留，等模块型号确定后增加 AT/PPP 驱动。

## GPIO

| 功能 | GPIO |
|---|---:|
| WS2812B DIN | 18 |
| MPU6050 SDA | 8 |
| MPU6050 SCL | 9 |
| MPU6050 INT | 7 |
| 压电蜂鸣器 | 10 |
| 4G RX / TX | 16 / 17 |
| 4G DTR | 11 |
| 4G PWRKEY | 12 |
| 4G RI | 13 |
| 4G RTS / CTS | 14 / 15 |
| Battery ADC | 4 |
| Microphone ADC | 5 |
| USB D- / D+ | 19 / 20 |

详细说明见 `docs/HARDWARE.md`。

## 构建

推荐 ESP-IDF v5.5.5：

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

仓库中的 GitHub Actions 同样以 `esp32s3` target 编译。

## 软件目录

```text
main/
├── main.cpp
├── c_api.hpp
├── app/
│   ├── Application.hpp
│   └── Application.cpp
├── ui/
│   ├── Ui.hpp
│   └── Ui.cpp
├── games/
│   ├── Game.hpp
│   ├── GameManager.*
│   └── *Game.cpp
│
├── display.c
├── imu.c
├── input.c
├── buzzer.c
├── storage.c
├── power.c
├── network.c
└── modem.c
```

硬件 C API 与 C++ 应用层之间只通过 `c_api.hpp` 连接。

## 参考项目

- https://github.com/cwm-peace/xiaofang
- https://github.com/LittleGuest/xiaofang

版权与许可说明见 `THIRD_PARTY_NOTICES.md`。
