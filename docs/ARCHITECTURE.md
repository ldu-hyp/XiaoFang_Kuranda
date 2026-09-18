# Software Architecture

## 目标

固件采用 **ESP-IDF + C/C++ 混合架构**。

原则：

1. 硬件和系统资源的拥有者用 C；
2. 产品状态机和多游戏对象用 C++；
3. 禁止应用层动态分配；
4. 禁用 C++ exceptions / RTTI；
5. 所有游戏通过统一接口，不直接操作 I2C、RMT、NVS；
6. 所有硬件 C API 通过 `c_api.hpp` 暴露给 C++。

## 分层

```text
Application (C++)
    |
    +-- UI (C++)
    |
    +-- GameManager (C++)
    |      |
    |      +-- Game abstract interface
    |             |
    |             +-- SnakeGame
    |             +-- MazeGame
    |             +-- HourglassGame
    |             +-- ...
    |
    +-- c_api.hpp
           |
           +-- display.c
           +-- imu.c
           +-- input.c
           +-- buzzer.c
           +-- storage.c
           +-- network.c
           +-- power.c
           +-- modem.c
```

## 为什么没有使用纯 C++

ESP-IDF 的底层 API 本身主要为 C API。驱动层保持 C 能让：

- 中断/任务资源所有权清晰；
- 无隐藏构造顺序；
- 可独立复用与测试；
- 与 ESP-IDF 示例和第三方模组 SDK 更直接兼容。

## 为什么上层使用 C++

九个以上游戏如果继续使用 C 函数表，会不断重复手工状态管理。C++ 的抽象基类让每个游戏天然拥有自己的状态和生命周期：

```cpp
class Game {
public:
    virtual esp_err_t start() = 0;
    virtual void update(const xf_input_t&, uint32_t dt) = 0;
    virtual void render() const = 0;
    virtual bool finished() const = 0;
};
```

所有游戏对象都是静态实例，不调用 `new/delete`。

## 实时策略

当前 8×8 游戏循环以 50ms 为基础 tick。WS2812 由 RMT 外设发送；应用层不 bit-bang。ESP-NOW 通过队列把回调数据交给游戏逻辑，不在无线回调里运行游戏。

未来 4G 建议使用独立 FreeRTOS task 处理 AT/PPP 收发，再通过队列或 event group 与 Application 通信。
