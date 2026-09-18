# XiaoFang_Kuranda

面向 **ESP32-WROOM-32D + MPU6050 + 8×8 WS2812B** 的“小方”重构版固件。项目使用 **ESP-IDF + C** 重写，不再沿用 ATmega328P 大型 `.ino` 或 Rust/Embassy 运行时结构。

当前版本融合了两个参考项目的主要玩法与设计思路：

- 原版体验：沙漏、骰子、卦象、贪吃蛇、迷宫、温度、声音开关、动作交互、低功耗/动作唤醒思路。
- ESP32 扩展：RGB 显示、随机迷宫、推箱子、下一百层、躲避方块、ESP-NOW 双机 Pong。
- 新架构：统一输入事件、统一游戏生命周期、NVS 持久化、真正 Deep Sleep、4G UART 预留、WS2812 帧级电流预算。

> 音乐频谱未作为当前菜单功能启用，因为本次明确硬件中没有麦克风；GPIO35 已预留 ADC1 麦克风输入。
> 电池检测/充电动画也暂不启用，因为尚未明确充电管理芯片与电阻分压；GPIO34 已预留电池 ADC。

## 硬件

| 功能 | GPIO | 说明 |
|---|---:|---|
| WS2812B DIN | GPIO18 | 8×8 Z/蛇形排列 |
| MPU6050 SDA | GPIO21 | I2C0 |
| MPU6050 SCL | GPIO22 | I2C0 |
| MPU6050 INT | GPIO33 | RTC GPIO，可 Deep Sleep 唤醒 |
| 无源压电喇叭 | GPIO25 | LEDC PWM |
| 4G UART RX | GPIO16 | UART2，预留 |
| 4G UART TX | GPIO17 | UART2，预留 |
| 4G DTR | GPIO26 | 预留，具体电平按模块定义 |
| 4G PWRKEY | GPIO27 | 预留，具体时序按模块定义 |
| 4G RI | GPIO32 | 预留 |
| 电池采样 | GPIO34 | ADC1，预留 |
| 麦克风 | GPIO35 | ADC1，预留 |

GPIO1/3 保持 UART0 下载与日志；GPIO6–11 不使用（模块内部 SPI Flash）；尽量不占用启动绑带脚。

### WS2812B 硬件注意

标准 5V WS2812B 建议使用 74AHCT125/74AHCT1G125 做 3.3V→5V 数据电平转换，DIN 前串 220–470Ω，灯板入口放 470–1000µF 电容并与 ESP32 共地。64 颗灯理论最坏全白电流可接近数安培，本固件在 `display.c` 中增加了帧级电流预算（默认 800mA），但电源设计仍应保留安全裕量。

如果“喇叭”实际是低阻扬声器而不是高阻压电片，不要直接由 GPIO25 驱动，应增加三极管/MOSFET 驱动级。

## 功能

当前菜单：

1. 沙漏
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

交互默认采用倾斜控制：左右选择、向前倾确认；游戏中“下甩”作为返回菜单动作。阈值集中在 `main/xf_config.h`，最终 PCB 安装方向确定后只需统一调参。

## 构建

推荐 **ESP-IDF v5.5.5**。

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

WS2812 使用 Espressif `led_strip` managed component（RMT backend），依赖会由 Component Manager 自动下载。

## 目录

```text
main/
├── app.c              # 顶层状态机：菜单/游戏/温度/睡眠
├── display.c          # RGB framebuffer、Z型映射、亮度/电流限制、WS2812
├── imu.c              # MPU6050 I2C 驱动与 Motion Interrupt
├── input.c            # 倾斜/摇动/下甩/倒扣动作事件
├── buzzer.c           # LEDC 无源蜂鸣器
├── storage.c          # NVS 设置与最高分
├── power.c            # Deep Sleep + MPU6050 INT 唤醒
├── network.c          # ESP-NOW 数据通道
├── modem.c            # 未来 4G UART2 传输层
├── ui.c               # 菜单图标/数字/表情
└── games/
    ├── game.c
    ├── game_hourglass.c
    ├── game_dice.c
    ├── game_bagua.c
    ├── game_snake.c
    ├── game_maze.c
    ├── game_cube_man.c
    ├── game_sokoban.c
    ├── game_dodge.c
    └── game_pong.c
```

## 设计原则

游戏逻辑不直接依赖 WS2812、MPU6050 或 NVS。硬件输入先转成统一事件，游戏只处理方向/摇动等逻辑；游戏渲染只操作逻辑像素，Z 型物理序号、亮度和 RMT 时序由显示层处理。

详细的合并取舍与已修复问题见 `docs/MERGE_NOTES.md`，硬件资源见 `docs/HARDWARE.md`。

## 参考与许可

本项目为独立 C 重写，但参考了以下项目的功能行为、游戏设计和部分 MIT 许可图标/布局：

- https://github.com/cwm-peace/xiaofang
- https://github.com/LittleGuest/xiaofang

LittleGuest/xiaofang 的 MIT 许可文本保存在 `THIRD_PARTY_NOTICES.md`。第一个项目 README 标注“供个人爱好者学习研究，请勿商用”，因此本仓库避免直接复制其主体源码；若未来发布商业版本，应进一步确认原作者授权边界。
