# AGENTS.md

本文件供 AI 编码助手加载使用，重点服务于**msDebug 出现问题时快速定位与分析**。包含：仓库定位、组件架构与数据流、症状→定位路径速查、日志/错误码、关键文件索引、编译与测试命令。

## 1. 仓库定位

MindStudio Debugger（msDebug）是基于 LLVM/LLDB 构建的**昇腾（Ascend）NPU 算子调试工具**，用于调试 NPU 侧运行的算子程序（Ascend C 算子：Vector/Cube/Mix）。

- 本仓是 **LLVM monorepo 的 fork**（含 llvm/clang/lld/lldb/compiler-rt 等），叠加昇腾定制代码。
- **定制代码以 `MS_DEBUGGER` 宏 / `MS_DEBUGGER=1` CMake 变量为编译守卫**，这是区分"上游代码"与"定制代码"的第一标志。排查问题时**优先看带 `MS_DEBUGGER` 的代码**。
- 产物：自解压 `.run` 安装包（`mindstudio-debugger_<version>_<arch>.run`），归档到 `artifacts/`。

## 2. 快速定位三问

拿到一个 msDebug 问题，先按下面三问缩小范围，再跳转到对应章节：

1. **问题发生在哪一端？** msDebug 是 `client（前端）` + `server（lldb-server 后端）` + `runtime_stub（注入库）` 三端架构，外加 `NPU 驱动`。
2. **问题在哪个阶段？** 启动/环境检查 → 设备使能 → kernel 上报 → 断点/单步 → 读写/展示。
3. **是哪个动作失败？** 断点不命中、单步异常、变量/内存/寄存器读不到、报某个错误码等。

## 3. 组件架构与数据流

```text
┌────────────────────────┐    GDB Remote Protocol    ┌───────────────────────────────┐
│ client（msdebug /      │◄─────────────────────────►│ server（lldb-server）          │
│ msdebug-mi）           │  qDevice*/m/x/Z/z/s/c 等   │  GDBRemoteCommunicationServerLLGS│
│ CommandObject*         │                           │  AscendProcessLinux            │
│ ProcessGDBRemote       │                           │  AscendThreadLinux             │
└────────────────────────┘                           │  AscendCommunicationServer     │
                                                     │  DeviceContext ── ioctl(SQ/CQ) │
┌────────────────────────┐    Unix Domain Socket     │        └───────► NPU 驱动      │
│ runtime_stub           │◄─────────────────────────►│  （ts_debug.ko / DEBUGGER_API） │
│ libruntime_stub.so     │  MSOP_SOCKET_PATH         └───────────────────────────────┘
│ (LD_PRELOAD 注入)      │
│ 劫持 rt*/aclrt* 接口   │
└────────────────────────┘
```

调试数据流：

1. 算子进程以 `LD_PRELOAD=libruntime_stub.so` 启动，CANN 运行时接口被劫持。
2. `rtSetDevice` → stub 上报设备信息 → server 创建 `DeviceContext`（`Factory::GetDeviceContext` 按 soc_version 实例化）。
3. `rtDevBinaryRegister` → stub 解析 kernel（SHA256/名/偏移/ELF）并缓存。
4. `rtKernelLaunch` → stub 上报 kernel 二进制与 `pc_base_addr` → server 入队 → client 拉取并解析断点。
5. 断点命中 → `DeviceContext` 监听线程收 `INTERRUPT_EVENT` → `MonitorBreakpoint` → 上报 client。
6. 前端命令（读写/单步/继续）→ `AscendProcessLinux` → `DeviceContext` → 驱动 → 设备。

## 4. 症状 → 定位路径速查表

| 症状 | 最可能层级 | 先看这里 |
|------|-----------|----------|
| 启动即失败、报 `0x10000`/`0x10100`/`0x20102` | stub 环境 | `lldb/tools/msdebug/runtime_stub.cpp`（`StubInit`/`EnvCheck`/`OpenRtLib`） |
| 报 `DRIVER_NOT_FOUND_ERR` 或设备被占用 | 驱动 | `DeviceContext::Init`、`/dev/drv_debug`、`/proc/debug_switch` |
| 断点不命中 | kernel 上报/断点 | `runtime_stub.cpp`（`rtKernelLaunch`→`SendKernelInfo`）、`AscendProcessLinux::HandleStubKernelInfo`、`SetDevice*Breakpoint`、地址解析 `pc_base_addr` |
| 单步/继续异常 | ThreadPlan / DeviceContext | `ThreadPlanStep*.cpp`、`AscendProcessLinux::SingleStep/Resume`、`DeviceContext::SingleStep/Resume` |
| 变量打印失败 | DWARF/表达式/内存 | `SymbolFileDWARF::ParseVariableDIE`（`DW_AT_address_class`）、`DWARFExpression::Evaluate`、`Value::GetValue` |
| 内存读失败（`memory read`） | 地址类路由 | `AscendProcessLinux::ReadMemoryWithoutTrap`、`DeviceContext::ReadMemory/ReadGlobalMemory/ReadLocalMemory` |
| 寄存器读失败 | 寄存器表/驱动 | `RegisterInfoPOSIX_ascend*`、`DeviceContext::ReadRegister` |
| 通信失败/挂死 | socket | `AscendCommunicationServer/Client`、`AscendDomainSocket`、`MSOP_SOCKET_PATH` |
| 多算子/多次 launch 状态错乱 | 状态管理 | `MapManager`、`HijackedLayerManager`、`m_device_binary_info_que` |

## 5. 日志与抓取手段

### 5.1 stub 端日志（被调试进程）

- 环境变量 `DEBUGGER_RT_STUB_LOG=1` 开启，日志直接 `printf` 到 stdout（`RT_STUB_LOG_INFO/WARNING/ERROR`，见 `rt_stub_log.h`）。
- 关键点日志：`rtSetDevice`、`rtKernelLaunch`、`SendKernelInfo`、socket 收发。

### 5.2 server 端日志（lldb-server）

- 在 msdebug 交互界面用 LLDB 日志命令，常用 category：`posix`、`process`、`breakpoints`、`thread`、`step`、`modules`、`dynamic-loader`、`lldb`。
- 例如：`log enable lldb process`、`log enable lldb breakpoints`。
- 源码日志宏：`GetLog(POSIXLog::Process)`、`GetLog(LLDBLog::Process)`、`GetLog(LLDBLog::Breakpoints)`、`GetLog(LLDBLog::Step)`。

### 5.3 环境/前置条件检查

- 驱动节点 `/dev/drv_debug` 必须存在（`EnvCheck`）。
- 调试开关 `/proc/debug_switch` 必须为 `debug_switch_status = 1`。
- `$ASCEND_TOOLKIT_HOME` 必须存在，`lib64/libruntime.so` 可加载。
- 抓 socket 路径：`MSOP_SOCKET_PATH=/tmp/msop_connect.{pid}.{timestamp}.sock`。

## 6. 错误码速查（`rt_stub_log.h`）

| 范围 | 含义 |
|------|------|
| `0x10000` | 环境变量错误（`ASCEND_TOOLKIT_HOME`/socket path 缺失） |
| `0x10100~0x1013F` | rt/aclrt/hal 符号加载失败（`dlsym` 失败，多为 CANN 版本不匹配） |
| `0x10200` | 设备 id 被占用 / 多次 set device 不一致 |
| `0x20000~0x20009` | rt/aclrt 接口调用返回异常 |
| `0x20100` | lldb 回复异常 |
| `0x20102` | `DRIVER_NOT_FOUND_ERR` 未找到调试 KO |
| `0x20200` | `OPEN_KO_ERR` 打开驱动失败（设备被占用） |
| `0x20202` | `INIT_DEBUG_MODE_ERR`（Profiling/Coredump 与调试互斥） |
| `0x20204` | `UNSUPPORTED_SOC_TYPE_ERR` 不支持的芯片 |
| `0x20205~0x20209` | server 解析 stub 消息失败（device/kernel/stream/header/ipc） |

## 7. 关键文件索引

| 组件 | 目录/文件 | 作用 |
|------|-----------|------|
| stub | `lldb/tools/msdebug/runtime_stub.cpp` | 劫持 `rt*` 接口，上报设备/kernel 信息 |
| stub | `lldb/tools/msdebug/aclrt_stub.cpp` | 劫持 `aclrt*` 接口 |
| stub | `lldb/tools/msdebug/AscendCommunicationClient.cpp` | socket 客户端 |
| stub | `lldb/tools/msdebug/rt_stub_log.h` | 错误码表 + 日志宏 |
| server | `lldb/source/Plugins/Process/Linux/AscendProcessLinux.cpp/.h` | 算子进程抽象（核心） |
| server | `lldb/source/Plugins/Process/Linux/AscendThreadLinux.cpp/.h` | 设备线程 |
| server | `lldb/source/Plugins/Process/Linux/AscendCommunicationServer.cpp/.h` | socket 服务端 + 消息解析 |
| server | `lldb/source/Plugins/Process/Linux/DeviceContext/DeviceContext.cpp/.h` | 驱动封装（SQ/CQ 命令） |
| server | `lldb/source/Plugins/Process/Linux/DeviceContext/Ascend{910B,950,310P}DeviceContext.cpp` | SoC 适配 |
| server | `lldb/source/Plugins/Process/gdb-remote/GDBRemoteCommunicationServerLLGS.cpp` | GDB 协议处理（qDevice*/m/x） |
| 公共 | `lldb/include/lldb/Utility/MessageDefines.h` | 三端共享消息结构体 |
| 寄存器 | `lldb/source/Plugins/Process/Utility/RegisterInfoPOSIX_ascend*` | 寄存器表 |
| 命令 | `lldb/source/Commands/CommandObjectAscend.cpp` | ascend 系列命令 |
| 步进 | `lldb/source/Target/ThreadPlanStep*.cpp` | step in/over/out |

## 8. 编译与测试

入口 `python build.py`：

| 命令 | 说明 |
|------|------|
| `python build.py` | 完整构建（拉依赖 + Release 编译 + 打包） |
| `python build.py local` | 跳过依赖下载（增量开发推荐） |
| `python build.py test` | 拉依赖 + 编译 + 执行单测 |
| `python build.py test local` | 跳过依赖 + 编译 + 单测 |

- 构建目录：`build/`（产品）、`build_ut/`（单测，`ENABLE_LLDB_TESTS=ON`）。
- 子项目：`ncurses → libedit → llvm_project（lldb/lldb-server/runtime_stub）→ msdebug_mi → package`。
- 产物：`output/` → `artifacts/`。
- pre-commit 钩子会自动修复行尾空格，**修复后需重新 `git add` 再提交**。

## 9. 提交与分支约定

- 提交信息风格：`[feature]` / `[bugfix]` / `[doc]` 中文前缀，如 `[bugfix] 修复断点不命中：...`。
- 改动尽量保持 `MS_DEBUGGER` 守卫，避免污染上游代码。
- 文档 wiki 位于 `docs/wiki/`（信息交互、断点/单步、变量/寄存器/内存读写等主题）。

## 10. 排查问题的最小步骤清单

1. 确认环境：`/dev/drv_debug`、`/proc/debug_switch`、`$ASCEND_TOOLKIT_HOME`、驱动/HDK 是否就绪。
2. 开日志：`DEBUGGER_RT_STUB_LOG=1`（stub）+ `log enable lldb process`（server）。
3. 复现并抓取：stub 打印的 `[Launch of Kernel ...]` / 错误码、server 日志、socket 消息。
4. 对照错误码表（§6）与症状速查表（§4）定位层级与文件。
5. 在对应文件用 `MS_DEBUGGER` 关键字快速定位定制逻辑，结合数据流（§3）梳理因果。
