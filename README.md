# ESP32-S3 FreeRTOS 多任务数据采集与网络通信系统

基于 ESP32-S3、ESP-IDF 和 FreeRTOS 实现的多任务数据采集与网络通信系统。

系统通过 I2C 驱动 QMA6100P 加速度传感器，以固定周期采集三轴加速度数据，经过独立数据处理任务计算加速度模长，并通过 Wi-Fi / UDP 发送至上位机。

项目重点围绕 FreeRTOS 构建任务间通信、同步、状态控制、周期调度和运行监控机制。

## 1. 系统架构

```text
                         ┌───────────────┐
                         │   QMA6100P    │
                         │      I2C      │
                         └───────┬───────┘
                                 │
                                 ▼
                          ┌────────────┐
                          │ SensorTask │
                          │ Priority 5 │
                          │   10 Hz    │
                          └─────┬──────┘
                                │
                              Queue
                                │
                                ▼
                         ┌─────────────┐
                         │ ProcessTask │
                         │ Priority 4  │
                         └──────┬──────┘
                                │
                              Queue
                                │
                                ▼
                          ┌────────────┐
                          │  UdpTask   │
                          │ Priority 3 │
                          └─────┬──────┘
                                │
                         Wi-Fi / lwIP / UDP
                                │
                                ▼
                            PC 上位机


 GPIO0
   │
   ▼
 GPIO ISR
   │
   │ Task Notification
   ▼
┌─────────────┐
│ ControlTask │
│ Priority 4  │
└──────┬──────┘
       │
       │ Event Group
       ▼
 SensorTask RUN / PAUSE


 SensorTask  ──┐
 ProcessTask ──┼────► system_stats ◄──── Mutex
 UdpTask     ──┘              │
                              │
                    ┌─────────▼─────────┐
 Software Timer ───►│    MonitorTask    │
 Task Notification  │    Priority 2     │
                    └─────────┬─────────┘
                              │
                              ▼
                        Task Watchdog
```

系统主要包含三条逻辑路径：

```text
数据路径：
QMA6100P
→ SensorTask
→ Queue
→ ProcessTask
→ Queue
→ UdpTask
→ Wi-Fi / UDP

控制路径：
GPIO ISR
→ Task Notification
→ ControlTask
→ Event Group
→ SensorTask

监控路径：
Software Timer
→ Task Notification
→ MonitorTask
→ system_stats / Mutex
→ Task Watchdog
```

## 2. 软件任务设计

| Task | Priority | 功能 |
|---|---:|---|
| SensorTask | 5 | 周期读取 QMA6100P，加速度数据进入采样队列 |
| ProcessTask | 4 | 获取采样数据并计算三轴加速度模长 |
| ControlTask | 4 | 处理 GPIO 按键事件并控制系统 RUN / PAUSE |
| UdpTask | 3 | 通过 Wi-Fi / UDP 向上位机发送处理后的数据 |
| MonitorTask | 2 | 周期统计系统运行状态并完成任务健康监控 |

SensorTask 使用 `vTaskDelayUntil()` 实现固定周期采样，避免普通相对延时带来的累计周期漂移。

## 3. FreeRTOS 机制

### 3.1 Queue

系统使用两级 Queue 构建数据流水线：

```text
SensorTask
    ↓
sensor_queue
    ↓
ProcessTask
    ↓
processed_queue
    ↓
UdpTask
```

生产者和消费者之间通过 Queue 完成数据传递，使采集、处理和网络通信任务彼此解耦。

当下游任务暂时无法及时处理数据时，Queue 可以提供一定的数据缓冲能力；队列满时采用丢弃当前实时数据的策略，并记录对应统计信息。

### 3.2 Task Notification

Task Notification 用于轻量级的一对一事件通知。

GPIO 按键中断路径：

```text
GPIO ISR
    ↓
vTaskNotifyGiveFromISR()
    ↓
ControlTask
```

ISR 中只负责发送通知，不执行按键消抖和复杂业务逻辑。

ControlTask 被唤醒后完成：

```text
软件消抖
→ 按键状态确认
→ RUN / PAUSE 状态切换
```

系统监控路径：

```text
Software Timer
    ↓
Timer Callback
    ↓
xTaskNotifyGive()
    ↓
MonitorTask
```

由于 Software Timer Callback 运行在 FreeRTOS Timer Service Task 上，而不是 ISR 中，因此使用普通版本的 `xTaskNotifyGive()`。

### 3.3 Event Group

系统使用 Event Group 表达持续性的运行状态。

```text
SYS_RUN_BIT = 1  → RUN
SYS_RUN_BIT = 0  → PAUSE
```

ControlTask 根据按键操作设置或清除 `SYS_RUN_BIT`。

SensorTask 使用：

```c
xEventGroupWaitBits(
    event_group,
    SYS_RUN_BIT,
    pdFALSE,
    pdTRUE,
    portMAX_DELAY
);
```

当系统进入 PAUSE 状态时，SensorTask 阻塞等待，不继续采集数据。

恢复 RUN 后重新建立 `vTaskDelayUntil()` 的时间基准，避免长时间暂停后出现补偿式连续采样。

### 3.4 Mutex

SensorTask、ProcessTask 和 UdpTask 都需要更新系统运行统计信息：

```text
SensorTask   ──┐
ProcessTask  ──┼──► system_stats
UdpTask      ──┘
                    ▲
                    │
               MonitorTask
```

使用同一个 Mutex 对共享的 `system_stats` 进行保护。

MonitorTask 的处理方式为：

```text
获取 Mutex
    ↓
复制 stats snapshot
    ↓
释放 Mutex
    ↓
输出日志
```

日志输出、网络通信等耗时操作均放在临界区之外，以缩短 Mutex 持有时间。

### 3.5 Software Timer

系统使用 FreeRTOS Software Timer 每 2 秒触发一次系统监控。

```text
Software Timer
      ↓
Timer Callback
      ↓
Task Notification
      ↓
MonitorTask
```

Software Timer 基于 FreeRTOS Tick 工作，其 Callback 运行在 Timer Service Task 上。

Callback 中只负责通知 MonitorTask，不执行日志、网络通信或其他耗时操作，避免长时间占用 Timer Service Task。

### 3.6 Task Watchdog

MonitorTask 注册 ESP-IDF Task Watchdog。

当前配置：

```text
Monitor 周期      : 2 s
Task WDT Timeout : 5 s
```

正常情况下：

```text
MonitorTask 被 Timer 唤醒
        ↓
完成系统统计
        ↓
esp_task_wdt_reset()
```

如果 MonitorTask 长时间无法取得运行进展，超过 Watchdog 超时时间，则 TWDT 输出异常任务信息和 CPU Backtrace。

项目中没有简单地将所有 Task 都加入固定周期 Watchdog。

例如 SensorTask 可能因为系统处于 PAUSE 状态而长期阻塞，ProcessTask 和 UdpTask 也可能因为没有输入数据而合法阻塞。

因此：

```text
长期没有运行
≠
一定发生故障
```

对事件驱动型任务，更合理的方式是根据系统运行状态和业务数据是否持续取得进展判断任务健康状态。

## 4. 数据采集与处理

QMA6100P 通过 I2C 与 ESP32-S3 通信。

当前配置：

```text
Measurement Range : ±8 g
Sensor ODR        : 100 Hz
MCU Sampling Rate : 10 Hz
```

SensorTask 每 100 ms 读取一次三轴加速度数据：

```text
x_g
y_g
z_g
```

同时记录：

```text
sequence
timestamp_us
```

ProcessTask 计算加速度模长：

```text
|a| = sqrt(x² + y² + z²)
```

处理后的数据结构继续通过 Queue 发送给 UdpTask。

## 5. 网络通信

ESP32-S3 工作于 Wi-Fi Station 模式。

Wi-Fi 连接采用 ESP-IDF Event Loop 驱动：

```text
WIFI_EVENT_STA_START
        ↓
esp_wifi_connect()

WIFI_EVENT_STA_DISCONNECTED
        ↓
清除 Connected 状态
        ↓
重新连接

IP_EVENT_STA_GOT_IP
        ↓
设置 Connected 状态
```

UdpTask 通过 lwIP Socket 创建 UDP Socket，并将处理后的传感器数据发送至上位机。

当 Wi-Fi 尚未获得有效 IP 时：

```text
当前实时数据
    ↓
直接丢弃
```

系统不会缓存大量历史数据，避免网络恢复后继续发送已经失去实时意义的数据。

UDP 数据格式示例：

```text
X=0.012 Y=-0.031 Z=0.998 |a|=0.999 g seq=1234 ts=123456789 us
```

## 6. 系统运行监控

系统维护以下运行统计：

```text
sensor_samples
processed_samples
process_dropped

udp_total
udp_sent
udp_wifi_drop
udp_send_fail
udp_format_fail
```

其中可以利用以下关系判断系统数据链路状态：

```text
sensor_samples ≈ processed_samples

processed_samples - process_dropped ≈ udp_total

udp_total =
    udp_sent
  + udp_wifi_drop
  + udp_send_fail
  + udp_format_fail
```

MonitorTask 每 2 秒读取一次统计快照，例如：

```text
sensor=279
processed=279
process_drop=0
UDP total=279
sent=239
wifi_drop=40
send_fail=0
format_fail=0
```

通过这些统计数据可以观察：

```text
采样速率是否正常
数据处理是否发生积压
Queue 是否发生丢包
Wi-Fi 是否已经连接
UDP 是否发送失败
```

## 7. QMA6100P 驱动设计

QMA6100P 驱动独立封装为 ESP-IDF Component。

初始化流程：

```text
创建 I2C Master Bus
        ↓
添加 QMA6100P Device
        ↓
Software Reset
        ↓
等待 OTP Ready
        ↓
检查 Chip Status
        ↓
Post Reset Init
        ↓
配置 ±8 g Range
        ↓
配置 100 Hz ODR
```

驱动区分：

```text
I2C Bus Handle
I2C Device Handle
完整初始化状态
```

只有所有初始化步骤全部成功后，驱动才进入 initialized 状态。

如果初始化过程中任一步骤失败：

```text
失败
 ↓
移除 I2C Device
 ↓
删除 I2C Bus
 ↓
Handle 清空
 ↓
恢复未初始化状态
```

避免出现底层 Handle 已经存在，但传感器实际没有完整初始化成功的半初始化状态。

## 8. 异常处理策略

不同类型的错误采用不同处理方式。

### 启动阶段关键错误

例如：

```text
QMA6100P 初始化失败
Queue 创建失败
Event Group 创建失败
Mutex 创建失败
Task 创建失败
```

这些错误意味着系统无法按照设计继续运行，因此视为启动阶段严重错误。

### 运行期可恢复错误

例如：

```text
Sensor Queue 满
Processed Queue 满
Wi-Fi 暂时断开
UDP sendto() 失败
传感器单次读取失败
```

这些情况不会立即终止整个系统，而是：

```text
记录错误
或
丢弃当前实时数据
或
等待下一次重试
```

UDP Socket 创建失败时采用延时重试机制，避免失败后进入高频空循环。

## 9. 关键设计考虑

项目中主要考虑了以下 RTOS 和嵌入式系统设计问题：

- 使用 Queue 在任务之间传递实际数据，而不是通过共享全局变量传递采样结果
- 使用 Mutex 保护多个 Task 访问的共享统计结构
- Mutex 临界区保持短小，不在持锁期间执行日志和网络操作
- GPIO ISR 中只发送 Task Notification，将业务处理放到任务上下文
- 使用 Event Group 表达 RUN / PAUSE 这种持续性的共享状态
- 使用 `vTaskDelayUntil()` 实现固定周期采样
- PAUSE 恢复后重新建立采样时间基准，避免补偿式连续执行
- Software Timer Callback 只负责事件通知，不执行复杂逻辑
- 根据通信语义分别选择 Queue、Task Notification、Event Group 和 Mutex
- 区分正常阻塞、任务饥饿、优先级反转和死锁
- 对可能合法长期阻塞的任务，不简单使用固定周期 Watchdog 喂狗
- 使用运行统计数据观察整个采集、处理和网络流水线的数据一致性
- 对驱动初始化过程进行状态管理，避免半初始化状态

## 10. FreeRTOS 机制选择

不同机制在项目中的用途如下：

| FreeRTOS 机制 | 项目中的用途 | 选择原因 |
|---|---|---|
| Queue | Sensor → Process → UDP 数据传递 | 需要传递实际数据并解耦生产者和消费者 |
| Task Notification | ISR → ControlTask、Timer → MonitorTask | 固定单一接收任务，只需要事件通知 |
| Event Group | RUN / PAUSE 状态 | 需要保存并共享持续性的状态 Bit |
| Mutex | system_stats 保护 | 多任务共享数据，需要互斥访问和优先级继承 |
| Software Timer | 周期触发系统监控 | 独立的软件定时事件，不需要单独 Task 自己计时 |
| Task Watchdog | MonitorTask 健康监控 | 检测本应周期取得进展的任务是否长时间失去响应 |

## 11. 开发环境

```text
MCU       : ESP32-S3
Board     : DNESP32S3
Framework : ESP-IDF v5.3.2
RTOS      : FreeRTOS
Sensor    : QMA6100P
Network   : Wi-Fi STA + lwIP UDP
Language  : C
```

## 12. 项目目录

```text
esp32s3-freertos-monitor/
├── components/
│   ├── qma6100p/
│   │   ├── include/
│   │   ├── qma6100p.c
│   │   └── CMakeLists.txt
│   │
│   └── wifi_sta/
│       ├── include/
│       ├── wifi_sta.c
│       └── CMakeLists.txt
│
├── main/
│   ├── include/
│   ├── sensor_task.c
│   ├── process_task.c
│   ├── udp_task.c
│   ├── control_task.c
│   ├── monitor_task.c
│   ├── my_esp32s3_app.c
│   └── CMakeLists.txt
│
├── CMakeLists.txt
└── sdkconfig
```