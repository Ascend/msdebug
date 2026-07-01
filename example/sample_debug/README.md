# msDebug 算子上板调试样例介绍

## 概述

本样例演示如何使用 **msdebug** 工具对算子进行上板调试。

## 适用场景

- 需要使用debug命令调试算子功能时。 TODO:丰富下描述

## 支持的产品范围

- Ascend 950PR/Ascend 950DT
- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品

## 目录结构

```text
├── sample_debug/
│   ├── CMakeLists.txt          // 编译工程文件
│   ├── debug.asc               // Ascend C 样例实现
│   └── README.md               // 本说明文件
```

## 样例描述

### 核心逻辑

本样例在 `debug.asc` 中实现了一个向量加法（Add）算子，其执行流程为：

1. 将输入向量 x 和 y 从 Global Memory 搬入 Local Memory（`AscendC::DataCopy`）。
2. 在 Local Memory 上执行向量加法（`AscendC::Add`）。
3. 将结果向量 z 从 Local Memory 搬回 Global Memory。

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_debug/` 目录下执行：

```bash
mkdir -p build && cd build
cmake -DCMAKE_ASC_ARCHITECTURES=dav-2201 ..
make -j96
```

编译完成后，`build` 目录下将生成算子二进制文件 `demo`。

> **说明**：编译选项 `--npu-arch` 已通过 CMakeLists.txt 自动配置。调试算子需在编译参数中增加 `-g -O0`（关闭优化以保留完整调试信息），CMakeLists.txt 中已默认配置。

- 编译选项说明

| 选项 | 可选值 | 说明 |
|------|--------|------|
| `CMAKE_ASC_RUN_MODE` | `npu`（默认）、`cpu`、`sim` | 运行模式：NPU 运行、CPU调试、NPU仿真 |
| `CMAKE_ASC_ARCHITECTURES` | `dav-2201`（默认）、`dav-3510` | NPU 架构：dav-2201 对应 Atlas A2/A3 系列产品，dav-3510 对应 Ascend 950PR/Ascend 950DT |

> **注意：** 切换编译模式前需清理 cmake 缓存，可在 build 目录下执行 `rm CMakeCache.txt` 后重新 cmake。

### 算子运行

1. 拉起算子

    使用 msdebug 工具拉起算子：

    ```bash
    msdebug ./demo
    ```

    > **说明**：msdebug命令含义请参考：[msdebug 用户指南](https://gitcode.com/Ascend/msdebug/blob/master/docs/zh/user_guide/msdebug_user_guide.md)。

    工具成功拉起算子后，将在终端输出如下日志：

    ```text
    msdebug 26.0.0
    (msdebug) target create "demo"
    Current executable set to '/path/to/sample_debug/build/demo' (aarch64).
    ```

    随后进入调试界面。

2. 调试界面功能演示

    msdebug 支持类 lldb 的调试命令体系，以下是上板调试常用命令及实测输出。

### 2.1 断点设置与运行控制

    **设置断点**（`b` / `breakpoint set`）：

    ```text
    (msdebug) b debug.asc:45
    Breakpoint 1: where = device_debugdata0`void vec_add_demo<2048u>(float*, float*, float*) + <offset> at debug.asc:45:5, address = 0x<addr>
    ```

    **运行程序**（`r` / `run`）：

    ```text
    (msdebug) r
    Process <pid> launched: '/path/to/sample_debug/build/demo' (aarch64)
    [Launch of Kernel _Z12vec_add_demoILj2048EEvPfS0_S0_ on Device 0]
    Process <pid> stopped
    [Switching to focus on Kernel ..., CoreId 32, Type aiv]
    * thread #1, name = 'demo', stop reason = breakpoint 1.2
        frame #0: ... at debug.asc:45:5
    -> 45       AscendC::DataCopy(bufA, gmA, tileSize);
    ```

    **继续运行**（`c` / `continue`）：程序从断点处继续执行直到下一个断点或程序结束。

### 2.2 单步调试

    **单步跳过**（`n` / `next` / `thread step-over`）：执行当前行并停在下一行。

    ```text
    (msdebug) n
    Process <pid> stopped
    * thread #1, stop reason = step over
    -> 47       AscendC::PipeBarrier<PIPE_ALL>();
    ```

    **单步进入**（`s` / `step` / `thread step-in`）：进入函数内部调试。

    ```text
    (msdebug) s
    Process <pid> stopped
    * thread #1, stop reason = step in
        frame #0: ... AscendC::DataCopy<float>(...) at kernel_operator_data_copy_intf_impl.h:772:27
    -> 772      struct DataCopyParams repeatParams;
    ```

    **单步跳出**（`finish` / `thread step-out`）：执行完当前函数并返回调用处。

    ```text
    (msdebug) finish
    Process <pid> stopped
    * thread #1, stop reason = step out
-> 45       AscendC::DataCopy(bufA, gmA, tileSize);
    ```

### 2.3 变量查看

    **查看所有局部变量**（`var` / `frame variable`）：

    ```text
    (msdebug) var
    (float *__gm__) a = 0x000012c0c0015000
    (float *__gm__) b = 0x000012c0c0026000
    (float *__gm__) c = 0x000012c0c0037000
    (AscendC::GlobalTensor<float>) gmA = {
      address_ = 0x000012c0c0015000
      bufferSize_ = 2048
      ...
    }
    (AscendC::LocalTensor<float>) bufA = {
      dataLen = 8192, bufferAddr = 0
      ...
    }
    (AscendC::LocalTensor<float>) bufB = {
      dataLen = 8192, bufferAddr = 8192
      ...
    }
    (AscendC::LocalTensor<float>) bufC = {
      dataLen = 8192, bufferAddr = 16384
      ...
    }
    ```

    **打印指定变量**（`p` / `print`）：

    ```text
    (msdebug) p bufA
    (AscendC::LocalTensor<float>) {
      AscendC::BaseLocalTensor<float> = {
        address_ = (dataLen = 8192, bufferAddr = 0, ...)
      }
      ...
    }
    ```

### 2.4 调用栈查看

    **查看调用栈**（`bt` / `thread backtrace`）：

    ```text
    (msdebug) bt
    * thread #1, name = 'demo', stop reason = breakpoint 1.2
      * frame #0: 0x<addr> void vec_add_demo<2048u>(a=0x<addr>, b=0x<addr>, c=0x<addr>) at debug.asc:45:5
        frame #1: 0x<addr> void vec_add_demo<2048u>(a=0, b=0, c=0) at debug.asc:43:48
    ```

    **切换栈帧**（`frame select <id>`）：

    ```text
    (msdebug) frame select 1
    frame #1: ... void vec_add_demo<2048u>(...) at debug.asc:43:48
    -> 43       AscendC::LocalTensor<float> bufC = ubAlloc.Alloc<float, tileSize>();
    ```

### 2.5 内存读取

    **读取 Global Memory**（`x -m GM`）：

    ```text
    (msdebug) x -m GM -f float32[] 0x12c0c0015000 -c 2 -s 32
    0x12c0c0015000: {0 0.100000001 0.200000003 0.300000012 0.400000006 0.5 0.600000024 0.699999988}
    0x12c0c0015020: {0.800000011 0.900000035 1 1.10000002 1.20000005 1.30000007 1.39999998 1.5}
    ```

    **读取 Unified Buffer**（`x -m UB`）：

    ```text
    (msdebug) x -m UB -f float32[] 0x0 -c 2 -s 32
    0x00000000: {0 0.100000001 0.200000003 0.300000012 0.400000006 0.5 0.600000024 0.699999988}
    0x00000020: {0.800000011 0.900000035 1 1.10000002 1.20000005 1.30000007 1.39999998 1.5}
    ```

    > **说明**：`-m` 支持的内存空间有 `GM`、`UB`、`L1`、`L0A`、`L0B`、`L0C`、`FB`；`-f` 支持 `float32[]`、`float16[]`、`int32[]`、`uint8[]` 等格式；`-c` 指定打印行数，`-s` 指定每行字节数。

### 2.6 寄存器查看

    **读取程序计数器**（`re r $PC`）：

    ```text
    (msdebug) re r $PC
                                PC = 0x<addr>  ... void vec_add_demo<2048u>(...) + <offset> at debug.asc:45:5
    ```

    **读取全部寄存器**（`register read -a`）：

    ```text
    (msdebug) register read -a
    Registers:
                                  PC = 0x000012c0412002d4
                                GPR0 = 0x00000000002cfd70
                                GPR1 = 0x00000000002cfea0
                                ...
                              STATUS = 0x0000000000000000
                                CTRL = 0x000100000000003c
                             SYS_CNT = 0x00002da919163baa
                                ...
    ```

### 2.7 设备与核信息查询

    **查询 Device 信息**（`ascend info devices`）：

    ```text
    (msdebug) ascend info devices
     Device Aic_Num Aiv_Num Aic_Mask     Aiv_Mask
    *     0       0       8      0x0 0xff00000000
    ```

    **查询算子运行核信息**（`ascend info cores`）：

    ```text
    (msdebug) ascend info cores
      CoreId  Type  Device Stream Task Block         PC               stop reason
    *  27     aiv      0     42     0     0     0x12c0412002f0         step over
       32     aiv      0     42     0     1     0x12c0412002d4         breakpoint 1.2
       33     aiv      0     42     0     2     0x12c0412002d4         breakpoint 1.2
       ...
    ```

    **查询 Task 信息**（`ascend info tasks`）：

    ```text
    (msdebug) ascend info tasks
      Device Stream Task Invocation
    *   0      42     0  _Z12vec_add_demoILj2048EEvPfS0_S0_
    ```

    **查询 Stream 信息**（`ascend info stream`）：

    ```text
    (msdebug) ascend info stream
      Device Stream Type
    *   0      42    aiv
    ```

    **查询 Block 信息**（`ascend info blocks` / `ascend info blocks -d`）：

    ```text
    (msdebug) ascend info blocks
      Device Stream Task Block
    *   0      42     0     0
        0      42     0     1
        ...
        0      42     0     7
    ```

    `-d` 选项可展示每个 block 当前中断位置的源代码：

    ```text
    (msdebug) ascend info blocks -d
    Current stop state of all blocks:

    [* CoreId 27, Block 0]
    * thread #1, stop reason = step over
    -> 47       AscendC::PipeBarrier<PIPE_ALL>();

    [CoreId 32, Block 1]
    * thread #1, stop reason = breakpoint 1.2
    -> 45       AscendC::DataCopy(bufA, gmA, tileSize);
    ...
    ```

### 2.8 切换调试聚焦核

    **切换 Vector 核**（`ascend aiv <id>`）：

    ```text
    (msdebug) ascend aiv 32
    [Switching to focus on Kernel ..., CoreId 32, Type aiv]
    * thread #1, stop reason = breakpoint 1.2
    -> 45       AscendC::DataCopy(bufA, gmA, tileSize);
    ```

    > **说明**：`ascend aic <id>` 用于 Cube 核切换，仅在 Cube 算子中可用；`ascend info threads` 和 `ascend thread` 同样仅适用于 SIMT（Cube）核场景。

### 2.9 命令速查表

    | 命令 | 缩写 | 功能 |
    |------|------|------|
    | `breakpoint set -f <file> -l <line>` | `b` | 设置断点 |
    | `run` | `r` | 运行程序 |
    | `continue` | `c` | 继续运行 |
    | `thread step-over` | `n` / `next` | 单步跳过 |
    | `thread step-in` | `s` / `step` | 单步进入 |
    | `thread step-out` | `finish` | 单步跳出 |
    | `frame variable` | `var` | 查看所有局部变量 |
    | `print <var>` | `p` | 打印指定变量 |
    | `thread backtrace` | `bt` | 查看调用栈 |
    | `frame select <id>` | — | 切换栈帧 |
    | `memory read -m <space> -f <fmt> <addr>` | `x` | 读取内存 |
    | `register read` | `re r` | 读取寄存器 |
    | `ascend info devices` | — | 查询 Device 信息 |
    | `ascend info cores` | — | 查询核运行信息 |
    | `ascend info tasks` | — | 查询 Task 信息 |
    | `ascend info stream` | — | 查询 Stream 信息 |
    | `ascend info blocks [-d]` | — | 查询 Block 信息 |
    | `ascend aiv <id>` | — | 切换 Vector 核 |
    | `ascend aic <id>` | — | 切换 Cube 核（仅 Cube 算子） |

## 调试流程总结

1. 使用 `-g -O0` 编译生成 demo 二进制文件
2. 拉起调试器 `msdebug ./demo`
3. 设置断点、单步执行、查看变量，定位并修复问题

## 注意事项

- 调试算子前请确认编译选项中包含 `-g -O0`（已在 CMakeLists.txt 中默认配置），否则无法设置断点或查看变量。
- 调试时建议关闭编译优化（`-O0`），避免变量被优化导致无法查看。
