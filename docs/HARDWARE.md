# XiaoFang_Kuranda 硬件规划

## MCU

正式目标：**ESP32-WROOM-32D（经典 ESP32）**。它没有 ESP32-S3 的原生 USB Serial/JTAG，因此下载和日志仍走 UART0。

## GPIO 分配

| 子系统 | GPIO | 说明 |
|---|---:|---|
| WS2812B DIN | 18 | RMT |
| MPU6050 SDA / SCL | 21 / 22 | I2C0 |
| MPU6050 INT | 33 | RTC GPIO，EXT0 唤醒 |
| 压电蜂鸣器 | 25 | LEDC |
| 4G RX / TX | 16 / 17 | UART2 |
| 4G RTS / CTS | 13 / 14 | 可选硬件流控 |
| 4G DTR / PWRKEY / RI | 26 / 27 / 32 | 预留 |
| Battery ADC | 34 | ADC1，输入专用 |
| Microphone ADC | 35 | ADC1，输入专用 |
| UART0 TX / RX | 1 / 3 | 下载与日志 |

## 需要避开的 GPIO

- GPIO6–11：模块内部 SPI Flash。
- GPIO0/2/5/12/15：启动绑带相关，避免挂关键负载。
- GPIO34–39：输入专用，无内部上下拉；本项目只用 34/35 做模拟输入。

## WS2812B

8×8 Z 型排列：

```text
row0:  0  1  2  3  4  5  6  7
row1: 15 14 13 12 11 10  9  8
row2: 16 17 18 19 20 21 22 23
...
```

建议使用独立 5V LED 电源、与 ESP32 共地、74AHCT125/74AHCT1G125 电平转换、DIN 串 220–470Ω，并在灯板入口放置 470–1000µF 储能电容。软件帧级限流不能替代正确的供电与走线设计。

## MPU6050 与低功耗

MPU6050 使用 GPIO21/22，INT 使用 GPIO33。进入 Deep Sleep 前固件会先配置 Motion Interrupt、清旧中断、确认 INT 为低，再启用 EXT0。任何一步失败都会取消休眠并恢复 MPU6050 正常模式。

## 4G 预留

```text
GPIO17 TX  -> MODEM RX
GPIO16 RX  <- MODEM TX
GPIO13 RTS
GPIO14 CTS
GPIO26 DTR
GPIO27 PWRKEY
GPIO32 RI
```

当前只实现 UART transport。PWRKEY/DTR/RI 时序、AT 指令和 PPP/TCP/MQTT 需在具体模组确定后实现。不同 4G 模组 UART IO 可能是 1.8V，必须查数据手册决定是否增加电平转换。

蜂窝模组的瞬时峰值负载较大，应单独评估稳压、储能和回流，不能直接从 ESP32 的小功率 3.3V LDO 分支供电。