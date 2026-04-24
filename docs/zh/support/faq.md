# **FAQ**

## msDebug工具打印Tensor变量功能不可用，提示“unavailable”或“memory read failed”

**问题现象**

提示“unavailable”或“Failed to dereference pointer from xxx for DW_OP_deref: memory read failed for xxx”。

**原因分析**

单步调试功能不支持Tensor按值传递的写法。

**解决方案**

当打印对象a为Tensor类型且以值传递作为函数入参时会出现该问题。

```cpp
void Foo(const LocalTensor<float> a); // 该写法变量a打印失败
```

若需打印该变量，可修改代码使对象a以引用传递作为函数入参，修复该问题。

```cpp
void Foo(const LocalTensor<float> &a); // 该写法变量a可正常打印
```

## msDebug工具在容器环境中调试运行失败，提示需安装HDK驱动包

**问题现象**

提示msdebug failed to initialize. please install HDK with --debug before debugging。

**原因分析**

未使用--debug选项安装HDK驱动包或msDebug工具依赖的驱动设备节点/dev/drv_debug未映射至容器环境内。

**解决方案**

1. 检查宿主机是否使用--debug选项安装HDK驱动包。

    若回显一致，则调试驱动已安装；否则需要使用--debug命令安装配套的HDK驱动包。

    ```bash
    [mindstudio@localhost ~]$ ls /dev/drv_debug     #查看是否存在/dev/drv_debug设备节点
    /dev/drv_debug
    ```

2. 若驱动包已安装，算子运行环境为容器环境，那么请检查该容器环境中是否满足以下条件。

    - 能找到调试依赖的设备节点/dev/drv_debug。
    - 容器环境具有该设备节点的访问权限。

    > [!NOTE]    
    > 建议在容器启动命令中增加选项--privileged --device=/dev/drv_debug，可保证调试依赖的设备节点被映射，且允许容器环境访问该节点。

## msDebug工具断点设置在核函数内，命中断点后执行continue命令，算子运行失败

**问题现象**

打印信息Synchronize stream failed. error code is 507035，查看plog显示aic error code=0x8000000000000000，并且在命中断点时使用ascend info cores命令可以看到当前核的PC值与预期不符。

**原因分析**

Kernel函数中workspace入参的空间大小在Tiling函数中被设置为0，经过单算子API调用后变成一个非法地址。虽然workspace入参在Kernel函数未被使用，调试器展示Kernel入参时也会对workspace指针进行解引用，导致算子运行错误。

**解决方案**

参考《Ascend C算子开发指南》中的“算子实现 \> 工程化算子开发 \>  [Host侧tiling实现](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_00021.html)”章节，将workspacesize从0设置成预留内存大小。API在计算过程需要一些workspace内存作为缓存，因此算子Tiling函数需要为API预留workspace内存，预留内存大小通过GetLibApiWorkSpaceSize接口获取。参考如下代码：

```cpp
#include "tiling/platform/platform_ascendc.h"
auto ascendcPlatform = platform_ascendc::PlatformAscendC(context->GetPlatformInfo());
size_t systemWorkspaceSize = ascendcPlatform.GetLibApiWorkSpaceSize();
size_t*currentWorkspace = context->GetWorkspaceSizes(1); // 只使用1块Workspace
currentWorkspace[0]= systemWorkspaceSize;
```

## msDebug工具在docker中执行"run"命令运行程序后，提示“'A' packet returned an error: 8”

**问题现象**

在docker中，msDebug工具在执行"run"命令运行程序后，出现以下报错。

```bash
(msdebug) run
'A' packet returned an error: 8
(msdebug)
...
```

**原因分析**

出现该错误，可能与“地址空间布局随机化”有关。

**解决方案**

需输入并执行下列命令来规避此问题。

```bash
...
(msdebug) settings set target.disable-aslr false
...
```

## 编译时出现报错error: undefined symbol: g_opSystemRunCfg

**问题现象**

在O0编译时，出现以下报错。

```bash
ld.lld: error: undefined symbol: g_opSystemRunCfg
```

**解决方案**
需要去掉编译选项`- DL2_CACHE_HINT`。
