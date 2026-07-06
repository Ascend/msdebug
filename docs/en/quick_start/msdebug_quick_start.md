# msDebug Quick Start

<br>

## 1. Overview

msDebug is an operator debugging tool designed for Ascend devices. It is used to debug operator programs executed on the NPU and provides key debugging capabilities for operator developers, including reading the memory and registers of Ascend devices, and pausing and resuming program running status.
This document demonstrates the core functions of msDebug based on the simple addition operator developed in the introductory tutorial. It helps beginners intuitively experience the efficiency and convenience the tool brings to the operator development process.

### 1.1 Recommendations

This document assumes that you have completed all operations in <a href="https://gitcode.com/Ascend/msot/blob/master/docs/en/quick_start/op_tool_quick_start.md" target="_blank">Ascend Operator Development Toolchain Quick Start</a>. If you have not done so, complete that guide first for a better learning experience.

### 1.2 Environment Setup

Strictly follow the <a href="https://gitcode.com/Ascend/msot/blob/master/docs/en/quick_start/installation_guide.md" target="_blank">Ascend AI Operator Development Toolchain Learning Environment Installation Guide</a> to complete the environment installation and workspace configuration.
Even if you have a similar environment, perform the steps in the guide again to ensure that all dependent components and environment variables are complete and consistent.

## 2. Operation

### 2.1 [Environment] Pre-checking the Runtime Environment

#### 2.1.1 Verifying Installation of Python Dependencies

Run the following command. If `All is OK` is displayed, the required Python packages and their versions meet the specifications:

```shell
python3 -c "import numpy, sympy, scipy, attrs, psutil, decorator; from packaging import version; assert version.parse(numpy.__version__) <= version.parse('1.26.4'); print('All is OK')"
```

If an error occurs, refer to [Section 1.2](#12-environment-setup) for correct installation.

### 2.2 [Prerequisite] Completing Operator Project Preparation

Follow the instructions in <a href="https://gitcode.com/Ascend/msot/blob/master/docs/en/quick_start/op_tool_quick_start.md" target="_blank">Ascend Operator Development Toolchain Quick Start</a> to complete sections 2.1 and 2.3.

### 2.3 [Debugging] Debugging Operator Code Using Breakpoints (msDebug)

If the operator function is abnormal, you can use msDebug to debug the operator code using breakpoints to efficiently identify the problem. Follow the operations first to experience the effect. You can read the principles later.

#### 2.3.1 Enabling Kernel Debugging

>[!CAUTION]CAUTION
>**msDebug requires the root permission.**
>msDebug can work properly only when the kernel debugging switch `/proc/debug_switch` is enabled. However, for security purposes, the switch is disabled by default and can be enabled only by the `root` user.
>This may not be possible in many environments (such as shared development machines and containers). In this case, contact the system administrator to enable the function or experience it in a privileged container.

Check whether the kernel debugging switch `debug_switch` is enabled.

```shell
cat /proc/debug_switch
```

If the output value is not `1`, run the following command with the `root` permission:

```shell
echo 1 > /proc/debug_switch
```

If the value cannot be set to `1`, the msDebug function is unavailable. In this case, skip this section.

#### 2.3.2 Modifying Compilation Options and Redeploying the Operator

**1. Modify the compilation options.**
Insert a line of configuration to the first line of the `CMakeLists.txt` file on the kernel to enable debugging information and disable compilation optimization.

```shell
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt op_kernel/CMakeLists.txt.orig.bak
sed -i "1i\\add_ops_compile_options(ALL OPTIONS -g -O0)" op_kernel/CMakeLists.txt
```

**2. Re-compile and deploy the operator.**

```shell
bash ./build.sh
MY_OP_PKG=$(find ./build_out -maxdepth 1 -name "custom_opp_*.run" | head -1) && bash $MY_OP_PKG
```

#### 2.3.3 Setting Debugging Environment Variables

>[!NOTE]NOTE
>**Q: When do I need to set LAUNCH_KERNEL_PATH?**
>You need to set `LAUNCH_KERNEL_PATH` for all projects except the `<<<>>>` project. That is, when the operator binary exists and is deployed independently in the form of an .o file,
>you need to explicitly tell msDebug to import the operator debugging information. Otherwise, the debugging function will be abnormal.

Set `LAUNCH_KERNEL_PATH` to specify the path for loading the operator .obj file and import the debugging symbol information.
> **Method for searching for the operator .obj file path:** Search in the operator deployment path. The example path is as follows: `/usr/local/Ascend/ascend-toolkit/latest/opp/vendors/customize/op_impl/ai_core/tbe/kernel/ascend910b/add_custom/AddCustom_ab1b6750d7f510985325b603cb06dc8b.o`

```shell
export LAUNCH_KERNEL_PATH={path_to_kernel}/my_kernel.o
```

#### 2.3.4 Debugging with Breakpoints and Viewing Variables

##### 2.3.4.1 Starting the Debugger

```shell
cd ~/ot_demo/workspace/src/caller/build
msdebug execute_add_op
```

##### 2.3.4.2 Setting Breakpoints

After the (msdebug) prompt is displayed, set a breakpoint at line 34 in `add_custom.cpp`.

```text
b add_custom.cpp:34
```

>[!CAUTION]CAUTION
>**If you perform operations in the directly applied container environment, pay attention to the fact that the `/proc/debug_switch = 1` may be in a false state.**
> If you perform operations in the container environment provided by the cloud service provider, even if the `/proc/debug_switch` is successfully set to 1 and queried in the container, the status may be false. For security purposes,
> the underlying host machine usually isolates the `/proc` directory through mechanisms such as copy-on-write (CoW), shadow files, or overlay mount. As a result, the settings do not take effect.
> In this case, setting breakpoints as described in the previous section will trigger a warning. When you run the `run` command according to the following sections, the following error message is displayed:
>
> ```text
> error: 'A' packet returned an error: 8
> ```
>
> If you cannot correctly set `/proc/debug_switch` on the host machine with the `root` permission or you do not have the conditions to switch to another proper environment, skip the hands-on experience of `msDebug` in this section.

##### 2.3.4.3 Running an Operator

Enter `run` to start the program and wait for the breakpoint to be hit.

```shell
run
```

If the following information is displayed, the breakpoint is successfully hit:

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
   33           // Set the global memory buffer and allocate the global memory area to the current AI Core.
-> 34           xGm.SetGlobalBuffer((__gm__ DTYPE_X *)x + this->blockLength * AscendC::GetBlockIdx(), this->blockLength);
   35           yGm.SetGlobalBuffer((__gm__ DTYPE_Y *)y + this->blockLength * AscendC::GetBlockIdx(), this->blockLength);
   36           zGm.SetGlobalBuffer((__gm__ DTYPE_Z *)z + this->blockLength * AscendC::GetBlockIdx(), this->blockLength);
```

##### 2.3.4.4 Viewing the Value of a Variable

Run the following command at the breakpoint to display all local variables in the current scope:

```text
var
```

Output example:

```text
(msdebug) var
(KernelAdd *__stack__) this = 0x00000000001d78a8
(uint8_t *__gm__) x = 0x000012c0c0013000 ""
(uint8_t *__gm__) y = 0x000012c0c001c000 ""
(uint8_t *__gm__) z = 0x000012c0c0025000 ""
(uint32_t) totalLength = 16384
(uint32_t) tileNum = 8
```

##### 2.3.4.5 Viewing the Register Value

```shell
register read -a
```

Output example:

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

##### 2.3.4.6 Querying Device Information

```shell
ascend info devices
```

Output example:

```text
(msdebug) ascend info devices
  Device Aic_Num Aiv_Num Aic_Mask Aiv_Mask
*    3      0       8      0x0     0xf0000000000f
```

##### 2.3.4.7 Querying AI Core Information of an Operator

```shell
ascend info cores
```

Output example:

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

##### 2.3.4.8 Querying Task Information of an Operator

```shell
ascend info tasks
```

Output example:

```text
(msdebug) ascend info tasks
  Device Stream Task Invocation
*   3      47     0  AddCustom_ab1b6750d7f510985325b603cb06dc8b_0
```

##### 2.3.4.9 Querying Stream Information of an Operator

```shell
ascend info stream
```

Output example:

```text
(msdebug) ascend info stream
  Device Stream Type
*   3      47    aiv
```

##### 2.3.4.10 Querying Block Information of an Operator

```shell
ascend info blocks
```

Output example:

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

##### 2.3.4.11 Exiting the Debugger

```text
q
y
```

#### 2.3.5 Restoring Modified Files

Run the following commands:

```shell
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt.orig.bak op_kernel/CMakeLists.txt
```
