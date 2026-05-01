# miniCore

`miniCore` 是从原始 `exp-core` 中裁剪出来的最小可复用内核, 只保留以下能力:

- 进程内 Pub/Sub Broker
- Topic 层级匹配
- 零拷贝消息对象
- Linux OSAL 实现
- 可选 RPC

当前仓库目标是作为一个独立、轻量、可直接嵌入其他项目的 C 静态库使用。

## 特性

- 纯 C11
- 最小依赖, Linux 下仅依赖 `pthread` 和 `rt`
- 支持同步队列式订阅
- 支持基于 Topic 的请求/响应 RPC
- 仓库内自带可运行示例

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
│   └── osal/linux/
└── examples/
    ├── minimal_pub_sub.c
    └── minimal_rpc.c
```

## 构建

### 默认构建

```bash
cmake -S . -B build
cmake --build build -j4
```

默认行为:

- 构建静态库 `minicore`
- 构建 `minicore_pub_sub`
- 构建 `minicore_rpc`

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
- 多平台 OSAL 适配

目前仓库主要面向 Linux 侧快速接入和原型验证。如果后续需要, 可以继续扩展出 `freertos` 或裸机版本的 OSAL 实现。

## 注意事项

- 当前对外 API 仍沿用 `epx_*` 命名, 便于从原仓库平滑迁移
- `include/epx_config.h` 是当前最小配置入口
- 若要移植到其他平台, 优先替换 `src/osal/linux/` 这层实现
