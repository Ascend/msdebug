# 预编译二进制库方案设计

## 1. 问题分析

当前编译问题：
- 每次构建msdebug都需要完整编译llvm和clang组件
- 编译时间长（估计2-4小时），占总编译时间95%+
- llvm和clang目录变化较少，适合预编译
- 用户无法选择编译模式

## 2. 解决方案设计

### 2.1 预编译库结构

预编译包只包含 LLVM/Clang 的静态库（`lib/libLLVM*.a`、`lib/libclang*.a`），
以及现编译 LLDB 所需的生成头文件和 cmake 配置（非"库"）：

```
prebuilt/<arch>/
├── lib/
│   ├── libLLVMCore.a            # 预编译 LLVM 静态库
│   ├── libLLVMSupport.a
│   └── ... (77 个 libLLVM*.a)
│   ├── libclangAST.a            # 预编译 Clang 静态库
│   └── ... (16 个 libclang*.a)
│   └── cmake/llvm/              # LLVMConfig.cmake 等（路径已重定位，可移植）
│   └── cmake/clang/             # ClangConfig.cmake 等
├── include/                     # 仅生成头（config.h / GenVT.inc 等）
├── tools/clang/include/         # 仅 Clang 生成头（DeclNodes.inc 等）
├── bin/llvm-tblgen               # 工具（LLDB standalone 编译必需，随包提供）
└── cmake/modules/               # LLVM 的 cmake 模块（LLVM-Config.cmake 等）
```

> **源码头文件不打包**：LLDB 在本仓库源码树（`llvm/include`、`clang/include`）现编译，
> 直接用仓库源码树的头文件。cmake 配置中的源码路径重定位为 `${MSDEBUG_SOURCE_DIR}/llvm` 等，
> 由 `cmake/Modules/LLVM.cmake` 以 `-DMSDEBUG_SOURCE_DIR` 传入。

### 2.2 核心功能

#### 2.2.1 预编译包制作
- `python build.py prebuild`：从源码全量编译 LLVM/Clang 并打包 `.a` 库 + 生成头 + cmake 配置
- cmake 配置内的绝对路径自动重定位为基于 `CMAKE_CURRENT_LIST_DIR` 的相对路径，保证可移植
- 生成 x86_64 / aarch64 两个架构的包

#### 2.2.2 构建模式选择
- 源码编译模式（默认）：完整编译 LLVM+Clang+LLDB
- 预编译库模式：LLVM/Clang 用预编译静态库，**LLDB、lldb-server、runtime_stub、msdebug、msdebug-mi 全部现编译**

### 2.3 预编译模式构建流程

由 `llvm_project` ExternalProject（`cmake/Modules/LLVM.cmake`）驱动，执行
`cmake/Modules/LLVM.cmake` 中 `llvm_project` 三阶段（均为内联 cmake 命令）：

1. **configure**：standalone 配置 LLDB（`cmake -S lldb`），`LLVM_DIR`/`Clang_DIR` 指向 prebuilt 包接口，`LLVM_TABLEGEN` 指向 prebuilt 包 `bin/llvm-tblgen`
2. **build**：现编译 `lldb` `lldb-server` `runtime_stub`
3. **install**：执行 `install-msdebug`，产出 `output/bin/msdebug.bin`、`output/lib/liblldb.so` 等

`msdebug-mi` 走原 `find_package(LLVM)` 流程，链接现编译的 `liblldb.so`。

### 2.4 构建优化预期

| 组件 | 源码编译时间 | 预编译库时间 | 优化比例 |
|------|-------------|-------------|----------|
| LLVM+Clang | ~180分钟 | 0（用预编译 .a） | 100% |
| llvm-tblgen 工具 | 包含在上 | 0（随 prebuilt 包提供） | - |
| LLDB 现编译 | 45分钟 | ~2分钟 | 96% |
| msdebug-mi | 15分钟 | 15分钟 | 0% |
| 打包 | ~2分钟 | ~2分钟 | 0% |
| **总计** | **~240分钟** | **~5分钟** | **98%** |

### 2.5 用户使用方式

```bash
# 1. 制作预编译包（从已有全量构建产物，架构固定为当前机器，可选 --tar 打可分发包）
python build.py prebuild --tar
#   或源码模式下用 CMake target 一键生成（全量编译 LLVM/Clang + 打包）
cmake --build <build_dir> --target package_prebuilt
#   产物：prebuilt/msdebug-prebuilt-llvm-<version>-<arch>.tar.gz（可上传 release）

# 2. 预编译构建（默认）
python build.py local
# 或
python build.py                     # 默认预编译构建

# 3. 完整源码编译（默认）
python build.py
```

### 2.6 release 分发闭环

`package_prebuilt` CMake target（仅源码模式）串起"编译→打包"闭环：

```
cmake --build . --target package_prebuilt
  → llvm_project（全量源码编译 LLVM/Clang → build/llvm-build/）
  → python build.py prebuild（全量编译 + 拷贝 .a + 生成头 + 校验 + 打 tar）
  → 产物：msdebug-prebuilt-llvm-<version>-<arch>.tar.gz
```

- tar 包内含版本元数据（`PREBUILT_VERSION`，取自 `package/conf/version.info`）
- tar 包可上传 release，配合 `USE_PREBUILT_LLVM` 构建模式在其他机器快速构建
- `build.py prebuild` 校验关键接口文件（LLVMConfig.cmake、GenVT.inc、DeclNodes.inc 等），
  缺失则报错，保证包完整性

### 2.6 关键设计说明

1. **LLVM.cmake 不再 `return()` 跳过**：`llvm_project` target 始终存在，
   依赖链（`build_debugger → llvm_project → install-msdebug → output/ → package`）完整，
   打包不再失败。
2. **`llvm-tblgen` 随 prebuilt 包提供**：LLDB driver 编译需要 `-gen-opt-parser-defs`
   （`Options.inc`），该 backend 在 llvm-tblgen 中。prebuilt 包 `bin/` 直接携带该工具，
   避免构建时现编译 68 个源文件。
3. **接口文件可移植**：cmake 配置中的绝对路径重定位为相对 `CMAKE_CURRENT_LIST_DIR`，
   prebuilt 包可整体搬迁到任意路径。
4. **LLDB standalone 构建**：`cmake -S lldb` 独立配置，
   LLVM/Clang 不在 ninja 构建图内，天然不重编译。
