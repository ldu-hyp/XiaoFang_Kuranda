# XiaoFang_Kuranda 硬件规划

## 1. MCU

正式目标改为 **ESP32-S3-WROOM-1**。

固件本身不依赖 PSRAM，因此 N8、N8R2、N8R8 等模块配置可以按 PCB 成本与未来扩展选择。为了兼容更多带 PSRAM 的 S3 模块，本项目主动避开 GPIO26–37 作为外设 IO。

## 2. GPIO 分配

| 子系统 | GPIO | 说明 |
|---|---:|---|
| WS2812B DIN | 18 | RMT 输出 |
| MPU6050 SDA | 8 | I2C0 |
| MPU6050 SCL | 9 | I2C0 |
| MPU6050 INT | 7 | RTC GPIO，Deep Sleep EXT0 唤醒 |
| 压电蜂鸣器 | 10 | LEDC PWM |
| 4G RX | 16 | UART2 |
| 4G TX | 17 | UART2 |
| 4G DTR | 11 | 预留 |
| 4G PWRKEY | 12 | 预留 |
| 4G RI | 13 | 预留 |
| 4G RTS | 14 | 可选硬件流控 |
| 4G CTS | 15 | 可选硬件流控 |
| Battery ADC | 4 | ADC1 预留 |
| Microphone ADC | 5 | ADC1 预留 |
| USB D- | 19 | ESP32-S3 原生 USB |
| USB D+ | 20 | ESP32-S3 原生 USB |

## 3. 特意不占用的 GPIO

- GPIO19 / 20：原生 USB D- / D+。
- GPIO0 / 3 / 45 / 46：启动绑带相关，避免挂关键负载。
- GPIO26–37：为了兼容可能内部使用这些信号的 Flash/PSRAM 配置。
- GPIO43 / 44：保留 UART0 调试/救援下载选择。

## 4. WS2812B

8×8 Z 型排列：

```text
row0:  0  1  2  3  4  5  6  7
row1: 15 14 13 12 11 10  9  8
row2: 16 17 18 19 20 21 22 23
...
```

推荐电路：

- 5V LED 电源；
- ESP32-S3 与灯板共地；
- 74AHCT125 / 74AHCT1G125：3.3V → 5V 数据电平；
- DIN 串 220–470Ω；
- LED 电源入口放 470–1000µF 电容；
- 5V 大电流回路不要经过 ESP32 的细地线。

软件 `display.c` 还实现了帧级电流预算，默认限制到约 800mA，但它不能替代正确的电源设计。

## 5. MPU6050

MPU6050：

- SDA GPIO8；
- SCL GPIO9；
- INT GPIO7。

休眠前 MPU6050 切换 Motion Interrupt；GPIO7 作为 EXT0 唤醒源。最终阈值需要依据 PCB 中 MPU6050 的实际朝向、安装方式和外壳机械振动进行真机校准。

## 6. USB-C

ESP32-S3 GPIO19/GPIO20 直接提供原生 USB D-/D+。

PCB 应按 Espressif S3 硬件设计建议：

- D-/D+ 差分布线；
- 预留 22/33Ω 串联电阻；
- USB-C 设备端 CC1/CC2 使用合适的 Rd；
- ESD 防护靠近接口放置。

这样可以直接使用 USB Serial/JTAG 进行下载、日志和调试，不再强制需要 CH340/CP2102。

## 7. 4G 模块

UART2 固定：

```text
ESP32-S3 GPIO17 TX  -> MODEM RX
ESP32-S3 GPIO16 RX  <- MODEM TX
GPIO14 RTS
GPIO15 CTS
GPIO11 DTR
GPIO12 PWRKEY
GPIO13 RI
```

`modem_uart_init(baud, hardware_flow_control)` 可以按具体模块决定是否启用 RTS/CTS。

目前不锁定 SIM7600 / EC200 / Air780 等型号，因此 PWRKEY 脉冲宽度、DTR 休眠极性、RI 行为和 AT/PPP 流程都留给模块专用驱动。

### 4G 电源尤其重要

蜂窝模块的瞬时峰值电流通常远高于 ESP32，PCB 上应给 4G 电源独立规划足够的稳压能力、低 ESR 储能电容和回流路径。不能简单从 ESP32 的 3.3V LDO 分支给 4G 模块供电。

## 8. 模拟输入

Battery ADC 与未来麦克风均放在 ADC1 侧，避免无线工作时使用 ADC2 所带来的资源限制。

当前固件只做引脚预留；在充电管理芯片、电池分压和麦克风前端确定后再启用对应模块。
