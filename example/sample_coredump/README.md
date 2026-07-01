# msDebug Coredump 解析样例介绍

## 概述

本样例演示如何使用 **msdebug** 工具加载并分析算子异常崩溃产生的 AI Core coredump 文件，定位导致崩溃的源码位置。

## 适用场景

- 算子上板运行时发生 AIC ERR（AI Core 异常崩溃），需要分析 coredump 文件定位崩溃原因。

## 支持的产品范围

- Ascend 950PR/Ascend 950DT
- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品

## 目录结构

```text
├── sample_coredump/
│   ├── CMakeLists.txt          // 编译工程文件
│   ├── add.asc                 // Ascend C 样例实现（含 coredump 触发注入）
│   ├── data_utils.h            // 数据读写工具
│   ├── scripts/
│   │   ├── gen_data.py         // 测试数据生成脚本
│   │   └── verify_result.py    // 结果验证脚本
│   └── README.md               // 本说明文件
```

## 样例描述

### 核心逻辑

本样例在 `add.asc` 中实现了一个基于 Double Buffer 优化的向量加法（Add）算子，使用 `LocalMemAllocator` 管理 UB 内存，通过 `SetFlag`/`WaitFlag` 进行流水同步。

### 注入异常

为演示 coredump 解析功能，在算子 CopyOut 阶段刻意注入了一处越界写入：

```cpp
// add.asc 第 84 行：
AscendC::DataCopy(gmC[i * tileSize + 100000000], bufC, tileSize);
// 正常应为：AscendC::DataCopy(gmC[i * tileSize], bufC, tileSize);
```

`+100000000` 导致 GM 写入地址远超出实际分配的缓冲区范围，触发 MTE_ERROR 并生成 coredump。

---

## 快速开始：从崩溃到修复

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_coredump/` 目录下执行：

```bash
mkdir -p build && cd build
cmake -DCMAKE_ASC_ARCHITECTURES=dav-2201 ..
make -j96
```

> **说明**：CMakeLists.txt 已默认配置 `-O2 -g`（`-O2` 保留 inline 以获取完整调用栈，`-g` 保留行号信息）。

### 1. 触发 coredump

设置环境变量启用异常 dump，然后运行算子：

```bash
export ASCEND_DUMP_SCENE="aic_err_detail_dump"
export ASCEND_DUMP_PATH="output"

python3 ../scripts/gen_data.py      # 生成输入数据
./add                               # 运行算子（将因越界写入崩溃）
```

算子崩溃后（exit 1），coredump 文件位于 `output/extra-info/data-dump/0/` 目录下。

### 2. 加载 coredump

```bash
msdebug --core <coredump_file>
```

加载后进入 msdebug 交互界面，崩溃原因为 **MTE_ERROR**（Memory Transfer Engine 异常），发生在 CopyOut 操作：

```text
(msdebug) target create --core "<coredump_file>"
Core file '<coredump_file>' (aarch64) was loaded.
[Switching to focus on Kernel _Z8add_demoPhS_S_S_, CoreId <N>, Type aiv]
* thread #1, stop reason = MTE_ERROR
    frame #0: 0x... DataCopyUB2GMImpl<float>(...) at kernel_operator_data_copy_impl.h:104:9
```

### 3. 定位异常代码行

执行 `bt` 查看调用栈，帧 #3 直接指向用户源码：

```text
(msdebug) bt
* thread #1, stop reason = MTE_ERROR
  * frame #0: ... DataCopyUB2GMImpl<float>(...) at kernel_operator_data_copy_impl.h:104:9
    frame #1: ... DataCopy<float>(...) at kernel_operator_data_copy_intf_impl.h:297:9
    frame #2: ... DataCopy<float>(...) at kernel_operator_data_copy_intf_impl.h:791:5
    frame #3: ... DoubleBufAdd::Process(...) at add.asc:84:13    ← 异常代码行
    frame #4: ... add_demo(...) at add.asc:104:8
```

根据帧 #3 定位到 `add.asc` 第 84 行，确认是 CopyOut 阶段写入地址越界。

### 4. 修复代码

```cpp
// 修复前（第 84 行）：
AscendC::DataCopy(gmC[i * tileSize + 100000000], bufC, tileSize);

// 修复后：
AscendC::DataCopy(gmC[i * tileSize], bufC, tileSize);
```

移除多余的 `+100000000` 偏移量。重新编译运行，程序正常退出（exit 0），结果验证通过。

---

## 调试命令参考

以下为 coredump 模式下可用命令的详细说明，均在 `(msdebug)` 交互界面中使用。

### ascend info summary — 查看崩溃概要

显示所有崩溃核的 ID、类型、PC 地址，以及 dump 文件中包含的内存区域。

```text
(msdebug) ascend info summary
  CoreId  CoreType        PC         DeviceId    ChipType
 *   6       AIV    0x12c0418032e8       0        A2/A3
     7       AIV    0x12c0418032e8       0        A2/A3
     ...（8 个核全部崩溃在同一 PC）

  Id    DataType                MemType        Addr                  Size
   0    DEVICE_KERNEL_OBJECT       GM        0x12c041800000         295504
   1    STACK                   GM/DCACHE    0xff0000f8000           32768
   ...
```

- **CoreId**：崩溃核的编号，`*` 标记当前聚焦的核。
- **PC**：崩溃时的程序计数器值，所有核相同说明是代码逻辑错误。
- **内存表**：列出 dump 中所有可读的内存区域及地址，供 `x` 命令使用。

### bt — 查看调用栈

回溯从崩溃点（帧 #0）到用户调用入口的函数调用链。

```text
(msdebug) bt
* thread #1, stop reason = MTE_ERROR
  * frame #0: ... DataCopyUB2GMImpl<float>(...) at kernel_operator_data_copy_impl.h:104:9
    frame #1: ... DataCopy<float>(...) at kernel_operator_data_copy_intf_impl.h:297:9
    frame #2: ... DataCopy<float>(...) at kernel_operator_data_copy_intf_impl.h:791:5
    frame #3: ... DoubleBufAdd::Process(...) at add.asc:84:13
    frame #4: ... add_demo(...) at add.asc:104:8
```

- 帧 #0~#2 为 AscendC 库函数链（`DataCopy` → `DataCopyUB2GMImpl`）。
- 帧 #3 为用户源码 `DoubleBufAdd::Process`，即异常 `DataCopy` 的调用位置。
- 帧 #4 为核函数入口 `add_demo`。

> **注意**：`bt` 仅在 stop_reason 为 MTE_ERROR / VEC_ERROR / CUBE_ERROR / CCU_ERROR / FIXP_ERROR 时保证准确性；编译时需带 `-g` 才能显示行号，建议用 `-O2` 以保留 inline 展开获取更多栈帧。

### re r — 查看寄存器

查看崩溃时的程序计数器（PC）值和其他寄存器状态。

```text
(msdebug) re r $PC
                            PC = 0x000012c0418032e8  ... at kernel_operator_data_copy_impl.h:104:9
```

PC 指向崩溃指令地址，与 `bt` 帧 #0 一致。

### x — 读取内存

读取 dump 中指定内存区域的数据。地址需在 `ascend info summary` 列出的范围内。

```text
(msdebug) x -m GM -f float32[] 0x12c041800000 -c 2 -s 32
0x12c041800000: {9.62965086E-35 9.40409829E-38 ...}
0x12c041800020: {1.16839145E-37 1.20746768E-34 ...}
```

| 参数 | 说明 | 可选值 |
|------|------|--------|
| `-m` | 内存空间 | `GM`、`UB`、`L1`、`L0A`、`L0B`、`L0C`、`FB`、`STACK` |
| `-f` | 数据格式 | `float32[]`、`float16[]`、`int32[]`、`uint8[]` 等 |
| `-c` | 打印行数 | 正整数 |
| `-s` | 每行字节数 | 正整数 |

> **注意**：coredump 模式下仅可读取 dump 范围内地址，否则报错 "addr is not in range"。

### ascend aiv — 切换 Vector 核

切换到其他崩溃的 Vector 核，查看不同核的崩溃状态。

```text
(msdebug) ascend aiv 7
[Switching to focus on Kernel _Z8add_demoPhS_S_S_, CoreId 7, Type aiv]
* thread #1, stop reason = MTE_ERROR
    frame #0: 0x... DataCopyUB2GMImpl<float>(...) at kernel_operator_data_copy_impl.h:104:9
```

### 命令速查表

| 命令 | 功能 | 可用性 |
|------|------|--------|
| `bt` | 查看调用栈，定位源码异常行 | ✅ |
| `ascend info summary` | 查看崩溃概要和内存布局 | ✅ |
| `re r $PC` | 查看程序计数器 | ✅ |
| `x -m <space> <addr>` | 读取内存（限 dump 范围） | ✅ |
| `ascend aiv <id>` | 切换 Vector 核 | ✅ |
| `ascend info cores` | 查看核信息 | ❌ coredump 不支持 |
| `ascend info tasks` | 查看 Task 信息 | ❌ coredump 不支持 |
| `n` / `s` / `c` | 单步/继续运行 | ❌ 静态快照不支持 |

---

## 流程总结

1. 编译算子（`-O2 -g`）
2. 设置 `ASCEND_DUMP_SCENE` 和 `ASCEND_DUMP_PATH` 环境变量
3. 运行算子触发 coredump
4. `msdebug --core <coredump> ./add` 加载分析
5. 执行 `bt` 定位源码异常行
6. 修复代码，重新编译验证

## 注意事项

- 必须设置 `ASCEND_DUMP_SCENE="aic_err_detail_dump"` 才会生成 coredump 文件。
- coredump 生成与 msdebug 调试互斥，不能同时进行。
- 编译建议用 `-O2 -g`：`-O2` 保留 inline 展开获取更多栈帧，`-g` 保留行号。
- coredump 模式下 `ascend info cores` / `n` / `s` / `c` 等实时命令不可用。
- `bt` 仅在 stop_reason 为 MTE_ERROR / VEC_ERROR 等硬件异常时保证准确性。
