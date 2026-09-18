# 硬件资源规划

## 固定分配

| 子系统 | 引脚 | 理由 |
|---|---|---|
| WS2812B | GPIO18 | 普通高速输出，远离启动脚；RMT 输出 |
| MPU6050 I2C | GPIO21/22 | ESP32 常用 I2C 引脚，布线直观 |
| MPU6050 INT | GPIO33 | RTC GPIO，可 EXT0 Deep Sleep 唤醒 |
| 压电蜂鸣器 | GPIO25 | LEDC PWM；RTC GPIO但本项目不拿它唤醒 |
| 4G UART2 | RX16/TX17 | ESP32-WROOM-32D 模块引出 U2RXD/U2TXD |
| 4G DTR/PWRKEY/RI | 26/27/32 | 为后续低功耗、开关机和来电/数据指示预留 |
| Battery ADC | GPIO34 | ADC1 输入专用脚，不与 Wi-Fi/ESP-NOW 的 ADC2 资源冲突 |
| Microphone ADC | GPIO35 | 同为 ADC1，未来频谱功能使用 |

## 不使用/慎用

- GPIO6–11：ESP32-WROOM-32D 内部 SPI Flash。
- GPIO1/3：UART0，保留下载与日志。
- GPIO0/2/5/12/15：涉及启动绑带或启动阶段电平，当前设计避免挂关键外设。
- GPIO34–39：输入专用；34/35 正好用于模拟输入。

## 4G 模块接口边界

当前只实现 `modem.c` UART 传输层，不假设具体模块型号，因此没有写死：

- PWRKEY 有效电平和保持时间；
- DTR 休眠语义；
- RI 有效电平；
- AT 指令集；
- 波特率；
- SIM/网络注册流程。

确定具体 4G 模块（例如 SIM7600、EC200、Air780 等）后，应在 `components/modem_driver` 或现有 `modem.c` 上增加型号层，而不要改变 UART2 GPIO16/17。

## WS2812B

逻辑矩阵坐标以左上角为 (0,0)，物理灯带按行 Z 型：

```text
row0:  0  1  2  3  4  5  6  7
row1: 15 14 13 12 11 10  9  8
row2: 16 17 18 19 20 21 22 23
...
```

`display.c` 唯一负责逻辑坐标到物理 index 的转换。

标准 5V WS2812B 建议：
- 74AHCT125/74AHCT1G125 电平转换；
- DIN 串 220–470Ω；
- 5V/GND 入口 470–1000µF；
- 大电流走线与 ESP32 数字地合理汇流；
- 必须共地。

## 低功耗

MPU6050 INT 接 GPIO33，休眠前切到 Motion Interrupt，ESP32 使用 EXT0 高电平唤醒。Deep Sleep 后 CPU/大部分 RAM/数字外设掉电，因此固件按“重新启动 + NVS/RTC 恢复”的模型设计，而不是假设函数原地继续执行。
