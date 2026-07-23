# msDebug算子调试工具快速入门

<br>

## 1. 概述

msDebug 是一款面向昇腾设备的算子调试工具，用于调试在NPU侧执行的算子程序，为算子开发人员提供关键调试能力，包括读取昇腾设备内存与寄存器、断点暂停与恢复程序运行状态等。
本文档基于入门教程中开发的简易加法算子，演示 msDebug 工具的核心功能，帮助初学者直观体验其在算子开发过程中带来的高效性与便捷性。

本章节以您已完成<a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/op_tool_quick_start.md" target="_blank">《算子开发工具链快速入门》</a>的全流程操作为前提；若尚未体验，建议先完成该指南以获得更佳的学习效果。

## 2. 操作步骤

### 2.1【环境】必备环境准备（强制前置 ⚠️）

🛑 **本节为强制前置步骤！跳过将导致后续操作大量出现失败。**  
本教程**仅支持**标准化 CANN 容器环境，不兼容裸机、虚拟机或其他非标准容器部署。

#### 2.1.1 安装 CANN 容器环境

✅ **请严格按以下指南完成环境安装：**  
👉 **<a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/installation_guide.md" target="_blank">《昇腾 AI 算子开发工具链学习环境安装指南》</a>**

> ⏱️ **外网可达环境下预计耗时：约 3 分钟**  
> 安装完成后，您将获得一个预装所有算子工具、示例代码和依赖库的标准化容器环境。

#### 2.1.2 执行环境自检脚本（必须通过！）

在如下正式体验前，请**全文复制下方整段脚本**，粘贴到终端执行，只有输出全部显示为 [PASS] 才能继续：

```bash
# 1. 容器环境检查
[ -f /.dockerenv ] && [ -n "$ASCEND_HOME_PATH" ] && [ -n "$ATB_HOME_PATH" ] && echo -e "\033[32m[PASS] CANN 容器环境 OK \033[0m" || echo -e "\033[31m[FAIL] 非标容器或未进入容器！\033[0m"
# 2. 示例代码仓检查
[ -d ~/ot_demo/msot/example/quick_start ] && echo -e "\033[32m[PASS] 示例代码仓 OK\033[0m" || echo -e "\033[31m[FAIL] 代码仓缺失\033[0m"
```

### 2.2【前提】算子工程准备完成

按照<a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/op_tool_quick_start.md#23开发构建算子工程msopgen" target="_blank">《算子开发工具链快速入门》</a>中 2.3 节操作，完成算子工程准备。

### 2.3 【调试】断点调试算子代码（msDebug）

若算子功能异常，可借助 msDebug 工具进行断点调试，高效定位问题。先按步骤操作体验效果，原理部分可稍后阅读。

#### 2.3.1 开启内核调试开关

> [!CAUTION]
> 
> **注意：msDebug 需要 root 权限**
> 
> msDebug 需要内核调试开关 /proc/debug_switch 开启才能正常工作，但出于安全考虑默认关闭，且需要 root 权限才能打开；
> 这在很多环境（如共享开发机、容器）中可能无法满足，这时请联系系统管理员开启，或在拥有特权的容器中体验此部分。

确认内核调试开关 debug_switch 是否已打开：

```shell
cat /proc/debug_switch
```

若输出值不为 1，请使用 root 权限执行以下命令：

```shell
echo 1 > /proc/debug_switch
```

如果不能成功设置为 1，则 msDebug 功能不可用，只能跳过本节 msDebug 的体验。

#### 2.3.2 修改编译选项并重新部署

**1. 修改编译选项**
在 Kernel 侧 CMakeLists.txt 首行插入一行配置，用于启用调试信息、禁用编译优化：

```shell
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt op_kernel/CMakeLists.txt.bak
printf '%s\n' "if(COMMAND add_ops_compile_options)" "  add_ops_compile_options(ALL OPTIONS -g -O0)" "elseif(COMMAND npu_op_kernel_options)" "  npu_op_kernel_options(ascendc_kernels ALL OPTIONS -g -O0)" "endif()" | cat - op_kernel/CMakeLists.txt > tmp && mv -f tmp op_kernel/CMakeLists.txt;
```

**2. 重新编译部署算子**

```shell
bash ./build.sh
MY_OP_PKG=$(find ./build_out -maxdepth 1 -name "custom_opp_*.run" | head -1) && bash $MY_OP_PKG
```

#### 2.3.3 断点调试与变量查看

##### 2.3.3.1 启动调试器

```shell
cd ~/ot_demo/workspace/src/caller/build
msdebug execute_add_op
```

##### 2.3.3.2 设置断点

待 (msdebug) 提示符出现后，设置断点于 add_custom.cpp 第 34 行：

```text
b add_custom.cpp:34
```

> [!CAUTION]
> 
> **若在直接申请的容器环境中操作，需特别留意 `/proc/debug_switch = 1` 可能为虚假状态**
> 
> 若您在云服务商提供的容器环境中操作，即使在容器内成功将 /proc/debug_switch 设置并查询为 1，该状态也可能是虚假的。因出于安全考虑，底层宿主机通常会通过写时复制（CoW）、影子文件或覆盖挂载（overlay mount）等机制对 /proc 目录进行隔离，导致设置未实际生效。
> 
> 在此情况下，执行上一节所述的断点设置将触发警告；而按照后续章节运行 `run` 命令时，则会报出如下错误：
>
> ```text
> error: 'A' packet returned an error: 8
> ```
>
> 若无法在宿主机上以 root 权限正确设置 `/proc/debug_switch`，或不具备切换至其他合适环境的条件，则只能跳过本节关于 `msDebug` 的实操体验。

##### 2.3.3.3 运行算子

输入 run 启动程序，等待命中断点：

```text
run
```

显示如下信息，则成功命中断点：

```text
(msdebug) run
Process 163027 launched: '/root/ot_demo/workspace/src/caller/build/execute_add_op' (aarch64)
[Launch of Kernel AddCustom_ab1b6750d7f510985325b603cb06dc8b_0 on Device 0]
Process 163027 stopped
[Switching to focus on Kernel AddCustom_ab1b6750d7f510985325b603cb06dc8b_0, CoreId 1, Type aiv]
* thread #1, name = 'execute_add_op', stop reason = breakpoint 1.1
    frame #0: 0x00000000000007e0 AddCustom_ab1b6750d7f510985325b603cb06dc8b.o`KernelAdd::Init(this=0x00000000001d78a8, x=0x12c0c0013000, y=0x12c0c001c000, z=0x12c0c0025000, totalLength=16384, tileNum=8) (.vector) at add_custom.cpp:34:9
   31           this->tileLength = this->blockLength / tileNum / BUFFER_NUM;
   32
   33           // 设置全局内存缓冲区，为当前AI Core分配其负责的全局内存区域
-> 34           xGm.SetGlobalBuffer((__gm__ DTYPE_X *)x + this->blockLength * AscendC::GetBlockIdx(), this->blockLength);
   35           yGm.SetGlobalBuffer((__gm__ DTYPE_Y *)y + this->blockLength * AscendC::GetBlockIdx(), this->blockLength);
   36           zGm.SetGlobalBuffer((__gm__ DTYPE_Z *)z + this->blockLength * AscendC::GetBlockIdx(), this->blockLength);
```

##### 2.3.3.4 查看变量的值

在断点处执行以下命令，显示当前作用域内的所有局部变量：

```text
var
```

输出示例如下：

```text
(msdebug) var
(KernelAdd *__stack__) this = 0x00000000001d78a8
(uint8_t *__gm__) x = 0x000012c0c0013000 ""
(uint8_t *__gm__) y = 0x000012c0c001c000 ""
(uint8_t *__gm__) z = 0x000012c0c0025000 ""
(uint32_t) totalLength = 16384
(uint32_t) tileNum = 8
```

##### 2.3.3.5 查看寄存器的值

```text
register read -a
```

输出示例如下：

```text
(msdebug) register read -a
                  PC = 0x12C04120088C
                COND = 0x0
                CTRL = 0x100000000003C
                GPR0 = 0x1D78A8
                GPR1 = 0x1D7E28
                GPR2 = 0x800
                GPR3 = 0x0
                GPR4 = 0x0
                GPR5 = 0x8
```

##### 2.3.3.6 查询Device信息

```text
ascend info devices
```

输出示例如下：

```text
(msdebug) ascend info devices
  Device Aic_Num Aiv_Num Aic_Mask Aiv_Mask
*    3      0       8      0x0     0xf0000000000f
```

##### 2.3.3.7 查询算子所运行的aicore相关信息

```text
ascend info cores
```

输出示例如下：

```text
(msdebug) ascend info cores
  CoreId Type Device Stream Task Block               PC    stop reason Filename Line
*      0  aiv      3    47    0     4  0x12c041200920  breakpoint 1.1       NA   NA
       1  aiv      3    47    0     5  0x12c041200920  breakpoint 1.1       NA   NA
       2  aiv      3    47    0     6  0x12c041200920  breakpoint 1.1       NA   NA
       3  aiv      3    47    0     7  0x12c041200920  breakpoint 1.1       NA   NA
      44  aiv      3    47    0     0  0x12c041200920  breakpoint 1.1       NA   NA
      45  aiv      3    47    0     1  0x12c041200920  breakpoint 1.1       NA   NA
      46  aiv      3    47    0     2  0x12c041200920  breakpoint 1.1       NA   NA
      47  aiv      3    47    0     3  0x12c041200920  breakpoint 1.1       NA   NA
```

##### 2.3.3.8 查询算子所运行的task相关信息

```shell
ascend info tasks
```

输出示例如下：

```text
(msdebug) ascend info tasks
  Device Stream Task Invocation
*   3      47     0  AddCustom_ab1b6750d7f510985325b603cb06dc8b_0
```

##### 2.3.3.9 查询算子所运行的stream相关信息

```shell
ascend info stream
```

输出示例如下：

```text
(msdebug) ascend info stream
  Device Stream Type
*   3      47    aiv
```

##### 2.3.3.10 查询算子所运行的block相关信息

```shell
ascend info blocks
```

输出示例如下：

```text
(msdebug) ascend info blocks
  Device Stream Task Block
*   3      47     0     4
    3      47     0     5
    3      47     0     6
    3      47     0     7
    3      47     0     0
    3      47     0     1
    3      47     0     2
    3      47     0     3
```

##### 2.3.3.11 退出调试器

```text
q
y
```

#### 2.3.4 恢复被修改的文件

执行如下命令：

```shell
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt.bak op_kernel/CMakeLists.txt
```
