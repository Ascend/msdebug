# **MindStudio Debugger工具快速入门**

## 概述

**简介**

MindStudio Debugger（算子调试工具，msDebug）支持调试所有昇腾算子，用户可以根据实际情况选择使用不同的功能，例如，可以设置断点、打印变量和内存、进行单步调试、中断运行、核切换等。

本文档以一个简单样例介绍msDebug算子调试工具的使用方法。

**环境准备**

请参考[MindStudio Debugger安装指南](./msdebug_install_guide.md)安装msDebug工具。

## 操作步骤

1. 在$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch目录执行以下命令，生成自定义算子工程，进行host侧和kernel侧的算子实现。\$\{git\_clone\_path\}为sample仓的存放路径。

    ```bash
    bash install.sh -v Ascendxxxyy    # xxxyy为用户实际使用的具体芯片类型
    ```

2. 在$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/CustomOp目录下修改CMakePresets.json文件的cacheVariables的配置项，将**Release**修改为**Debug**。

    ```bash
    "cacheVariables": {
        "CMAKE_BUILD_TYPE": {
          "type": "STRING",
          "value": "Debug"
        },
        ...
      }
    ```

3. 执行以下命令，重新编译部署算子。
    1. 在$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/CustomOp目录下，执行以下命令，重新编译部署算子。

        ```bash
        bash build.sh
        ./build_out/custom_opp_<target_os>_<target_architecture>.run  # 当前目录下run包的名称
        ```

    2. 切换到$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/AclNNInvocation目录，并执行以下命令，将会在`./output`路径下生成可执行文件`execute_add_op`。

        ```bash
        bash run.sh
        cd  ./output
        ```

4. 在调试前，配置如下环境变量，指定算子加载路径，导入调试信息，示例如下。

    ```bash
    export LAUNCH_KERNEL_PATH=${INSTALL_DIR}/opp/vendors/customize/op_impl/ai_core/tbe/kernel/${soc_version}/add_custom/AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o   # soc_version为昇腾AI处理器的Chip Name，需为小写
    ```

5. 指定算子依赖的动态库路径，将动态库so文件加载进来。$\{INSTALL\_DIR\}为CANN软件安装后文件存储路径。

    ```bash
    export LD_LIBRARY_PATH=${INSTALL_DIR}/opp/vendors/customize/op_api/lib:$LD_LIBRARY_PATH
    ```

6. 在可执行文件目录下执行**msdebug execute\_add\_op**，进入msDebug工具。

    ```bash
    msdebug execute_add_op
    ```

7. 执行以下命令，设置断点。

    ```bash
    (msdebug) b add_custom.cpp:55
    ```

    回显将会显示断点信息添加成功。

    ```tex
    Breakpoint 1: where = AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o`KernelAdd::Compute(int) (.vector) + 68 at add_custom.cpp:55:9, address = 0x00000000000014f4
    ```

8. 键盘输入**r**命令，运行算子程序，等待直到命中断点。

    ```bash
    (msdebug) r
    Process 1454802 launched: '${INSTALL_DIR}/add_cus/AclNNInvocation/output/execute_add_op' (aarch64)
    [INFO]  Set device[0] success
    [INFO]  Get RunMode[1] success
    [INFO]  Init resource success
    [INFO]  Set input success
    [INFO]  Copy input[0] success
    [INFO]  Copy input[1] success
    [INFO]  Create stream success
    [INFO]  Execute aclnnAddCustomGetWorkspaceSize success, workspace size 0
    [Launch of Kernel AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b on Device 0]
    [INFO]  Execute aclnnAddCustom success
    Process 1454802 stopped
    [Switching to focus on Kernel AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b, CoreId 39, Type aiv]
    * thread #1, name = 'execute_add_op', stop reason = breakpoint 1.1
        frame #0: 0x00000000000014f4 AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o`KernelAdd::Compute(this=0x00000000003078a8, progress=0) (.vector) at add_custom.cpp:55:9
       52       __aicore__ inline void Compute(int32_t progress)
       53       {
       54           LocalTensor<DTYPE_X> xLocal = inQueueX.DeQue<DTYPE_X>();
    -> 55           LocalTensor<DTYPE_Y> yLocal = inQueueY.DeQue<DTYPE_Y>();   # 断点处的行号正确即可，其余信息以实际为准
       56           LocalTensor<DTYPE_Z> zLocal = outQueueZ.AllocTensor<DTYPE_Z>();
       57           Add(zLocal, xLocal, yLocal, this->tileLength);
       58           outQueueZ.EnQue<DTYPE_Z>(zLocal);
    ```

9. 键盘输入以下命令，继续运行。

    ```bash
    (msdebug) c
    ```

    显示程序再次命中该断点。

    ```tex
    Process 1454802 resuming
    Process 1454802 stopped
    [Switching to focus on Kernel AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b, CoreId 39, Type aiv]
    * thread #1, name = 'execute_add_op', stop reason = breakpoint 1.1
        frame #0: 0x00000000000014f4 AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o`KernelAdd::Compute(this=0x00000000003078a8, progress=0) (.vector) at add_custom.cpp:55:9
       52       __aicore__ inline void Compute(int32_t progress)
       53       {
       54           LocalTensor<DTYPE_X> xLocal = inQueueX.DeQue<DTYPE_X>();
    -> 55           LocalTensor<DTYPE_Y> yLocal = inQueueY.DeQue<DTYPE_Y>();  # 断点处的行号正确即可，其余信息以实际为准
       56           LocalTensor<DTYPE_Z> zLocal = outQueueZ.AllocTensor<DTYPE_Z>();
       57           Add(zLocal, xLocal, yLocal, this->tileLength);
       58           outQueueZ.EnQue<DTYPE_Z>(zLocal);
    ```

10. 结束调试。

    ```bash
    (msdebug) q
    ```
