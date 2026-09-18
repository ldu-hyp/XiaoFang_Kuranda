# 两个 xiaofang 版本的合并取舍

## 当前正式路线

- MCU：**ESP32-WROOM-32D**
- SDK：ESP-IDF
- 底层驱动：C
- Application / UI / Game：轻量 C++
- C++ 禁用 exception / RTTI，游戏层无 `new/delete`
- CI：`target=esp32`

## 已保留的核心能力

- 原版的重力交互、沙漏、温度/声音、睡眠与动作唤醒思路；
- RGB 显示、随机迷宫、推箱子、下一百层、躲避方块和 ESP-NOW Pong；
- NVS 持久化、WS2812 RMT 驱动和模块化 Game 架构。

## 已修复 / 改进

- Snake 使用 next-head 自撞判断；
- Dodge 障碍方向修正；
- Hourglass 粒子在下半部真实保留；
- Maze 使用 DFS + BFS 最远终点；
- 输入层增加 `dir_pressed / dir_repeat / dir_released`；
- 持续倾斜、角速度和加速度变化可刷新 inactivity timer；
- Maze / CubeMan / Dodge / Pong 支持保持倾斜自动重复；
- GameResult 区分 Success / Failure / Timeout / Disconnected；
- 蜂鸣器改为独立 FreeRTOS Sound Task，不阻塞游戏循环；
- ESP-NOW 初始化失败完整回滚；
- Pong 增加 JOIN 重发与 START/状态确认，消除假配对窗口；
- Deep Sleep 前验证 MPU6050 Motion Interrupt 与 GPIO33 电平；
- 4G UART2 预留 RTS/CTS。

## 暂缓

- 音乐频谱：GPIO35 预留 ADC1，后续使用固定采样率 ADC continuous/DMA + FFT。
- 电池/充电：GPIO34 预留 ADC1，等待实际分压与电源管理方案。
- 4G 协议：等待具体模组后实现型号层。