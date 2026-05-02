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
- Linux 配置下可构建示例与 CTest 测试 (队列订阅, 通配回调, 订阅数上限等)

### 目录结构

```text
miniCore/
├── CMakeLists.txt
├── LICENSE
├── CHANGELOG.md
├── CONTRIBUTING.md
├── README.md
├── .github/workflows/
├── output/                 # 构建生成的可执行文件 (默认, 见下方构建说明)
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
│   ├── broker_smoke.c
│   ├── broker_wildcard_smoke.c
│   └── broker_trunc_smoke.c
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
- 构建静态库 `build/libminicore.a` (或生成器对应路径, 位于构建目录 `build/`)
- 示例与测试可执行文件输出到源码树下的 **`output/`** (缓存变量 `MINICORE_EXECUTABLE_OUTPUT_DIR`, 默认 `${CMAKE_SOURCE_DIR}/output`)
- 在 Linux 上默认开启 `MINICORE_BUILD_TESTS`, CTest 包含: `broker_smoke`, `broker_wildcard`, `broker_trunc`
- 构建示例 `minicore_pub_sub` 与 `minicore_rpc` (RPC 开启时)

自定义可执行文件输出目录:

```bash
cmake -S . -B build -DMINICORE_EXECUTABLE_OUTPUT_DIR=/path/to/dir
```

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

说明: 桌面可执行示例仅在 `MINICORE_OSAL_PLATFORM=LINUX` 且开启 `MINICORE_BUILD_EXAMPLES` 时生成, 选用 FreeRTOS / RT-Thread 时通常在 BSP 工程中嵌入静态库, 而不使用仓库内示例目标.

#### 持续集成与发布 (CI/CD)

- **[.github/workflows/ci.yml](.github/workflows/ci.yml)**: 在 `push` / `pull_request` 至 `master` 或 `main` 时, 于 Ubuntu 上矩阵构建 `MINICORE_ENABLE_RPC=ON|OFF`, 执行 **CTest**, 校验 **`cmake --install`** 产物, 并上传 **`libminicore.a`** 与示例/测试可执行文件为 Artifact. 可在 Actions 页手动 **Run workflow**.
- **[.github/workflows/release.yml](.github/workflows/release.yml)**: 推送 **`v*`** 标签 (如 `v0.1.0`) 时构建, 测试通过后打包 `include` / `lib` / CMake 包及 `LICENSE`, `README` 为 **`minicore-<tag>-linux-amd64.tar.gz`**, 并创建 **GitHub Release** (需仓库开启 Actions 写权限).

### 运行示例

可执行文件在 **`output/`** 下 (与 `build/` 分离):

```bash
./output/minicore_pub_sub
./output/minicore_rpc
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
ret = epx_subscribe_queue("sensor/temp", queue);
ret = epx_publish_data("sensor/temp", "36.5", 5);

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

### Topic 命名与使用

- **语法**: Topic 为以 `/` 分隔的层次路径, 建议从左到右由粗到细 (例如 `域/位置/信号名`).
- **发布与订阅**: 发布侧使用**具体路径**, 字符串中**不要**包含 `+` 或 `#`. 订阅过滤器可使用单段通配符 `+` 与多级通配符 `#`; `#` **只能出现在末尾一段** (如 `sensor/+/temp`, `sensor/#`), 否则 `epx_topic_subscribe` 返回参数错误. 行为与常见 MQTT Topic 规则一致, 便于日后对接网关或外部位 broker.
- **长度与深度**: 整串长度须小于 [include/epx_config.h](include/epx_config.h) 中的 `EPX_TOPIC_STR_MAX_LEN`, 段数不超过 `EPX_TOPIC_MAX_DEPTH` (含通配符展开前的路径段).
- **字符与风格**: 建议使用 **ASCII 小写字母, 数字与 `_`**, 段内避免空格与未转义的控制字符; 多词段可用 `snake_case` (如 `room_a/temp_c`) 或简短单词组合, 团队内保持统一即可.
- **命名空间 (可选)**: 若启用框架层并需与 RPC, 日志, 云边通道等划分边界, 可参考 [include/framework/epx_topic_names.h](include/framework/epx_topic_names.h) 中的前缀宏 (如 `sys/`, `home/`, `cloud/`), 在应用侧用宏或常量集中定义业务 Topic, 避免散落魔法字符串.
- **生命周期**: 经 `epx_msg_new` 发出的消息只保存 topic **指针**, 须保证该字符串存活至最后一次 `epx_msg_release`; 临时字符串请用 `epx_msg_new_copy`, 详见上文「注意事项」与 [include/core/epx_msg.h](include/core/epx_msg.h).

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
- `include/epx_config.h` 是当前最小配置入口; 可选钩子如 `EPX_BROKER_HOOK_SUBSCRIBER_TRUNCATED`, `EPX_OS_MEM_DEBUG_HOOK_*` 可在集成前 `#define` 覆盖默认空实现
- `epx_msg_new` 为零拷贝 topic 指针语义, 须保证 topic 字符串在最后一次 `epx_msg_release` 之前有效; 临时缓冲请用 `epx_msg_new_copy` (详见 `include/core/epx_msg.h` 注释)
- `epx_publish_data` 同一 topic 的队列订阅者超过 `EPX_MAX_SUBSCRIBERS_PER_TOPIC` 时仅投递前 N 路, 并返回 **`EPX_ERR_BUSY`**; 某路队列满则返回 **`EPX_ERR_QUEUE_FULL`**
- `epx_broker_deinit` 进行期间会拒绝新的 `epx_publish_data` / `epx_publish` / `epx_subscribe_queue` 等, 返回 **`EPX_ERR`**; 集成侧应先停业务线程再 `deinit`
- `epx_broker_is_idle` 为尽力检测, 不能替代显式同步
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
- On Linux, examples and CTest targets can be built (queue subscription, wildcard callbacks, subscriber cap)

### Repository layout

Same directory tree as in the Chinese section above (see the `text` block under 中文), including `output/` for built executables and the three test sources under `tests/`.

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
- Produces static library `minicore` under the build tree (e.g. `build/libminicore.a`)
- Example and test executables are written to **`output/`** at the project root (cache variable `MINICORE_EXECUTABLE_OUTPUT_DIR`, default `${CMAKE_SOURCE_DIR}/output`)
- On Linux, `MINICORE_BUILD_TESTS` is on by default; CTest runs `broker_smoke`, `broker_wildcard`, and `broker_trunc`
- Builds `minicore_pub_sub` and, when RPC is enabled, `minicore_rpc`

Override the executable output directory:

```bash
cmake -S . -B build -DMINICORE_EXECUTABLE_OUTPUT_DIR=/path/to/dir
```

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

Desktop examples are only built when `MINICORE_OSAL_PLATFORM=LINUX` and `MINICORE_BUILD_EXAMPLES` is on. For FreeRTOS / RT-Thread, embed the static library from your BSP; bundled examples are not used.

#### CI/CD

- **[.github/workflows/ci.yml](.github/workflows/ci.yml)**: On `push` / `pull_request` to `master` or `main`, builds on Ubuntu with `MINICORE_ENABLE_RPC` **ON** and **OFF**, runs **CTest**, verifies **`cmake --install`**, and uploads **`libminicore.a`** plus example/test binaries as workflow artifacts. **Run workflow** is available from the Actions tab.
- **[.github/workflows/release.yml](.github/workflows/release.yml)**: On **`v*`** tags (e.g. `v0.1.0`), builds and tests, then publishes **`minicore-<tag>-linux-amd64.tar.gz`** (install tree plus `LICENSE` and `README`) to a **GitHub Release** (requires Actions `contents: write`).

### Run examples

Binaries are under **`output/`** (not `build/`):

```bash
./output/minicore_pub_sub
./output/minicore_rpc
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

### Topic naming and usage

- **Syntax:** Topics are hierarchical paths separated by `/`, ordered from broad to narrow (e.g. `domain/place/signal`).
- **Publish vs subscribe:** Publish with a **concrete** path; do **not** put `+` or `#` in publish topic strings. Subscription filters may use `+` (one segment) and `#` (remainder); `#` must be the **final** segment (e.g. `sensor/+/temp`, `sensor/#`), otherwise `epx_topic_subscribe` returns a parameter error. This aligns with common MQTT topic rules for future gateway or external broker integration.
- **Length and depth:** The full string must fit under `EPX_TOPIC_STR_MAX_LEN` and segment count under `EPX_TOPIC_MAX_DEPTH` in [include/epx_config.h](include/epx_config.h).
- **Characters and style:** Prefer **ASCII lowercase**, digits, and `_`; avoid spaces and raw control characters inside segments; use `snake_case` or short word groups per segment and stay consistent across the project.
- **Namespaces (optional):** When using the framework layer and splitting RPC, logging, or cloud paths, consider the prefix macros in [include/framework/epx_topic_names.h](include/framework/epx_topic_names.h) (e.g. `sys/`, `home/`, `cloud/`). Centralize application topics in macros or constants instead of scattering literals.
- **Lifetime:** `epx_msg_new` stores the topic **pointer** only; keep the string alive until the last `epx_msg_release`, or use `epx_msg_new_copy` for ephemeral buffers (see **Notes** above and [include/core/epx_msg.h](include/core/epx_msg.h)).

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
- [include/epx_config.h](include/epx_config.h) is the minimal configuration entry point; optional hooks such as `EPX_BROKER_HOOK_SUBSCRIBER_TRUNCATED` and `EPX_OS_MEM_DEBUG_HOOK_*` can be overridden with `#define` before include
- `epx_msg_new` stores the topic pointer (zero-copy); keep the string valid until the last `epx_msg_release`; use `epx_msg_new_copy` for stack or short-lived buffers (see [include/core/epx_msg.h](include/core/epx_msg.h))
- `epx_publish_data` delivers to at most `EPX_MAX_SUBSCRIBERS_PER_TOPIC` queue subscribers per topic; if there are more, extras are skipped and **`EPX_ERR_BUSY`** is returned; a full subscriber queue yields **`EPX_ERR_QUEUE_FULL`**
- While `epx_broker_deinit` runs, new `epx_publish_data` / `epx_publish` / `epx_subscribe_queue` / etc. return **`EPX_ERR`**; stop worker threads before teardown
- `epx_broker_is_idle` is a best-effort hint, not a synchronization primitive
- To port to another RTOS or bare metal, implement `include/osal/` against new sources under `src/osal/<platform>/`, or select an existing backend via `MINICORE_OSAL_PLATFORM`
