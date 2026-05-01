# miniCore

`miniCore` 是从原始 `exp-core` 中裁剪出来的最小可复用内核, 只保留以下能力:

- 进程内 Pub/Sub Broker
- Topic 层级匹配
- 零拷贝消息对象
- OSAL 抽象层 (Linux / FreeRTOS / RT-Thread)
- 可选 RPC

当前仓库目标是作为一个独立、轻量、可直接嵌入其他项目的 C 静态库使用。

## 特性

- 纯 C11
- Linux 下仅依赖 `pthread` 与 `librt`; FreeRTOS / RT-Thread 由 BSP 工程提供内核头文件与链接
- 支持同步队列式订阅
- 支持基于 Topic 的请求/响应 RPC
- Linux 配置下仓库内自带可运行示例

## 目录结构

```text
miniCore/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── epx_config.h
│   ├── epx_types.h
│   ├── core/
│   ├── osal/
│   ├── framework/
│   └── minimal/
├── src/
│   ├── core/
│   ├── framework/
│   └── osal/
│       ├── linux/
│       ├── freertos/
│       └── rtthread/
└── examples/
    ├── minimal_pub_sub.c
    └── minimal_rpc.c
```

## 构建

### 默认构建 (Linux OSAL)

```bash
cmake -S . -B build
cmake --build build -j4
```

默认行为:

- `MINICORE_OSAL_PLATFORM` 为 `LINUX`
- 构建静态库 `minicore`
- 构建示例 `minicore_pub_sub` 与 `minicore_rpc`

### 选择 OSAL 后端

通过缓存变量 `MINICORE_OSAL_PLATFORM` 选择实现: `LINUX` (默认), `FREERTOS`, `RTTHREAD`. 未配置或为空时 CMake 会默认使用 `LINUX`。

```bash
cmake -S . -B build -DMINICORE_OSAL_PLATFORM=FREERTOS
cmake --build build -j4
```

FreeRTOS 与 RT-Thread 模式下, 需在**上层工程**中为 `minicore` 配置 RTOS 头文件搜索路径 (例如 `FreeRTOS.h`, `rtthread.h`) 并链接对应 BSP / 内核库; 本仓库的 CMake 仅编译 OSAL 源文件, 不拉取 RTOS 源码。

编译定义: 链接 `miniCore::minicore` 时会带上 `EPX_PLATFORM_LINUX`, `EPX_PLATFORM_FREERTOS` 或 `EPX_PLATFORM_RTTHREAD` 之一, 便于应用侧条件编译。

### 关闭 RPC

```bash
cmake -S . -B build -DMINICORE_ENABLE_RPC=OFF
cmake --build build -j4
```

### 关闭示例

```bash
cmake -S . -B build -DMINICORE_BUILD_EXAMPLES=OFF
cmake --build build -j4
```

说明: 桌面可执行示例仅在 `MINICORE_OSAL_PLATFORM=LINUX` 且开启 `MINICORE_BUILD_EXAMPLES` 时生成; 选用 FreeRTOS / RT-Thread 时通常在 BSP 工程中嵌入静态库, 而不使用仓库内示例目标。

## 运行示例

```bash
./build/minicore_pub_sub
./build/minicore_rpc
```

## 快速使用

推荐入口头文件:

```c
#include "minimal/epx_minimal.h"
```

### Pub/Sub

```c
epx_err_t ret;
epx_os_queue_t queue = NULL;
epx_msg_t msg = NULL;

ret = epx_broker_init();
ret = epx_os_queue_create(&queue, sizeof(epx_msg_t), 8);
ret = epx_sub("sensor/temp", queue);
ret = epx_pub("sensor/temp", "36.5", 5);

ret = epx_os_queue_recv(queue, &msg, 1000);
if (ret == EPX_OK && msg != NULL) {
    void* payload = epx_msg_payload(msg);
    epx_msg_release(msg);
}
```

### RPC

启用 RPC 时, 需要构建时打开 `MINICORE_ENABLE_RPC=ON`.

```c
epx_rpc_register("math/add", add_handler, NULL);
epx_rpc_call("math/add", &req, sizeof(req), &resp, sizeof(resp), 1000);
```

完整示例见:

- `examples/minimal_pub_sub.c`
- `examples/minimal_rpc.c`

## 集成到其他项目

最简单的方式是把 `miniCore` 作为子目录引入:

```cmake
add_subdirectory(third_party/miniCore)
target_link_libraries(your_app PRIVATE miniCore::minicore)
```

FreeRTOS / RT-Thread 工程中一般先设定 OSAL 再 `add_subdirectory`:

```cmake
set(MINICORE_OSAL_PLATFORM FREERTOS CACHE STRING "" FORCE)
add_subdirectory(third_party/miniCore)
target_include_directories(minicore PRIVATE ${YOUR_FREERTOS_INCLUDE_DIRS})
target_link_libraries(your_app PRIVATE miniCore::minicore /* BSP / RTOS libs */)
```

如果只需要 Broker, 可以关闭 RPC:

```cmake
set(MINICORE_ENABLE_RPC OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/miniCore)
target_link_libraries(your_app PRIVATE miniCore::minicore)
```

## 当前边界

当前 `miniCore` 明确不包含以下能力:

- HAL
- Gateway
- KV
- OTA
- App Manager

仓库优先保证 Linux 下快速接入与原型验证; OSAL 已通过 `src/osal/freertos/` 与 `src/osal/rtthread/` 提供 MCU 常见 RTOS 适配, 具体芯片仍需由 BSP 提供时钟、链接脚本与外设驱动。

## 注意事项

- 当前对外 API 仍沿用 `epx_*` 命名, 便于从原仓库平滑迁移
- `include/epx_config.h` 是当前最小配置入口
- 移植到其他 RTOS 或裸机时, 优先对照 `include/osal/` 接口实现新的 `src/osal/<平台>/` 源文件, 或通过 `MINICORE_OSAL_PLATFORM` 选用已有后端
