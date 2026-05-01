# miniCore

[中文](#中文) | [English](#english)

---

## 中文

`miniCore` 是轻量级 **进程内 Pub/Sub 消息内核**, 提供 Topic 层级匹配, 零拷贝风格的消息对象, 可替换的 OSAL (Linux / FreeRTOS / RT-Thread) 以及可选 RPC.

本仓库为**独立项目**, 以 C 静态库形式嵌入其他应用或固件工程.

**许可:** [Apache License 2.0](LICENSE)

### 特性

- 纯 C11
- Linux 下仅依赖 `pthread` 与 `librt`, FreeRTOS / RT-Thread 由 BSP 工程提供内核头文件与链接
- 支持同步队列式订阅
- 支持基于 Topic 的请求/响应 RPC
- Linux 配置下可构建示例与 CTest 冒烟测试

### 目录结构

```text
miniCore/
├── CMakeLists.txt
├── LICENSE
├── CHANGELOG.md
├── CONTRIBUTING.md
├── README.md
├── .github/workflows/
├── include/
│   ├── epx_config.h
│   ├── epx_types.h
│   ├── minicore_version.h
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
├── tests/
│   └── broker_smoke.c
└── examples/
    ├── minimal_pub_sub.c
    └── minimal_rpc.c
```

### 版本

当前版本见 [include/minicore_version.h](include/minicore_version.h), 与 `CMakeLists.txt` 中 `project(VERSION ...)` 保持一致. 变更记录见 [CHANGELOG.md](CHANGELOG.md).

### 构建

#### 默认构建 (Linux OSAL)

```bash
cmake -S . -B build -DMINICORE_BUILD_TESTS=ON
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

默认行为:

- `MINICORE_OSAL_PLATFORM` 为 `LINUX`
- 构建静态库 `minicore`
- 在 Linux 上默认开启 `MINICORE_BUILD_TESTS` (CTest `broker_smoke`)
- 构建示例 `minicore_pub_sub` 与 `minicore_rpc` (RPC 开启时)

#### 选择 OSAL 后端

通过缓存变量 `MINICORE_OSAL_PLATFORM` 选择实现: `LINUX` (默认), `FREERTOS`, `RTTHREAD`. 未配置或为空时 CMake 会默认使用 `LINUX`.

```bash
cmake -S . -B build -DMINICORE_OSAL_PLATFORM=FREERTOS
cmake --build build -j4
```

FreeRTOS 与 RT-Thread 模式下, 需在**上层工程**中为 `minicore` 配置 RTOS 头文件搜索路径 (例如 `FreeRTOS.h`, `rtthread.h`) 并链接对应 BSP / 内核库, 本仓库的 CMake 仅编译 OSAL 源文件, 不拉取 RTOS 源码.

编译定义: 链接 `miniCore::minicore` 时会带上 `EPX_PLATFORM_LINUX`, `EPX_PLATFORM_FREERTOS` 或 `EPX_PLATFORM_RTTHREAD` 之一, 便于应用侧条件编译.

#### 安装与 `find_package`

```bash
cmake -S . -B build
cmake --build build -j4
cmake --install build --prefix /path/to/prefix
```

安装后可通过 `find_package(miniCore CONFIG)` 使用导出的 `miniCore::minicore` 目标 (需将 `prefix` 加入 `CMAKE_PREFIX_PATH`).

#### 关闭 RPC

```bash
cmake -S . -B build -DMINICORE_ENABLE_RPC=OFF
cmake --build build -j4
```

#### 关闭示例或测试

```bash
cmake -S . -B build -DMINICORE_BUILD_EXAMPLES=OFF
cmake -S . -B build -DMINICORE_BUILD_TESTS=OFF
```

说明: 桌面可执行示例仅在 `MINICORE_OSAL_PLATFORM=LINUX` 且开启 `MINICORE_BUILD_EXAMPLES` 时生成, 选用 FreeRTOS / RT-Thread 时通常在 BSP 工程中嵌入静态库, 而不使用仓库内示例目标. **CI 当前仅验证 Linux OSAL 构建与 CTest.**

### 运行示例

```bash
./build/minicore_pub_sub
./build/minicore_rpc
```

### 快速使用

推荐入口头文件:

```c
#include "minimal/epx_minimal.h"
```

#### Pub/Sub

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

#### RPC

启用 RPC 时, 需要构建时打开 `MINICORE_ENABLE_RPC=ON`.

```c
epx_rpc_register("math/add", add_handler, NULL);
epx_rpc_call("math/add", &req, sizeof(req), &resp, sizeof(resp), 1000);
```

完整示例见 `examples/minimal_pub_sub.c`, `examples/minimal_rpc.c`.

### 集成到其他项目

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

### 当前边界

当前 `miniCore` **不包含**以下能力:

- HAL
- Gateway
- KV
- OTA
- App Manager

仓库优先保证 Linux 下快速接入与原型验证, OSAL 已通过 `src/osal/freertos/` 与 `src/osal/rtthread/` 提供 MCU 常见 RTOS 适配, 具体芯片仍需由 BSP 提供时钟, 链接脚本与外设驱动.

### 贡献

见 [CONTRIBUTING.md](CONTRIBUTING.md).

### 注意事项

- 对外 API 使用 `epx_*` 前缀以保持命名空间稳定
- `include/epx_config.h` 是当前最小配置入口
- 移植到其他 RTOS 或裸机时, 优先对照 `include/osal/` 接口实现新的 `src/osal/<平台>/` 源文件, 或通过 `MINICORE_OSAL_PLATFORM` 选用已有后端

---

## English

`miniCore` is a lightweight **in-process Pub/Sub messaging core** with hierarchical topic matching, zero-copy style message objects, a pluggable OSAL (Linux / FreeRTOS / RT-Thread), and optional RPC.

This repository is a **standalone** project, shipped as a C static library for embedding in applications or firmware.

**License:** [Apache License 2.0](LICENSE)

### Features

- C11 only
- On Linux, depends only on `pthread` and `librt`; FreeRTOS / RT-Thread builds require BSP-supplied RTOS headers and linking
- Queue-based synchronous subscriptions
- Topic-based request/response RPC
- On Linux, examples and a CTest smoke test can be built

### Repository layout

Same directory tree as in the Chinese section above (see the `text` block under 中文).

### Versioning

The version is defined in [include/minicore_version.h](include/minicore_version.h) and must match `project(VERSION ...)` in [CMakeLists.txt](CMakeLists.txt). See [CHANGELOG.md](CHANGELOG.md) for release notes.

### Build

#### Default (Linux OSAL)

```bash
cmake -S . -B build -DMINICORE_BUILD_TESTS=ON
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

By default:

- `MINICORE_OSAL_PLATFORM` is `LINUX`
- Produces static library `minicore`
- On Linux, `MINICORE_BUILD_TESTS` is on by default (CTest `broker_smoke`)
- Builds `minicore_pub_sub` and, when RPC is enabled, `minicore_rpc`

#### Choosing the OSAL backend

Set cache variable `MINICORE_OSAL_PLATFORM` to `LINUX` (default), `FREERTOS`, or `RTTHREAD`. If unset or empty, CMake defaults to `LINUX`.

```bash
cmake -S . -B build -DMINICORE_OSAL_PLATFORM=FREERTOS
cmake --build build -j4
```

For FreeRTOS and RT-Thread, your **top-level project** must add RTOS include paths (e.g. `FreeRTOS.h`, `rtthread.h`) and link the BSP/kernel. This repo only compiles OSAL sources and does not vendor an RTOS.

Compile definitions: linking `miniCore::minicore` exposes `EPX_PLATFORM_LINUX`, `EPX_PLATFORM_FREERTOS`, or `EPX_PLATFORM_RTTHREAD` for conditional compilation.

#### Install and `find_package`

```bash
cmake -S . -B build
cmake --build build -j4
cmake --install build --prefix /path/to/prefix
```

After install, use `find_package(miniCore CONFIG)` and target `miniCore::minicore` (add `prefix` to `CMAKE_PREFIX_PATH`).

#### Disable RPC

```bash
cmake -S . -B build -DMINICORE_ENABLE_RPC=OFF
cmake --build build -j4
```

#### Disable examples or tests

```bash
cmake -S . -B build -DMINICORE_BUILD_EXAMPLES=OFF
cmake -S . -B build -DMINICORE_BUILD_TESTS=OFF
```

Desktop examples are only built when `MINICORE_OSAL_PLATFORM=LINUX` and `MINICORE_BUILD_EXAMPLES` is on. For FreeRTOS / RT-Thread, embed the static library from your BSP; bundled examples are not used. **CI currently validates only the Linux OSAL build and CTest.**

### Run examples

```bash
./build/minicore_pub_sub
./build/minicore_rpc
```

### Quick start

Recommended umbrella header:

```c
#include "minimal/epx_minimal.h"
```

#### Pub/Sub

Same sample code as in the Chinese section.

#### RPC

Enable RPC at configure time with `MINICORE_ENABLE_RPC=ON`.

Same RPC sample code as in the Chinese section.

See `examples/minimal_pub_sub.c` and `examples/minimal_rpc.c`.

### Integrating into another project

Typical `add_subdirectory` usage:

```cmake
add_subdirectory(third_party/miniCore)
target_link_libraries(your_app PRIVATE miniCore::minicore)
```

FreeRTOS / RT-Thread example:

```cmake
set(MINICORE_OSAL_PLATFORM FREERTOS CACHE STRING "" FORCE)
add_subdirectory(third_party/miniCore)
target_include_directories(minicore PRIVATE ${YOUR_FREERTOS_INCLUDE_DIRS})
target_link_libraries(your_app PRIVATE miniCore::minicore /* BSP / RTOS libs */)
```

Broker-only (no RPC):

```cmake
set(MINICORE_ENABLE_RPC OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/miniCore)
target_link_libraries(your_app PRIVATE miniCore::minicore)
```

### Non-goals

The following are **out of scope** for `miniCore`:

- HAL
- Gateway
- KV store
- OTA
- App manager

Linux is the primary environment for quick integration; OSAL folders under `src/osal/freertos/` and `src/osal/rtthread/` target common RTOS ports; per-chip clocks, linker scripts, and drivers remain the BSP's responsibility.

### Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

### Notes

- Public APIs use the `epx_*` prefix for a stable namespace
- [include/epx_config.h](include/epx_config.h) is the minimal configuration entry point
- To port to another RTOS or bare metal, implement `include/osal/` against new sources under `src/osal/<platform>/`, or select an existing backend via `MINICORE_OSAL_PLATFORM`
