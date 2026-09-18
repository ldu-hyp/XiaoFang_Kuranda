# 两个 xiaofang 版本的合并取舍

## 当前正式技术路线

从 v0.2 开始：

- MCU：ESP32-S3-WROOM-1
- SDK：ESP-IDF
- 底层：C
- Application / UI / Game：轻量 C++
- C++ 禁用 exception / RTTI
- 应用游戏层无动态分配

## 从 cwm-peace/xiaofang 保留

- 小方的整体交互范式；
- 沙漏真实堆积的体验；
- 温度、声音、睡眠/动作唤醒思路。

不再保留：

- ATmega328P 专用 LowPower；
- MAX7219 / LedControl；
- AT24C16 地图；
- 大型 .ino + 阻塞 while(flag)。

## 从 LittleGuest/xiaofang 保留

- RGB 显示思路；
- 模块化游戏；
- 随机迷宫；
- 推箱子、下一百层、躲避方块；
- ESP-NOW Pong。

不直接翻译 Rust/Embassy，而是重新实现成 ESP-IDF 架构。

## 修复/改进

- 贪吃蛇碰撞检测检查 next-head；
- 非增长时允许蛇头进入即将离开的尾格；
- 躲避方块真正从顶部向下移动；
- 沙漏粒子落下后保留在下半部；
- 迷宫使用 DFS 生成、BFS 选择最远终点；
- 迷宫 BFS/DFS 工作缓冲改为对象静态存储，避免在任务栈上放数 KB 临时数组；
- Pong 统一 Host 世界坐标，Client 渲染镜像；
- 持久化统一使用 NVS；
- 真正 Deep Sleep + MPU6050 Motion Interrupt；
- 4G UART 预留 RTS/CTS，便于后续 PPP/高吞吐数据链路。

## 暂缓功能

### 音乐频谱

GPIO5 已预留 ADC1 麦克风。加入麦克风后使用固定采样率 ADC continuous/DMA + FFT，不采用不定采样周期的 oneshot 循环。

### 电池 / 充电

GPIO4 已预留 ADC1。待充电管理芯片和电阻分压确定后加入。

### 4G 协议

硬件传输层已经准备好，但不猜测具体模组 AT 指令和开关机时序。
