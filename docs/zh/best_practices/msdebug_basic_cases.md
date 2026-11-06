# **典型案例**

## 上板调试Vector算子

**概述**

展示如何使用msDebug工具来上板调试一个Vector算子，该Vector算子可实现两个向量相加并输出结果的功能。

**前期准备**

- 单击[链接](https://gitcode.com/Ascend/mstt/tree/7.0.RC3/sample)获取样例工程，为进行算子调试做准备。
- 完成相关环境变量配置，请参见[MindStudio Debugger工具用户指南](../user_guide/msdebug_user_guide.md)。

**操作步骤**

1. 基于样例工程编译算子，获取可执行文件add.fatbin。
    1. 修改sample/normal_sample/vec_only/Makefile中的COMPILER_FLAG编译选项，将`-O2`修改为`-O0 -g --cce-ignore-always-inline=true`，使能编译器调试功能。

        ```bash
        # Makefile
        ...
        COMPILER            := $(ASCEND_HOME_PATH)/compiler/ccec_compiler/bin/ccec
        COMPILER_FLAG       := -xcce -O0 -g --cce-ignore-always-inline=true -std=c++17 # 使能编译器调试功能
        ```

    2. 执行以下命令完成算子编译。

        > [!NOTE]
        >
        > 非首次场景，可以使用make clean && make命令替代make命令。

        ```bash
        cd ./mstt/sample/normal_sample/vec_only/
        make clean && make
        ```

2. 设置断点。
    1. 启动msDebug工具拉起算子程序，进入调试界面。

        ```bash
        msdebug add.fatbin
        (msdebug) target create "add.fatbin"
        Current executable set to '/home/mindstudio/projects/mstt/sample/build/add.fatbin' (aarch64).
        (msdebug)
        ```

    2. 该sample中核函数的代码实现位于add_kernel.cpp中，在此文件中，为需要的代码行设置NPU断点。

        ```bash
        (msdebug) b add_kernel.cpp:69
        Breakpoint 1: where = device_debugdata`::add_custom(uint8_t *, uint8_t *, uint8_t *) + 18804 [inlined]
        KernelAdd::Compute(int) + 5144 at add_kernel.cpp:69:9, address = 0x0000000000004974
        (msdebug)
        ```

3. 运行算子程序。

    程序会开始运行直到命中第一个断点（add_kernel.cpp:69）后停下，msDebug检测到NPU核函数add_custom开始运行，运行在Device 0。

    ```cpp
    (msdebug) run
    Process 730254 launched
    [Launch of Kernel add_custom on Device 0]
    Process 730254 stopped
    [Switching to focus on Kernel add_custom, CoreId 13, Type aiv]
    * thread #1, name = 'add.fatbin', stop reason = breakpoint 2.1
        frame #0: 0x0000000000004974 device_debugdata`::add_custom(uint8_t *, uint8_t *, uint8_t *) [inlined] KernelAdd::Compute(this=0x000000000019a930, progress=0) at add_kernel.cpp:69:9
       66              // call Add instr for computation
       67              Add(zLocal, xLocal, yLocal, TILE_LENGTH);
       68              // enqueue the output tensor to VECOUT queue
    -> 69              outQueueZ.EnQue<int16_t>(zLocal);  # 断点位置
       70              // free input tensors for reuse
       71              inQueueX.FreeTensor(xLocal);
       72              inQueueY.FreeTensor(yLocal);
    (msdebug)
    ```

4. 检视信息。
    - 使用ascend info cores命令查询NPU核信息。

        ```bash
        (msdebug) ascend info cores
          CoreId Type Device Stream Task Block               PC    Exception Filename Line
        *    13  aiv      0     3    0     0  0x1240c0034974     f0000000       NA   NA
             14  aiv      0     3    0     1  0x1240c0034974     f0000000       NA   NA
             15  aiv      0     3    0     2  0x1240c0034974     f0000000       NA   NA
             20  aiv      0     3    0     3  0x1240c0034974     f0000000       NA   NA
             21  aiv      0     3    0     4  0x1240c0034974     f0000000       NA   NA
             22  aiv      0     3    0     5  0x1240c0034974     f0000000       NA   NA
             23  aiv      0     3    0     6  0x1240c0034974     f0000000       NA   NA
             24  aiv      0     3    0     7  0x1240c0034974     f0000000       NA   NA
        (msdebug)
        ```

    - 使用print命令直接打印变量信息。

        ```bash
        (msdebug) print progress
        (int32_t) $0 = 0
        ```

    - 使用print命令与memory read命令配合可打印出Tensor变量中存放的值。
        - 打印位于UB内存上的LocalTensor中存放的数据。

            > [!NOTE]
            >
            > UB内存打印起始地址需参考LocalTensor变量展示的**address_**字段中的bufferAddr参数。此处以变量**xLocal**为例，其内存起始地址为**0**。

            ```bash
            (msdebug) print xLocal
            (AscendC::LocalTensor<short>) $0 = {
              address_ = (dataLen = 256, bufferAddr = 0, bufferHandle = "", logicPos = '\t')
              shapeInfo_ = {
                shapeDim = '\0'
                originalShapeDim = '\0'
                shape = ([0] = 0, [1] = 0, [2] = 0, [3] = 0, [4] = 0, [5] = 0, [6] = 0, [7] = 0)
                originalShape = ([0] = 0, [1] = 0, [2] = 0, [3] = 0, [4] = 0, [5] = 0, [6] = 0, [7] = 0)
                dataFormat = ND
              }
            }
            (msdebug) memory read -m UB -f int16_t[] 0 -s 256 -c 1
            0x00000000: {0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127}
            (msdebug)
            ```

        - 打印位于GM内存上的GlobalTensor中存放的数据。

            > [!NOTE]
            >
            > GM内存打印的起始地址需参考GlobalTensor变量展示的**address_**字段。此处以变量**xGm**为例，其内存起始地址为**0x00001240c0015000**。

            ```bash
            (msdebug) print xGm
            (AscendC::GlobalTensor<short>) $0 = {
              bufferSize_ = 2048
              shapeInfo_ = {
                shapeDim = '\0'
                originalShapeDim = '\0'
                shape = ([0] = 0, [1] = 0, [2] = 0, [3] = 0, [4] = 0, [5] = 0, [6] = 0, [7] = 0)
                originalShape = ([0] = 0, [1] = 0, [2] = 0, [3] = 0, [4] = 0, [5] = 0, [6] = 0, [7] = 0)
                dataFormat = ND
              }
              address_ = 0x00001240c0015000
            }
            (msdebug) memory read -m GM -f int16_t[] 0x00001240c0015000 -s 256 -c 1
            0x1240c0015000: {0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127}
            ```

    - 进行核切换，切换至另一个aiv核，并打印需要的信息。

        ```cpp
        (msdebug) ascend aiv 24  // ascend info cores中选择block 7对应的coreId,此处为24
        [Switching to focus on Kernel add_custom, CoreId 24, Type aiv]
        * thread #1, name = 'add.fatbin', stop reason = breakpoint 2.1
            frame #0: 0x0000000000004974 device_debugdata`::add_custom(uint8_t *, uint8_t *, uint8_t *) [inlined] KernelAdd::Compute(this=0x00000000001c6930, progress=0) at add_kernel.cpp:69:9
           66              // call Add instr for computation
           67              Add(zLocal, xLocal, yLocal, TILE_LENGTH);
           68              // enqueue the output tensor to VECOUT queue
        -> 69              outQueueZ.EnQue<int16_t>(zLocal);
                        ^
           70              // free input tensors for reuse
           71              inQueueX.FreeTensor(xLocal);
           72              inQueueY.FreeTensor(yLocal);
        (msdebug) p xLocal
        (AscendC::LocalTensor<short>) $0 = {
          address_ = (dataLen = 256, bufferAddr = 0, bufferHandle = "", logicPos = '\t')
          shapeInfo_ = {
            shapeDim = '\0'
            originalShapeDim = '\0'
            shape = ([0] = 0, [1] = 0, [2] = 0, [3] = 0, [4] = 0, [5] = 0, [6] = 0, [7] = 0)
            originalShape = ([0] = 0, [1] = 0, [2] = 0, [3] = 0, [4] = 0, [5] = 0, [6] = 0, [7] = 0)
            dataFormat = ND
          }
        }
        (msdebug) memory read -m UB -f int16_t[] 0 -s 256 -c 1
        0x00000000: {14336 14337 14338 14339 14340 14341 14342 14343 14344 14345 14346 14347 14348 14349 14350 14351 14352 14353 14354 14355 14356 14357 14358 14359 14360 14361 14362 14363 14364 14365 14366 14367 14368 14369 14370 14371 14372 14373 14374 14375 14376 14377 14378 14379 14380 14381 14382 14383 14384 14385 14386 14387 14388 14389 14390 14391 14392 14393 14394 14395 14396 14397 14398 14399 14400 14401 14402 14403 14404 14405 14406 14407 14408 14409 14410 14411 14412 14413 14414 14415 14416 14417 14418 14419 14420 14421 14422 14423 14424 14425 14426 14427 14428 14429 14430 14431 14432 14433 14434 14435 14436 14437 14438 14439 14440 14441 14442 14443 14444 14445 14446 14447 14448 14449 14450 14451 14452 14453 14454 14455 14456 14457 14458 14459 14460 14461 14462 14463}
        (msdebug)
        ```

5. 查询并删除断点，恢复程序运行。

    ```bash
    (msdebug) breakpoint list
    Current breakpoints:
    1: name = 'main', locations = 1, resolved = 1, hit count = 1
      1.1: where = add.fatbin`main + 36 at main.cpp:39:12, address = 0x0000aaaaaab0f568, resolved, hit count = 1
    2: file = 'add_kernel.cpp', line = 69, exact_match = 0, locations = 1, resolved = 1, hit count = 1
      2.1: where = device_debugdata`::add_custom(uint8_t *, uint8_t *, uint8_t *) + 18804 [inlined] KernelAdd::Compute(int) + 5144 at add_kernel.cpp:69:9, address = 0x0000000000004974, resolved, hit count = 1
    (msdebug) breakpoint delete 2
    1 breakpoints deleted; 0 breakpoint locations disabled.
    (msdebug) continue
    Process 730254 resuming
    0 2 4 6 8 10 12 14
    16 18 20 22 24 26 28 30
    Process 730254 exited with status = 0 (0x00000000)
    ```

6. 调试完以后，执行q命令并输入Y或y结束调试。

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```

## 调用Ascend CL单算子

**前期准备**

单击[链接](https://gitcode.com/cann/cann-learning-hub/tree/master/tutorials/ascendc_operator_development/03_intermediate_vector_operator_development/src/custom_op_04)获取算子样例工程，为进行算子调试做准备。

> [!NOTE]
>
>- 此样例工程不支持Atlas A3 系列产品。
>- 下载代码样例时，需执行以下命令指定分支版本。
>
>    ```bash
>    git clone https://gitcode.com/cann/cann-learning-hub.git -b 8.5.0
>    ```

**操作步骤**

1. 切换到自定义算子编译目录。

    ```bash
    cd ${git_clone_path}/cann-learning-hub/tutorials/ascendc_operator_development/03_intermediate_vector_operator_development/src/custom_op_04
    ```

2. 在`op_kernel/CMakeLists.txt`中添加kernel侧调试编译选项，使能kernel编译器调试功能。

    > [!NOTE]
    >
    > `-DCMAKE_BUILD_TYPE=Debug`仅影响Host侧C++编译，kernel侧需通过`npu_op_kernel_options`显式添加`-g -O0`选项。

    ```cmake
    # op_kernel/CMakeLists.txt 中 npu_op_kernel_sources 之前添加
    if ("${CMAKE_BUILD_TYPE}x" STREQUAL "Debugx")
        npu_op_kernel_options(ascendc_kernels ALL OPTIONS -g -O0)
    endif()
    ```

3. 执行以下命令，以Debug模式编译算子并部署。<a id="步骤3算子编译"></a>

    ```bash
    source $ASCEND_HOME_PATH/bin/setenv.bash
    mkdir -p build && cd build
    cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j binary package
    bash custom_opp_*.run -q
    source $ASCEND_HOME_PATH/opp/vendors/customize/bin/set_env.bash
    ```

4. 切换到单算子调用应用目录，编译并得到可执行文件**execute_add_op**。<a id="步骤4"></a>

    ```bash
    cd ${git_clone_path}/cann-learning-hub/tutorials/ascendc_operator_development/03_intermediate_vector_operator_development/src/custom_op_04/test
    export DDK_PATH=$ASCEND_HOME_PATH
    export NPU_HOST_LIB=$ASCEND_HOME_PATH/lib64
    mkdir -p build && cd build
    cmake .. && make -j
    ```

5. 使用msDebug工具加载[4](#步骤4)中得到的单算子可执行文件execute_add_op。

    ```bash
    export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/opp/vendors/customize/op_api/lib:$LD_LIBRARY_PATH
    cd ${git_clone_path}/cann-learning-hub/tutorials/ascendc_operator_development/03_intermediate_vector_operator_development/src/custom_op_04/test/build
    msdebug execute_add_op
    (msdebug) target create "execute_add_op"
    Current executable set to '.../custom_op_04/test/build/execute_add_op' (aarch64).
    (msdebug)
    ```

6. 断点设置。

    ```bash
    (msdebug) b add_custom_template.cpp:78
    Breakpoint 1: no locations (pending on future shared library load).
    WARNING:  Unable to resolve breakpoint to any actual locations.
    ```

7. 运行算子程序，等待直到命中断点。

    ```bash
    (msdebug) r
    Process 82475 launched: '.../custom_op_04/test/build/execute_add_op' (aarch64)
    1 location added to breakpoint 1
    [Launch of Kernel AddCustomTemplate_862645f6bafeab7b8e397662e7e517c2_0 on Device 0]
    1 location added to breakpoint 1
    Process 82475 stopped
    [Switching to focus on Kernel AddCustomTemplate_862645f6bafeab7b8e397662e7e517c2_0, CoreId 0, Type aiv]
    * thread #1, name = 'execute_add_op', stop reason = breakpoint 1.3
        frame #0: 0x000012c041201cd0 device_debugdata_0`KernelAdd<half, half, half>::Compute(this=0x00000000001cf840, progress=0) (.vector) at add_custom_template.cpp:78:17
       75           AscendC::LocalTensor<TYPE_Y> yLocal = inQueueY.DeQue<TYPE_Y>();
       76           AscendC::LocalTensor<TYPE_Z> zLocal = outQueueZ.AllocTensor<DTYPE_Z>();
       77           AscendC::Add(zLocal, xLocal, yLocal, this->processDataNum);
    -> 78           outQueueZ.EnQue<TYPE_Z>(zLocal);
       79           inQueueX.FreeTensor(xLocal);
       80           inQueueY.FreeTensor(yLocal);
       81       }
    (msdebug)
    ```

    > [!NOTE]
    >
    > 后续调试过程可参考[导入调试信息](../user_guide/msdebug_user_guide.md#工具使用)、[内存与变量打印功能介绍](../user_guide/msdebug_user_guide.md#内存与变量打印功能介绍)及[核切换功能介绍](../user_guide/msdebug_user_guide.md#核切换功能介绍)等，与其操作一致。

## 调试PyTorch接口调用的算子

**概述**

展示如何使用msDebug工具来上板调试一个PyTorch接口调用的add算子，该add算子可实现两个向量相加并输出结果的功能。

**前期准备**

- 单击[链接](https://gitcode.com/cann/cann-learning-hub/tree/6dfac1dad4b8eb9a3cd08322c76f98039480ea31/tutorials/ascendc_operator_development_light/02_AscendC_basic/src/02.06/kernel_pytorch_call)获取样例工程，为进行算子调试做准备。

    > [!NOTE]
    >
    > - 此样例使用torch.library机制将Ascend C Kernel注册为PyTorch自定义算子，编译为独立动态库，无需重编torch_npu。
    > - 此样例工程不支持Atlas A3 系列产品。
    > - 下载代码样例时，需执行以下命令指定分支版本。
    >
    >    ```bash
    >    git clone https://gitcode.com/cann/cann-learning-hub.git && cd cann-learning-hub && git checkout 6dfac1dad4b8
    >    ```

- 已参考《[TorchNPU软件安装](https://gitcode.com/Ascend/pytorch/blob/v2.7.1-26.1.0/docs/zh/installation_guide/installation_description.md)》，完成PyTorch框架和torch_npu插件的安装。
- 完成相关环境变量配置，请参见[MindStudio Debugger工具用户指南](../user_guide/msdebug_user_guide.md)。

**操作步骤**

1. 进入到样例目录，该样例使用torch.library机制将add_custom Kernel注册为PyTorch自定义算子。

    ```bash
    cd ${git_clone_path}/cann-learning-hub/tutorials/ascendc_operator_development_light/02_AscendC_basic/src/02.06/kernel_pytorch_call/torch_library
    ```

    > [!NOTE]
    >
    > PyTorch接入工程的样例工程目录如下：
    >
    > ```text
    > torch_library
    > ├── CMakeLists.txt        // 编译工程文件
    > ├── torch_binding.cpp     // torch.library封装与算子注册
    > ├── add_custom_test.py    // PyTorch调用与精度校验脚本
    > └── README.md             // 样例说明文档
    > ```
    >
    > Kernel实现位于上级目录 `../common/add_custom.asc`。

2. 在CMakeLists.txt中添加调试编译选项，使能kernel编译器和host编译器的调试功能。

    > [!NOTE]
    >
    > 不添加调试编译选项时，编译产物不含调试信息，msdebug无法命中源码断点。

    ```cmake
    # kernel侧（ASC编译器）添加 -O0 -g --cce-ignore-always-inline=true
    target_compile_options(ascendc_kernels PRIVATE
        $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=${CMAKE_ASC_ARCHITECTURES} -O0 -g --cce-ignore-always-inline=true>
    )

    # host侧（C++编译器）添加 -O0 -g
    target_compile_options(ascendc_ops PRIVATE
        ${TORCH_CXX_FLAGS}
        -O0 -g
    )
    ```

3. 执行以下命令，编译算子并生成动态库。

    ```bash
    source ${install_path}/cann/set_env.sh
    mkdir -p build && cd build
    cmake -DCMAKE_ASC_ARCHITECTURES=dav-2201 ..
    make -j
    ```

    编译完成后，在build目录下生成`libascendc_kernels.so`和`libascendc_ops.so`。

4. 执行样例，验证算子功能正确性。

    ```bash
    python3 ../add_custom_test.py
    Ran 1 test in 1.341s
    OK
    ```

5. 启动msDebug工具拉起Python程序，进入调试界面。

    ```bash
    msdebug python3 ../add_custom_test.py
    (msdebug) target create "python3"
    Current executable set to '/opt/buildtools/Python-3.11.4/bin/python3' (aarch64).
    (msdebug) settings set -- target.run-args  "add_custom_test.py"
    (msdebug)
    ```

6. 设置断点。

    根据指定源码文件与对应行号，在核函数中设置NPU断点。

    ```bash
    (msdebug) b add_custom.asc:34
    Breakpoint 1: no locations (pending on future shared library load).
    WARNING:  Unable to resolve breakpoint to any actual locations.
    ```

7. 运行程序，等待直到命中断点。

    ```bash
    (msdebug) r
    Process 74664 launched: '/opt/buildtools/Python-3.11.4/bin/python3' (aarch64)
    Process 74664 stopped and restarted: thread 2 received signal: SIGCHLD
    Process 74664 stopped and restarted: thread 1 received signal: SIGCHLD
    1 location added to breakpoint 1
    [Launch of Kernel _Z10add_customPhS_S_j on Device 0]
    1 location added to breakpoint 1
    Process 74664 stopped
    [Switching to focus on Kernel _Z10add_customPhS_S_j, CoreId 8, Type aiv]
    * thread #1, name = 'python3', stop reason = breakpoint 1.2
        frame #0: 0x000012c041400604 device_debugdata_0`add_custom(x=0x12c041200000, y=0x12c041208200, z=0x12c041210400, totalLength=16384) at add_custom.asc:34:15
       31       yLocal = inQueueY.DeQue<half>();
       32       AscendC::LocalTensor<half> zLocal = outQueueZ.AllocTensor<half>();
       33       AscendC::Add(zLocal, xLocal, yLocal, blockLength);
    -> 34       outQueueZ.EnQue<half>(zLocal);
       35       inQueueX.FreeTensor(xLocal);
       36       inQueueY.FreeTensor(yLocal);
    (msdebug)
    ```

    > [!NOTE]
    >
    > 其他调试操作可参考[导入调试信息](../user_guide/msdebug_user_guide.md#工具使用)、[内存与变量打印功能介绍](../user_guide/msdebug_user_guide.md#内存与变量打印功能介绍)、[调试信息展示功能介绍](../user_guide/msdebug_user_guide.md#调试信息展示功能介绍)及[核切换功能介绍](../user_guide/msdebug_user_guide.md#核切换功能介绍)等，与其操作一致。

8. 删除断点，具体操作请参见[断点设置功能介绍](../user_guide/msdebug_user_guide.md#断点设置功能介绍)。
9. 调试完以后，执行q命令并输入Y或y结束调试。

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```

## 上板调试模板库的算子

**概述**

展示如何使用msDebug工具来上板调试一个模板库算子（matmul），该算子可实现两个矩阵相乘并输出结果的功能。

**前期准备**<a id="前期准备"></a>

- 单击[链接](https://gitcode.com/cann/catlass)获取样例工程，为进行算子调试做准备。
- 完成相关环境变量配置，请参见[MindStudio Debugger工具用户指南](../user_guide/msdebug_user_guide.md)。

**操作步骤**

1. 基于[前期准备](#前期准备)中的样例工程编译算子，获取可执行文件00_basic_matmul。

    执行以下命令完成算子编译，编译完成后，在output/bin目录下生成可执行文件00_basic_matmul。

    ```bash
    bash ./scripts/build.sh 00_basic_matmul --debug --msdebug
    ```

2. 启动msDebug工具拉起算子程序，进入调试界面。

    ```bash
    msdebug ./output/bin/00_basic_matmul 256 512 1024 0
    (msdebug) target create "./output/bin/00_basic_matmul"
    Current executable set to '/home/mindstudio/projects/ascendc-templates/output/bin/00_basic_matmul' (aarch64).
    (msdebug)
    ```

3. 设置断点。

    该用例中核函数的代码实现位于basic_matmul.hpp中，在此文件中，为需要的代码行设置NPU断点。

    ```bash
    (msdebug) b basic_matmul.hpp:121
    Breakpoint 1: 2 locations.
    (msdebug)
    ```

4. 运行算子程序，等待直到命中断点。

    程序会开始运行直到命中第一个断点（basic_matmul.hpp:121）后停下，msDebug检测到NPU核函数开始运行，运行在Device 0。

    ```cpp
    (msdebug) run
    Process 3344307 launched: '/home/mindstudio/projects/ascendc-templates/output/bin/00_basic_matmul' (aarch64)
    [Launch of Kernel _ZN7Catlass13KernelAdapterINS_4Gemm6Kernel11BasicMatmulINS1_5Blo on Device 0]
    Process 3344307 stopped
    [Switching to focus on Kernel _ZN7Catlass13KernelAdapterINS_4Gemm6Kernel11BasicMatmulINS1_5Blo, CoreId 21, Type aic]
    * thread #1, name = '00_basic_matmul', stop reason = breakpoint 1.1
        frame #0: 0x0000000000001c38 device_debugdata`_ZN7Catlass13KernelAdapterINS_4Gemm6Kernel11BasicMatmulINS1_5Block9BlockMmadINS1_19MmadAtlasA2PingpongILb1EEENS_9GemmShapeILj128ELj256ELj256EEENS8_ILj128ELj256ELj64EEENS1_8GemmTypeIDhNS_6layout8RowMajorELN7AscendC9TPositionE0EEESG_SG_vNS1_4Tile8TileCopyINS_4Arch7AtlasA2ESG_SG_SG_vEENSH_8TileMmadISK_SG_SG_vEEEEvNS4_24GemmIdentityBlockSwizzleILj3ELj0EEEEEEEvNT_6ParamsE_mix_aic at basic_matmul.hpp:121:71
       118
       119          for (uint32_t loopIdx = AscendC::GetBlockIdx(); loopIdx < coreLoops; loopIdx += AscendC::GetBlockNum()) {
       120              // Compute block location
    -> 121              GemmCoord blockCoord = matmulBlockScheduler.GetBlockCoord(loopIdx);
       122              GemmCoord actualBlockShape = matmulBlockScheduler.GetActualBlockShape(blockCoord);
       123
       124              // Compute initial location in logical coordinates
    (msdebug)
    ```

    > [!NOTE]
    >
    > **_ZN7Catlass13KernelAdapterINS_4Gemm6Kernel11BasicMatmulINS1_5Blo**为模板库的kernel名字，示例仅显示前面64位。

5. 检视信息。

    - 使用ascend info cores命令查询NPU核信息。

        ```bash
        (msdebug) ascend info cores
          CoreId Type Device Stream Task Block               PC    stop reason Filename Line
        *    21  aic      0    48    0     0  0x12c0c00d6c38  breakpoint 1.1       NA   NA
             22  aic      0    48    0     1  0x12c0c00d6c38  breakpoint 1.1       NA   NA
             23  aic      0    48    0     2  0x12c0c00d6c38  breakpoint 1.1       NA   NA
             24  aic      0    48    0     3  0x12c0c00d6c38  breakpoint 1.1       NA   NA
        (msdebug)
        ```

    - 使用print命令直接打印**gmA**变量信息。

        ```bash
        (msdebug) print gmA
        (AscendC::GlobalTensor<__fp16>) $0 = {
          AscendC::BaseGlobalTensor<__fp16> = {
            address_ = 0x000012c0c0013000
            oriAddress_ = 0x000012c0c0013000
          }
          bufferSize_ = 0
          cacheMode_ = CACHE_MODE_NORMAL
        }
        ```

    - 继续使用memory read命令可打印出gmA变量中存放的值。
        - 打印位于GM内存上的gmA中存放的数据。

            ```bash
            (msdebug) memory read -m GM 0x12c0c0013000 -f float16[] -s 256 -c 1
            0x12c0c0013000: {3.40234 -1.05664 2.83008 2.98438 4.11719 -3.02539 -1.64746 2.68164 -2.22266 0.539551 -0.226074 1.28906 -1.35254 0.134033 4.52344 4.16016 1.35742 2.17383 -3.58398 1.06934 -4.83594 -2.57031 -3.62695 3.04102 -3.43359 -0.990723 -3.70117 -3.91211 4.98828 -2.81836 0.129272 3.39062 1.12598 -2.03906 1.37598 0.24292 -0.0641479 4.72656 -2.07422 2.71289 0.267334 2.69922 -0.997559 3.91602 -2.16602 -1.47559 3.07812 4.19141 -4.30078 4.49219 0.26001 -4.14062 -3.07812 1.63184 3.90234 -1.51074 -4.35938 -4.80078 -0.423096 -4.36719 -2.61719 4.70703 4.02344 3.50977 -2.33398 0.397705 -1.24805 2.60156 0.125366 1.67676 0.316162 -4.60547 -0.623535 4.31641 4.30859 2.20898 -2.15625 2.38477 1.39941 -1.45996 1.87891 -3.33984 -0.599121 3.80078 3.29297 -1.69629 -2.71094 3.93359 -1.49609 1.86621 4.56641 0.88623 1.57324 3.58594 -0.604492 4.23828 -1.01562 3.14844 1.8418 4.10938 -0.175049 -2.8418 4.50391 4.20312 -3.52344 3.81055 1.41113 -0.680664 1.19629 -2.18945 2.85938 -1.92578 -0.529785 -2.73828 -3.125 -2.23828 0.564453 -0.834961 -3.30469 4.06641 -3.96875 -3.73828 -0.0455627 2.60547 4.84766 4.35156 1.84473 -1.16797}
            (msdebug)
            ```

    - 进行核切换，切换至另一个aic核，并打印需要的信息。

        ```cpp
        (msdebug) ascend aic 24  // ascend info cores中选择block 3对应的coreId,此处为24
        [Switching to focus on Kernel _ZN7Catlass13KernelAdapterINS_4Gemm6Kernel11BasicMatmulINS1_5Blo, CoreId 24, Type aic]
        * thread #1, name = '00_basic_matmul', stop reason = breakpoint 1.1
            frame #0: 0x0000000000001c38 device_debugdata`_ZN7Catlass13KernelAdapterINS_4Gemm6Kernel11BasicMatmulINS1_5Block9BlockMmadINS1_19MmadAtlasA2PingpongILb1EEENS_9GemmShapeILj128ELj256ELj256EEENS8_ILj128ELj256ELj64EEENS1_8GemmTypeIDhNS_6layout8RowMajorELN7AscendC9TPositionE0EEESG_SG_vNS1_4Tile8TileCopyINS_4Arch7AtlasA2ESG_SG_SG_vEENSH_8TileMmadISK_SG_SG_vEEEEvNS4_24GemmIdentityBlockSwizzleILj3ELj0EEEEEEEvNT_6ParamsE_mix_aic at basic_matmul.hpp:121:71
           118
           119          for (uint32_t loopIdx = AscendC::GetBlockIdx(); loopIdx < coreLoops; loopIdx += AscendC::GetBlockNum()) {
           120              // Compute block location
        -> 121              GemmCoord blockCoord = matmulBlockScheduler.GetBlockCoord(loopIdx);
           122              GemmCoord actualBlockShape = matmulBlockScheduler.GetActualBlockShape(blockCoord);
           123
           124              // Compute initial location in logical coordinates
        (msdebug) p loopIdx
        (uint32_t) $1 = 0
        ```

    - 查看当前断点附近的反汇编代码。

        ```cpp
        (msdebug) dis -p
        device_debugdata_0`void Catlass::Gemm::Kernel::BasicMatmul<Catlass::Gemm::Block::BlockMmad<Catlass::Gemm::MmadAtlasA2Pingpong<true>, Catlass::GemmShape<128u, 256u, 256u>, Catlass::GemmShape<128u, 256u, 64u>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, void, Catlass::Gemm::Tile::TileCopy<Catlass::Arch::AtlasA2, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, void>, Catlass::Gemm::Tile::TileMmad<Catlass::Arch::AtlasA2, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, void>, void>, void, Catlass::Gemm::Block::GemmIdentityBlockSwizzle<3u, 0u>>::operator()<1>(Catlass::Gemm::Kernel::BasicMatmul<Catlass::Gemm::Block::BlockMmad<Catlass::Gemm::MmadAtlasA2Pingpong<true>, Catlass::GemmShape<128u, 256u, 256u>, Catlass::GemmShape<128u, 256u, 64u>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, void, Catlass::Gemm::Tile::TileCopy<Catlass::Arch::AtlasA2, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, void>, Catlass::Gemm::Tile::TileMmad<Catlass::Arch::AtlasA2, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, Catlass::Gemm::GemmType<half, Catlass::layout::RowMajor, (AscendC::TPosition)0>, void>, void>, void, Catlass::Gemm::Block::GemmIdentityBlockSwizzle<3u, 0u>>::Params const&) (.cube):
        ->  0x12c041400de4 <+556>: LD.b32                        X1, [X30], #-1508, #0
            0x12c041400de8 <+560>: BAR.ALL                       <_ZN7AscendCL23CheckLoadData2dDatatypeIDhEEvv.cube+0x6>
            0x12c041400dec <+564>: ST.b64                        X2, [X30], #-1808, #0
            0x12c041400df0 <+568>: ADD.s64                       X0, X30, #352
        ```

    > [!NOTE]
    >
    > 其他调试操作可参考[内存与变量打印功能介绍](../user_guide/msdebug_user_guide.md#内存与变量打印功能介绍)、[调试信息展示功能介绍](../user_guide/msdebug_user_guide.md#调试信息展示功能介绍)及[核切换功能介绍](../user_guide/msdebug_user_guide.md#核切换功能介绍)等，与其操作一致。

6. 查询并删除断点，恢复程序运行。

    ```bash
    (msdebug) breakpoint list
    Current breakpoints:
    1: file = 'basic_matmul.hpp', line = 121, exact_match = 0, locations = 2, resolved = 2, hit count = 1
      1.1: where = device_debugdata`_ZN7Catlass13KernelAdapterINS_4Gemm6Kernel11BasicMatmulINS1_5Block9BlockMmadINS1_19MmadAtlasA2PingpongILb1EEENS_9GemmShapeILj128ELj256ELj256EEENS8_ILj128ELj256ELj64EEENS1_8GemmTypeIDhNS_6layout8RowMajorELN7AscendC9TPositionE0EEESG_SG_vNS1_4Tile8TileCopyINS_4Arch7AtlasA2ESG_SG_SG_vEENSH_8TileMmadISK_SG_SG_vEEEEvNS4_24GemmIdentityBlockSwizzleILj3ELj0EEEEEEEvNT_6ParamsE_mix_aic + 4748 [inlined] _ZN7Catlass4Gemm6Kernel11BasicMatmulINS0_5Block9BlockMmadINS0_19MmadAtlasA2PingpongILb1EEENS_9GemmShapeILj128ELj256ELj256EEENS7_ILj128ELj256ELj64EEENS0_8GemmTypeIDhNS_6layout8RowMajorELN7AscendC9TPositionE0EEESF_SF_vNS0_4Tile8TileCopyINS_4Arch7AtlasA2ESF_SF_SF_vEENSG_8TileMmadISJ_SF_SF_vEEEEvNS3_24GemmIdentityBlockSwizzleILj3ELj0EEEEclILi1EEEvRKNSQ_6ParamsE_mix_aic + 4632 at basic_matmul.hpp:121:71, address = 0x0000000000001c38, resolved, hit count = 1
      1.2: where = device_debugdata`_ZN7Catlass13KernelAdapterINS_4Gemm6Kernel11BasicMatmulINS1_5Block9BlockMmadINS1_19MmadAtlasA2PingpongILb1EEENS_9GemmShapeILj128ELj256ELj256EEENS8_ILj128ELj256ELj64EEENS1_8GemmTypeIDhNS_6layout8RowMajorELN7AscendC9TPositionE0EEESG_SG_vNS1_4Tile8TileCopyINS_4Arch7AtlasA2ESG_SG_SG_vEENSH_8TileMmadISK_SG_SG_vEEEEvNS4_24GemmIdentityBlockSwizzleILj3ELj0EEEEEEEvNT_6ParamsEm_mix_aic + 4772 [inlined] _ZN7Catlass4Gemm6Kernel11BasicMatmulINS0_5Block9BlockMmadINS0_19MmadAtlasA2PingpongILb1EEENS_9GemmShapeILj128ELj256ELj256EEENS7_ILj128ELj256ELj64EEENS0_8GemmTypeIDhNS_6layout8RowMajorELN7AscendC9TPositionE0EEESF_SF_vNS0_4Tile8TileCopyINS_4Arch7AtlasA2ESF_SF_SF_vEENSG_8TileMmadISJ_SF_SF_vEEEEvNS3_24GemmIdentityBlockSwizzleILj3ELj0EEEEclILi1EEEvRKNSQ_6ParamsE_mix_aic + 4632 at basic_matmul.hpp:121:71, address = 0x000000000000dd54, resolved, hit count = 0
    (msdebug) breakpoint delete 1
    1 breakpoints deleted; 0 breakpoint locations disabled.
    (msdebug) continue
    Process 3344307 resuming
    Compare success.
    Process 3344307 exited with status = 0 (0x00000000)
    ```

7. 调试完以后，执行q命令并输入Y或y结束调试。

    ```bash
    (msdebug) q
    ```
