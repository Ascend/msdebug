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
