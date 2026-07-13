# **FAQs**

## msDebug Cannot Print Tensor Variables, Prompting "Unavailable" or "Memory Read Failed"

**Symptom**

The message `unavailable` or `Failed to dereference pointer from xxx for DW_OP_deref: memory read failed for xxx` is displayed.

**Possible Causes**

The step-by-step debugging function does not support tensor passing by value.

**Solution**

This problem occurs when the printed object a is of the Tensor type and the value is passed as the input parameter of the function.

```bash
void Foo(const LocalTensor<float> a); // The variable a fails to be printed.
```

To print the variable, modify the code so that object *a* is passed by reference as the input parameter of the function.

```bash
void Foo(const LocalTensor<float> &a); // The variable a can be printed normally.
```

## msDebug Fails to Be Debugged in the Container Environment and HDK Driver Package Needs to Be Installed

**Symptom**

The message `msdebug failed to initialize. please install HDK with –debug before debugging` is displayed.

**Possible Causes**

The HDK driver package is not installed using the `--debug` option, or the driver device node `/dev/drv_debug` on which the msDebug tool depends is not mapped to the container environment.

**Solution**

1. Check whether the host machine uses the `--debug` option to install the HDK driver package.

    If the command output is the same, the debugging driver has been installed. Otherwise, run the `--debug` command to install the matching HDK driver package.

    ```bash
    [mindstudio@localhost ~]$ ls /dev/drv_debug     # Check whether the /dev/drv_debug device node exists.
    /dev/drv_debug
    ```

2. If the driver package has been installed and the operator operating environment is a container, check whether the container environment meets the following conditions:

    - The device node `/dev/drv_debug` on which debugging depends can be found.
    - The container environment has permission to access the device node.

    > [!NOTE]NOTE  
    > You are advised to add the `--privileged --device=/dev/drv_debug` option to the container startup command to ensure that the device node on which debugging depends is mapped and the container environment can access the node.

## Operator Execution Fails When the continue Command Is Run After the msDebug Breakpoint in the Kernel Function Is Hit

**Symptom**

The message `Synchronize stream failed. error code is 507035` is displayed. The "aic error code=0x8000000000000000" is displayed in the plog. The PC value of the current core displayed by running the `ascend info cores` command is different from the expected value.

**Possible Causes**

The size of the workspace input parameter in the kernel function is set to `0` in the tiling function. After the single-operator API is called, the workspace input parameter becomes an invalid address. Although the workspace input parameter is not used in the kernel function, the debugger dereferences the workspace pointer when displaying the kernel input parameter. As a result, the operator fails to run.

**Solution**

Set the workspace size from 0 to the reserved memory size by referring to section "Operator Implementation" \> "Project-based Operator Development" \> "[Host-side Tiling Implementation](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_00021.html)" in *Ascend C Operator Development Guide*. During API compute, some workspace memory is required as the cache. Therefore, the operator tiling function needs to reserve workspace memory for the API. The reserved memory size can be obtained by calling `GetLibApiWorkSpaceSize`. See the following code:

```cpp
#include "tiling/platform/platform_ascendc.h"
auto ascendcPlatform = platform_ascendc::PlatformAscendC(context->GetPlatformInfo());
size_t systemWorkspaceSize = ascendcPlatform.GetLibApiWorkSpaceSize();
size_t*currentWorkspace = context->GetWorkspaceSizes(1); // Only one workspace is used.
currentWorkspace[0]= systemWorkspaceSize;
```

## `'A' Packet Returned an Error: 8` Is Displayed After msDebug Runs the run Command in Docker

**Symptom**

In Docker, after msDebug runs the `run` command, the following error message is displayed:

```bash
(msdebug) run
'A' packet returned an error: 8
(msdebug)
...
```

**Possible Causes**

This error may be related to address space layout randomization.

**Solution**

Run the following commands to avoid this problem:

```bash
...
(msdebug) settings set target.disable-aslr false
...
```

## Error Message `error: undefined symbol: g_opSystemRunCfg` Is Displayed During Compilation

**Symptom**

The following error message is displayed during `O0` compilation:

```bash
ld.lld: error: undefined symbol: g_opSystemRunCfg
```

**Solution**  
The `- DL2_CACHE_HINT` compilation option needs to be removed.

## Hit Breakpoint Code Position Does Not Match Expectation

**Symptom**

The actual hit breakpoint line does not match the set line number.

**Possible Causes**

- Cause 1: Files with the same name exist on both the host and kernel sides. The breakpoint on the host-side file is hit first when the program starts. If there is no breakpoint at that line on the host side, the debugger searches downward for the nearest available line.
- Cause 2: The target line in the kernel file does not contain breakpoint information. The debugger searches downward for the nearest available line. This is an expected behavior of the tool, usually caused by compiler optimization on that line, or the kernel not being compiled with `-O0`.

**Solution**

- Cause 1: If the output does not contain `[Switching to focus...]` and `stop reason = breakpoint x`, the host-side breakpoint is hit. Run `c` to continue execution and the kernel-side breakpoint will be hit. Host-side breakpoint addresses usually start with `0x7ff`, while kernel-side breakpoint addresses usually start with `device_debugdata`.
- Cause 2: Ensure that the kernel is compiled with the `-g -O0` option to prevent compiler optimization from removing breakpoint information.

The following is an example of cause 1. After setting a breakpoint at line 28, line 49 on the host side (`libascendc_ops.so`) is hit first:

```bash
(msdebug) b add_custom_kernel.asc:28
Breakpoint 1: no locations (pending on future shared library load).
WARNING:  Unable to resolve breakpoint to any actual locations.
(msdebug) r
...
1 location added to breakpoint 1
[Launch of Kernel _ZN11ascendc_ops10add_customEPfS0_S0_ on Device 0]
1 location added to breakpoint 1
Process 1436447 stopped
* thread #1, name = 'python3', stop reason = breakpoint 1.1
    frame #0: 0x00007ffe53b8a232 libascendc_ops.so`ascendc_ops::add_custom(cce_launch_config.block=8, cce_launch_config.dynamicShmemSz=0, cce_launch_config.stream=0x0000555583616d38, x=0x000012004ce30800, y=0x000012004ce20800, z=<unavailable>) at add_custom_kernel.asc:49:1
   46
   47       AscendC::DataCopy(zGm, zLocal, BLOCK_LENGTH);
   48       AscendC::PipeBarrier<PIPE_ALL>();
-> 49   }
   50   } // namespace ascendc_ops
```

Run `c` to continue execution and hit the kernel-side breakpoint:

```bash
(msdebug) c
Process 1436447 resuming
Process 1436447 stopped
[Switching to focus on Kernel _ZN11ascendc_ops10add_customEPfS0_S0_, CoreId 36, Type aiv]
* thread #1, name = 'python3', stop reason = breakpoint 1.2
    frame #0: 0x000012004100011c device_debugdata_0`ascendc_ops::add_custom(x=0x12004ce30800, y=0x12004ce20800, z=0x12004ce40800) at add_custom_kernel.asc:31:27
   28       AscendC::GlobalTensor<float> xGm;
   29       AscendC::GlobalTensor<float> yGm;
   30       AscendC::GlobalTensor<float> zGm;
-> 31       xGm.SetGlobalBuffer(x + block_idx * BLOCK_LENGTH, BLOCK_LENGTH);
   32       yGm.SetGlobalBuffer(y + block_idx * BLOCK_LENGTH, BLOCK_LENGTH);
   33       zGm.SetGlobalBuffer(z + block_idx * BLOCK_LENGTH, BLOCK_LENGTH);
   34
```
