# XiaoFang_Kuranda

“小方”彩色重构版固件。当前正式硬件基线是 **ESP32-WROOM-32D**，不是 ESP32-S3。

- SDK：ESP-IDF 5.5.x
- 底层驱动：C
- Application / UI / Game：轻量 C++
- 显示：8×8 WS2812B，Z 型排列
- 姿态：MPU6050
- 声音：无源压电片 / 无源蜂鸣器
- 双机联机：ESP-NOW
- 未来 4G：UART2 预留完整控制与流控引脚

ESP32-WROOM-32D 没有原生 USB Serial/JTAG；下载和日志通过 UART0 GPIO1/GPIO3。

## GPIO

| 功能 | GPIO |
|---|---:|
| WS2812B DIN | 18 |
| MPU6050 SDA / SCL | 21 / 22 |
| MPU6050 INT | 33 |
| 压电蜂鸣器 | 25 |
| 4G RX / TX | 16 / 17 |
| 4G RTS / CTS | 13 / 14 |
| 4G DTR / PWRKEY / RI | 26 / 27 / 32 |
| Battery ADC | 34 |
| Microphone ADC | 35 |
| UART0 TX / RX | 1 / 3 |

详细硬件说明见 [docs/HARDWARE.md](docs/HARDWARE.md)。

## 当前功能

- 彩色沙漏、骰子、八卦、贪吃蛇；
- DFS+BFS 随机迷宫；
- 下一百层、推箱子、躲避方块；
- ESP-NOW 双机 Pong；
- MPU6050 倾斜/摇动/下甩/倒扣输入；
- NVS 设置与最高分；
- Deep Sleep + MPU6050 Motion Interrupt；
- 非阻塞蜂鸣器 Sound Task；
- 4G UART transport 预留。

MPU6050 菜单中显示的是**芯片内部温度**，不是准确环境温度。

## 本轮修复

### 输入

新增 `dir_pressed / dir_repeat / dir_released`。保持方向约 300ms 后，每约 130ms 自动重复。Maze、CubeMan、Dodge、Pong 支持连续控制；Sokoban 保持一步一动作。

`activity` 现在还参考持续非中性倾斜、角速度和加速度变化，因此持续操作不会再被 inactivity timer 误判为静止。

### Deep Sleep

进入休眠前必须成功配置 MPU6050 Motion Interrupt、清中断、确认 GPIO33 为低并成功启用 EXT0；失败则拒绝休眠，避免无法唤醒或立即唤醒循环。

### GameResult

游戏框架支持 Running / Success / Failure / Timeout / Disconnected。推箱子通关返回 Success，Pong 可区分胜负、配对超时和掉线。

### 蜂鸣器

蜂鸣器已改为 FreeRTOS Sound Task + Queue，音效不再在 Application/Game 中 `vTaskDelay()`，避免阻塞 Pong 和 ESP-NOW。

### ESP-NOW / Pong

ESP-NOW 初始化失败会按逆序释放已获得资源。Pong 使用 `SEEK → JOIN 重试 → START/有效状态确认 → CONNECTED`，Client 不再发送一次 JOIN 就直接认为已配对；Host 收到重复 JOIN 会重发 START/状态。

## 构建

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

GitHub Actions 同样使用 ESP-IDF v5.5.5、`target: esp32`。

## 真机仍需验证

1. WS2812B 首灯、Z 型方向和 GRB 顺序；
2. LED 最大亮度下的电源压降与温升；
3. MPU6050 安装方向及 tilt/shake/pause 阈值；
4. GPIO33 Motion Interrupt → Deep Sleep → 唤醒；
5. NVS 掉电保存；
6. 两台设备 Pong 的丢包恢复、掉线和重连；
7. Sound Task 队列在高频音效下的行为；
8. 最终 4G 模组的电源和 UART IO 电平。

## 软件目录

```text
main/
├── app/          # Application 状态机
├── ui/           # 菜单/数字/图标
├── games/        # GameManager + 各游戏
├── display.c
├── imu.c
├── input.c
├── buzzer.c
├── storage.c
├── power.c
├── network.c
└── modem.c
```

架构说明见 [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)，合并与修复记录见 [docs/MERGE_NOTES.md](docs/MERGE_NOTES.md)。

## 参考项目与许可

- https://github.com/cwm-peace/xiaofang
- https://github.com/LittleGuest/xiaofang

第三方许可见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。