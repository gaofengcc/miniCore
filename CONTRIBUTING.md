# Contributing to miniCore

[中文](#中文) | [English](#english)

---

## 中文

### 构建

```bash
cmake -S . -B build -DMINICORE_BUILD_TESTS=ON
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

常用 CMake 选项: `MINICORE_ENABLE_RPC`, `MINICORE_BUILD_EXAMPLES`, `MINICORE_OSAL_PLATFORM` (默认 `LINUX`). 当前 CI 仅验证 **Linux** OSAL 路径.

### CI/CD

- **CI**: [.github/workflows/ci.yml](.github/workflows/ci.yml) 在 `master` / `main` 的 push 与 PR 上运行; 可 `workflow_dispatch` 手动触发. CI 将可执行文件输出到 **`output-ci`** 目录, 避免污染本地默认 `output/`.
- **Release (CD)**: 推送符合 `v*` 的 **tag** 后 [.github/workflows/release.yml](.github/workflows/release.yml) 会构建, 测试, 打包安装树并创建 **GitHub Release**. 发版前请同步版本号: [include/minicore_version.h](include/minicore_version.h), `CMakeLists.txt` 中 `project(VERSION ...)`, [CHANGELOG.md](CHANGELOG.md).

### 提交合并请求 (Pull request)

- 每次改动尽量单一主题, 风格与仓库一致 (C11, 四个空格缩进, `snake_case` 与对外 `epx_*` API).
- 对外发布版本时, 请同步更新 [include/minicore_version.h](include/minicore_version.h), [CMakeLists.txt](CMakeLists.txt) 中的 `project(VERSION ...)`, 以及 [CHANGELOG.md](CHANGELOG.md).

### 贡献者许可协议 (CLA)

本仓库**不**使用 CLA. 你对提交内容保留著作权, 一旦发起 Pull request, 即表示你愿意按与本仓库相同的条款授权 ([LICENSE](LICENSE)).

---

## English

### Build

```bash
cmake -S . -B build -DMINICORE_BUILD_TESTS=ON
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Relevant CMake options: `MINICORE_ENABLE_RPC`, `MINICORE_BUILD_EXAMPLES`, `MINICORE_OSAL_PLATFORM` (default `LINUX`). Continuous integration only exercises the **Linux** OSAL build.

### CI/CD

- **CI**: [.github/workflows/ci.yml](.github/workflows/ci.yml) runs on pushes and PRs to `master` / `main`, plus `workflow_dispatch`. Executables are written to **`output-ci`** during CI to avoid clobbering a local `output/` tree.
- **Release (CD)**: Pushing a **`v*`** tag triggers [.github/workflows/release.yml](.github/workflows/release.yml) to build, test, tarball the install prefix (plus `LICENSE` / `README`), and create a **GitHub Release**. Bump [include/minicore_version.h](include/minicore_version.h), `project(VERSION ...)` in [CMakeLists.txt](CMakeLists.txt), and [CHANGELOG.md](CHANGELOG.md) before tagging.

### Pull requests

- Keep each change focused; follow the existing style (C11, 4-space indent, `snake_case` and the public `epx_*` API).
- For a user-facing release, bump [include/minicore_version.h](include/minicore_version.h), `project(VERSION ...)` in [CMakeLists.txt](CMakeLists.txt), and [CHANGELOG.md](CHANGELOG.md) together.

### Contributor License Agreement (CLA)

This project does **not** use a CLA. You keep copyright in your contributions; by opening a pull request you license them under the same terms as this repository ([LICENSE](LICENSE)).
