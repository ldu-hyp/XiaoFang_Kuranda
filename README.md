# XiaoFang_Kuranda

“小方”彩色重构版固件。当前代码已经从早期 ATmega328P/Arduino 方案和 ESP32 纯 C 原型进一步重构为 **ESP32-S3 + ESP-IDF + C/C++ 混合架构**。

> **重要：当前仓库目标已经是 ESP32-S3-WROOM-1，而不是 ESP32-WROOM-32D。**
> 如果最终硬件仍然使用 ESP32-WROOM-32D，需要重新切回 `esp32` target，并恢复对应 GPIO、USB、Deep Sleep 与外设资源规划，不能直接烧录当前版本。

当前 `main` 已通过 GitHub Actions 的 **ESP-IDF v5.5.5 / esp32s3** 完整编译。

## 当前硬件目标

- MCU：**ESP32-S3-WROOM-1**
- SDK：ESP-IDF 5.5.x
- 底层驱动：C
- Application / UI / Game：轻量 C++
- 显示：8×8 WS2812B，Z 型蛇形排列
- 姿态：MPU6050
- 声音：无源压电片 / 无源蜂鸣器
- 双机通信：ESP-NOW
- 未来 4G：UART2，已预留 RX/TX/RTS/CTS/PWRKEY/DTR/RI
- USB：ESP32-S3 原生 USB Serial/JTAG

本项目不是将两个上游项目逐文件拼接，而是参考其玩法和交互后重新实现。

---

## 软件架构

```text
Application (C++)
    │
    ├── UI (C++)
    │
    ├── GameManager (C++)
    │     └── Game interface
    │          ├── HourglassGame
    │          ├── DiceGame
    │          ├── BaguaGame
    │          ├── SnakeGame
    │          ├── MazeGame
    │          ├── CubeManGame
    │          ├── SokobanGame
    │          ├── DodgeGame
    │          └── PongGame
    │
    └── c_api.hpp
          ├── display.c
          ├── imu.c
          ├── input.c
          ├── buzzer.c
          ├── storage.c
          ├── power.c
          ├── network.c
          └── modem.c
```

驱动和系统资源继续使用 C，以贴近 ESP-IDF 原生 API；产品状态机和游戏对象使用 C++。工程关闭 exceptions / RTTI，并且应用层不使用 `new/delete`。

详细设计见 [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)。

---

## 当前功能状态

| 功能 | 状态 | 说明 |
|---|---|---|
| WS2812B RGB 显示 | 已实现 | RMT 驱动、8×8 framebuffer、Z 型映射 |
| RGB 亮度控制 | 已实现 | NVS 可保存亮度，但当前 UI 尚未提供亮度设置入口 |
| 帧级 LED 电流预算 | 已实现 | 默认约 800 mA 软件限制，仅作保护，不代替电源设计 |
| MPU6050 加速度/陀螺仪 | 已实现 | I2C 400 kHz |
| 倾斜/摇动/下甩/倒扣检测 | 已实现 | 阈值仍需根据实际安装方向真机标定 |
| MPU6050 温度显示 | 已实现 | **这是 MPU6050 芯片内部温度，不是环境温度计** |
| NVS 设置/最高分 | 已实现 | 声音、亮度、休眠时间及部分游戏最高分 |
| Deep Sleep | 已实现 | MPU6050 Motion Interrupt + EXT0 唤醒 |
| 沙漏 | 已实现 | 维护上下腔粒子占用和堆积 |
| 骰子 | 已实现 | 摇动随机 |
| 八卦 | 已实现 | 摇动随机 |
| 贪吃蛇 | 已实现 | 已修复 next-head 自撞逻辑 |
| 随机迷宫 | 已实现 | DFS 生成 + BFS 最远终点 |
| 下一百层 | 已实现 | 多种平台类型 |
| 推箱子 | 已实现 | 当前 3 个内置关卡 |
| 躲避方块 | 已实现 | 障碍自顶部向下移动 |
| ESP-NOW Pong | 已实现原型 | 可双机联机，但握手可靠性仍需改进 |
| 4G UART 传输层 | 已预留 | 支持可选 RTS/CTS；尚无具体 AT/PPP 协议 |
| Battery ADC | 仅预留 | GPIO4；需要确定分压与电源管理芯片 |
| 麦克风/音乐频谱 | 仅预留 | GPIO5；需要模拟前端与固定采样率 ADC |
| BLE / OTA | 未实现 | 作为后续扩展 |

---

## 当前交互

默认交互由 MPU6050 完成：

- 菜单左右倾斜：切换项目；
- 向上倾斜：确认；
- 游戏中下甩：返回菜单；
- 摇动：骰子 / 八卦等功能触发；
- 长时间无有效活动：进入 Deep Sleep；
- 非菜单状态下倒扣：进入 Deep Sleep；
- MPU6050 Motion Interrupt：唤醒 ESP32-S3。

所有姿态阈值集中在 `main/xf_config.h`。

### 当前输入逻辑的一个限制

`input.activity` 目前只在“方向发生变化、检测到摇动或下甩”时置位。

因此设备如果长时间保持同一倾斜方向，即使用户仍在操作，系统仍可能在休眠超时后认为“无活动”。正式版本建议增加：

- 加速度变化量；
- 陀螺仪角速度；
- 或持续非中性姿态；

作为 activity 判定依据。

---

## 游戏层已知问题 / 待改进

### 1. Pong 握手可靠性

当前 Client 收到 Host 的 SEEK 后会立即：

1. 发送 JOIN；
2. 直接将本地状态标记为 paired。

如果 JOIN 丢包，Host 仍未完成配对，但 Client 已停止按未配对状态继续握手，最终只能依赖超时退出。

推荐将连接过程明确拆成：

```text
IDLE
  ↓
SEEKING
  ↓
JOIN_SENT
  ↓
START/ACK
  ↓
CONNECTED
```

Client 只有收到 Host 的 START/ACK 后才能进入 CONNECTED，并在 JOIN_SENT 阶段周期性重发 JOIN。

### 2. Maze / Sokoban 等方向事件是“边沿触发”

目前大部分游戏依据 `dir_changed` 移动一步。

这适合推箱子，但对于迷宫和部分实时游戏意味着用户需要不断“倾斜 → 回中 → 再倾斜”才能连续移动。

建议输入层进一步提供：

- `dir_pressed`
- `dir_repeat`
- `dir_released`

并实现类似按键的首延时 + 自动重复。

### 3. Game finished 只有一个布尔结果

当前 `Game::finished()` 只能表示“结束”，Application 对所有结束统一播放 `buzzer_game_over()`。

但实际结束原因可能是：

- 游戏失败；
- 推箱子全部通关；
- Pong 获胜；
- Pong 配对超时；
- 网络掉线。

建议后续改成：

```cpp
enum class GameResult {
    Running,
    Success,
    Failure,
    Timeout,
    Disconnected,
};
```

这样 UI、声音和统计逻辑可以正确区分。

### 4. 蜂鸣器 API 当前是阻塞式

`buzzer_tone()` 内部通过 `vTaskDelay()` 等待音符结束。

短提示音影响不大，但在 Pong/实时游戏中会直接阻塞 Application 主循环和网络包处理。

建议后续增加独立 Sound Task 或非阻塞音符队列。

### 5. 网络初始化错误路径需要完整回滚

`network_init()` 依次初始化 Wi-Fi、ESP-NOW、回调和 Queue。正常路径没有问题，但如果中途初始化失败，目前没有完整释放前面已经申请的资源。

建议按资源获取顺序增加统一 rollback。

### 6. Deep Sleep 唤醒准备缺少失败保护

`power_enter_deep_sleep()` 当前调用 `imu_prepare_motion_wake()` 后直接启用 EXT0 和睡眠，没有检查 MPU6050 Motion Interrupt 配置是否成功。

正式产品中应当：

- 检查 MPU 配置结果；
- 检查唤醒 GPIO 电平；
- 确认 INT 不处于永久高电平；
- 失败时拒绝进入无可靠唤醒源的 Deep Sleep，或增加 Timer/按键备用唤醒。

---

## GPIO 分配

| 功能 | GPIO | 说明 |
|---|---:|---|
| WS2812B DIN | 18 | RMT |
| MPU6050 SDA | 8 | I2C0 |
| MPU6050 SCL | 9 | I2C0 |
| MPU6050 INT | 7 | Deep Sleep EXT0 |
| 压电蜂鸣器 | 10 | LEDC |
| 4G RX | 16 | UART2 |
| 4G TX | 17 | UART2 |
| 4G DTR | 11 | 预留 |
| 4G PWRKEY | 12 | 预留 |
| 4G RI | 13 | 预留 |
| 4G RTS | 14 | 可选流控 |
| 4G CTS | 15 | 可选流控 |
| Battery ADC | 4 | ADC1 预留 |
| Microphone ADC | 5 | ADC1 预留 |
| USB D- | 19 | ESP32-S3 原生 USB |
| USB D+ | 20 | ESP32-S3 原生 USB |

更详细的硬件规划见 [docs/HARDWARE.md](docs/HARDWARE.md)。

---

## WS2812B 物理排列

逻辑坐标以左上角为 `(0,0)`，物理灯珠使用 Z 型蛇形连接：

```text
row0:  0  1  2  3  4  5  6  7
row1: 15 14 13 12 11 10  9  8
row2: 16 17 18 19 20 21 22 23
...
```

推荐：

- WS2812B 使用独立 5V 电源；
- ESP32-S3 与灯板共地；
- 使用 74AHCT125 / 74AHCT1G125 做 3.3V → 5V 数据电平转换；
- DIN 串 220–470Ω；
- 灯板电源入口加 470–1000µF 储能电容；
- 不要依赖软件限流替代硬件电源设计。

---

## 4G 扩展

当前只完成 UART transport：

```text
GPIO17 TX -> MODEM RX
GPIO16 RX <- MODEM TX
GPIO14 RTS
GPIO15 CTS
GPIO11 DTR
GPIO12 PWRKEY
GPIO13 RI
```

支持：

```c
modem_uart_init(baud, hardware_flow_control);
```

尚未实现：

- 模块开关机状态机；
- PWRKEY/DTR/RI 型号适配；
- AT 命令解析器；
- SIM / 网络注册；
- TCP/MQTT；
- PPP 数据模式。

确定具体 4G 模块型号后再增加设备驱动层，不建议把型号相关逻辑放进 `Application`。

---

## 构建

推荐 ESP-IDF **v5.5.5**：

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

GitHub Actions 当前同样使用：

```text
ESP-IDF v5.5.5
target = esp32s3
```

当前 `main` 已通过 CI 编译。

---

## 真机验证清单

在把当前版本视为可发布固件之前，至少需要完成：

1. WS2812B Z 型映射和 GRB 顺序；
2. 最大亮度、电源压降和温升；
3. MPU6050 PCB 实际安装方向；
4. 倾斜、摇动、下甩阈值；
5. Deep Sleep → Motion Interrupt → 唤醒；
6. 长时间运行时误休眠；
7. NVS 保存/掉电恢复；
8. 两台设备 ESP-NOW Pong 握手、断线和重连；
9. USB Serial/JTAG 下载与日志；
10. 不同 ESP32-S3-WROOM-1 Flash/PSRAM 型号下的 GPIO 兼容性。

---

## 目录

```text
main/
├── main.cpp
├── c_api.hpp
├── xf_config.h
├── xf_types.h
│
├── app/
│   ├── Application.hpp
│   └── Application.cpp
│
├── ui/
│   ├── Ui.hpp
│   └── Ui.cpp
│
├── games/
│   ├── Game.hpp
│   ├── GameManager.hpp
│   ├── GameManager.cpp
│   ├── GameRegistry.hpp
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

---

## 参考项目与许可

- https://github.com/cwm-peace/xiaofang
- https://github.com/LittleGuest/xiaofang

许可与第三方说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

两个上游版本的合并取舍和历史问题见 [docs/MERGE_NOTES.md](docs/MERGE_NOTES.md)。
