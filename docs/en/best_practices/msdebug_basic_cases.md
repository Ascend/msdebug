# **Typical Cases**

## Debugging a Vector Operator on the Board

**Overview**

This section shows how to use msDebug to debug a vector operator on the board. The vector operator can add two vectors and output the result.

**Preparations**

- Obtain the [sample project](https://gitcode.com/Ascend/mstt/tree/7.0.RC3/sample).
- Configure related environment variables. For details, see the [MindStudio Debugger User Guide](../user_guide/msdebug_user_guide.md).

**Procedure**

1. Compile the operator based on the sample project and obtain the executable file `add.fatbin`.
    1. Modify the `COMPILER_FLAG` option in the `sample/normal_sample/vec_only/Makefile` file, that is, change `-O2` to `-O0 -g --cce-ignore-always-inline=true` to enable the compiler debugging function.

        ```bash
        # Makefile
        ...
        COMPILER            := $(ASCEND_HOME_PATH)/compiler/ccec_compiler/bin/ccec
        COMPILER_FLAG       := -xcce -O0 -g --cce-ignore-always-inline=true -std=c++17 # Enable compiler debugging
        ```

    2. Compile the operator.

        > [!NOTE]
        >
        > In non-initial scenarios, you can run the `make clean && make` command instead of the `make` command.

        ```bash
        cd ./mstt/sample/normal_sample/vec_only/
        make clean && make
        ```

2. Set a breakpoint.
    1. Start msDebug to start the operator program and enter the debugging page.

        ```bash
        msdebug add.fatbin
        (msdebug) target create "add.fatbin"
        Current executable set to '/home/mindstudio/projects/mstt/sample/build/add.fatbin' (aarch64).
        (msdebug)
        ```

    2. In this sample, the implementation code of the kernel function is stored in `add_kernel.cpp`. Set NPU breakpoints in this file for required code lines.

        ```bash
        (msdebug) b add_kernel.cpp:69
        Breakpoint 1: where = device_debugdata`::add_custom(uint8_t *, uint8_t *, uint8_t *) + 18804 [inlined]
        KernelAdd::Compute(int) + 5144 at add_kernel.cpp:69:9, address = 0x0000000000004974
        (msdebug)
        ```

3. Run the operator program.

    The program starts to run until the first breakpoint (`add_kernel.cpp:69`) is hit. msDebug detects that the NPU core function `add_custom` starts to run on device 0.

    ```cpp
    (msdebug) run
    Process 730254 launched
    [Launch of Kernel add_custom on Device 0]
    Process 730254 stopped
    [Switching to focus on Kernel add_custom, CoreId 13, Type aiv]
    * thread #1, name = 'add.fatbin', stop reason = breakpoint 2.1
        frame #0: 0x0000000000004974 device_debugdata`::add_custom(uint8_t *, uint8_t *, uint8_t *) [inlined] KernelAdd::Compute(this=0x000000000019a930, progress=0) at add_kernel.cpp:69:9
       66              // Call Add instr for computation
       67              Add(zLocal, xLocal, yLocal, TILE_LENGTH);
       68              // Enqueue the output tensor to VECOUT queue
    -> 69              outQueueZ.EnQue<int16_t>(zLocal);  # Breakpoint position
       70              // Free input tensors for reuse
       71              inQueueX.FreeTensor(xLocal);
       72              inQueueY.FreeTensor(yLocal);
    (msdebug)
    ```

4. Review information.
    - Run the `ascend info cores` command to query NPU core information.

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

    - Run the `print` command to print variable information.

        ```bash
        (msdebug) print progress
        (int32_t) $0 = 0
        ```

    - Run the `print` and `memory read` commands together to print values stored in the tensor variables.
        - Print the data stored in `LocalTensor` in the UB memory.

            > [!NOTE]
            >
            > For details about the start address for printing the UB memory, see the `bufferAddr` parameter in the `address_` field of the `LocalTensor` variable. The following uses the `xLocal` variable as an example. The start address of the memory is 0.

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

        - Print the data stored in `GlobalTensor` in the GM.

            > [!NOTE]
            >
            > For details about the start address for GM memory printing, see the `address_` field of the `GlobalTensor` variable. The following uses the `xGm` variable as an example. The start address of the memory is 0x00001240c0015000.

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

    - Switch to another AIV core and print the required information.

        ```cpp
        (msdebug) ascend aiv 24  // Select the core ID corresponding to block 7 in ascend info cores. In this example, the core ID is 24
        [Switching to focus on Kernel add_custom, CoreId 24, Type aiv]
        * thread #1, name = 'add.fatbin', stop reason = breakpoint 2.1
            frame #0: 0x0000000000004974 device_debugdata`::add_custom(uint8_t *, uint8_t *, uint8_t *) [inlined] KernelAdd::Compute(this=0x00000000001c6930, progress=0) at add_kernel.cpp:69:9
           66              // Call Add instr for computation
           67              Add(zLocal, xLocal, yLocal, TILE_LENGTH);
           68              // Enqueue the output tensor to VECOUT queue
        -> 69              outQueueZ.EnQue<int16_t>(zLocal);
                        ^
           70              // Free input tensors for reuse
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

5. Query and delete breakpoints to resume program execution.

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

6. After the debugging is complete, run the `q` command and enter `Y` or `y` to end the debugging.

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```

## Calling AscendCL Single-Operator

**Preparations**

Obtain the [sample project](https://gitcode.com/cann/cann-learning-hub/tree/master/tutorials/ascendc_operator_development/03_intermediate_vector_operator_development/src/custom_op_04) for operator debugging.

> [!NOTE]
>
>- This sample project does not support <term>Atlas A3 training products</term>.
>- When downloading the code sample, run the following command to specify the branch version:
>
>    ```bash
>    git clone https://gitcode.com/cann/cann-learning-hub.git -b 8.5.0
>    ```

**Procedure**

1. Go to the custom operator compilation directory.

    ```bash
    cd ${git_clone_path}/cann-learning-hub/tutorials/ascendc_operator_development/03_intermediate_vector_operator_development/src/custom_op_04
    ```

2. Add the kernel-side debugging compilation option in `op_kernel/CMakeLists.txt` to enable the debugging function of the kernel compiler.

    > [!NOTE]
    >
    > `-DCMAKE_BUILD_TYPE=Debug` affects only host-side C++ compilation. For the kernel side, explicitly add the `-g -O0` option through `npu_op_kernel_options`.

    ```cmake
    # Add the following content before npu_op_kernel_sources in op_kernel/CMakeLists.txt
    if ("${CMAKE_BUILD_TYPE}x" STREQUAL "Debugx")
        npu_op_kernel_options(ascendc_kernels ALL OPTIONS -g -O0)
    endif()
    ```

3. Run the following commands to compile and deploy the operator in Debug mode. <a id="step-3-operator-compilation"></a>

    ```bash
    source $ASCEND_HOME_PATH/bin/setenv.bash
    mkdir -p build && cd build
    cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j binary package
    bash custom_opp_*.run -q
    source $ASCEND_HOME_PATH/opp/vendors/customize/bin/set_env.bash
    ```

4. Go to the single-operator calling application directory, and compile the application to obtain the executable file `execute_add_op`. <a id="step-4"></a>

    ```bash
    cd ${git_clone_path}/cann-learning-hub/tutorials/ascendc_operator_development/03_intermediate_vector_operator_development/src/custom_op_04/test
    export DDK_PATH=$ASCEND_HOME_PATH
    export NPU_HOST_LIB=$ASCEND_HOME_PATH/lib64
    mkdir -p build && cd build
    cmake .. && make -j
    ```

5. Use msDebug to load the single-operator executable file `execute_add_op` obtained in [4](#step-4).

    ```bash
    export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/opp/vendors/customize/op_api/lib:$LD_LIBRARY_PATH
    cd ${git_clone_path}/cann-learning-hub/tutorials/ascendc_operator_development/03_intermediate_vector_operator_development/src/custom_op_04/test/build
    msdebug execute_add_op
    (msdebug) target create "execute_add_op"
    Current executable set to '.../custom_op_04/test/build/execute_add_op' (aarch64).
    (msdebug)
    ```

6. Set a breakpoint.

    ```bash
    (msdebug) b add_custom_template.cpp:78
    Breakpoint 1: no locations (pending on future shared library load).
    WARNING:  Unable to resolve breakpoint to any actual locations.
    ```

7. Run the operator program and wait until the breakpoint is hit.

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
    > For details about the subsequent debugging process, see [Importing Debugging Information](../user_guide/msdebug_user_guide.md#tool-usage), [Memory and Variable Printing](../user_guide/msdebug_user_guide.md#memory-and-variable-printing), and [Core Switching](../user_guide/msdebug_user_guide.md#core-switching).

## Debugging the Operators Called by a PyTorch Interface

**Overview**

This section shows how to use msDebug to debug the add operator called by a PyTorch interface on the board. The add operator can add two vectors and output the result.

**Preparations**

- Obtain the [sample project](https://gitcode.com/cann/cann-learning-hub/tree/6dfac1dad4b8eb9a3cd08322c76f98039480ea31/tutorials/ascendc_operator_development_light/02_AscendC_basic/src/02.06/kernel_pytorch_call) for operator debugging.

    > [!NOTE]
    >
    > - This sample uses the `torch.library` mechanism to register the Ascend C kernel as a PyTorch custom operator and compiles it into a separate dynamic library. Therefore, you do not need to recompile `torch_npu`.
    > - This sample project does not support <term>Atlas A3 training products</term>.
    > - When downloading the code sample, run the following command to specify the branch version:
    >
    >    ```bash
    >    git clone https://gitcode.com/cann/cann-learning-hub.git && cd cann-learning-hub && git checkout 6dfac1dad4b8
    >    ```

- Install the PyTorch framework and TorchNPU plugin by referring to [TorchNPU Software Installation](https://gitcode.com/Ascend/pytorch/blob/v2.7.1-26.1.0/docs/en/installation_guide/installation_description.md).
- Configure related environment variables. For details, see the [MindStudio Debugger User Guide](../user_guide/msdebug_user_guide.md).

**Procedure**

1. Go to the sample directory. This sample uses the `torch.library` mechanism to register the `add_custom` kernel as a PyTorch custom operator.

    ```bash
    cd ${git_clone_path}/cann-learning-hub/tutorials/ascendc_operator_development_light/02_AscendC_basic/src/02.06/kernel_pytorch_call/torch_library
    ```

    > [!NOTE]
    >
    > The sample project directory of the PyTorch integration project is as follows:
    >
    > ```text
    > torch_library
    > ├── CMakeLists.txt        // Build project file
    > ├── torch_binding.cpp     // torch.library encapsulation and operator registration
    > ├── add_custom_test.py    // PyTorch invocation and accuracy verification script
    > └── README.md             // Sample description
    > ```
    >
    > The kernel implementation is located in the `../common/add_custom.asc` file in the parent directory.

2. Add debugging compilation options in `CMakeLists.txt` to enable the debugging function of the kernel compiler and the host compiler.

    > [!NOTE]
    >
    > If you do not add the debugging compilation options, the compilation output does not contain debugging information, and msDebug cannot hit source breakpoints.

    ```cmake
    # Add -O0 -g --cce-ignore-always-inline=true on the kernel side (ASC compiler)
    target_compile_options(ascendc_kernels PRIVATE
        $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=${CMAKE_ASC_ARCHITECTURES} -O0 -g --cce-ignore-always-inline=true>
    )

    # Add -O0 -g on the host side (C++ compiler)
    target_compile_options(ascendc_ops PRIVATE
        ${TORCH_CXX_FLAGS}
        -O0 -g
    )
    ```

3. Run the following commands to compile the operator and generate the dynamic library.

    ```bash
    source ${install_path}/cann/set_env.sh
    mkdir -p build && cd build
    cmake -DCMAKE_ASC_ARCHITECTURES=dav-2201 ..
    make -j
    ```

    After the compilation is complete, `libascendc_kernels.so` and `libascendc_ops.so` are generated in the `build` directory.

4. Run the sample to verify that the operator functions correctly.

    ```bash
    python3 ../add_custom_test.py
    Ran 1 test in 1.341s
    OK
    ```

5. Start msDebug to start the Python program and enter the debugging page.

    ```bash
    msdebug python3 ../add_custom_test.py
    (msdebug) target create "python3"
    Current executable set to '/opt/buildtools/Python-3.11.4/bin/python3' (aarch64).
    (msdebug) settings set -- target.run-args  "add_custom_test.py"
    (msdebug)
    ```

6. Set a breakpoint.

    Set an NPU breakpoint in the kernel function based on the specified source code file and corresponding line number.

    ```bash
    (msdebug) b add_custom.asc:34
    Breakpoint 1: no locations (pending on future shared library load).
    WARNING:  Unable to resolve breakpoint to any actual locations.
    ```

7. Run the program and wait until the breakpoint is hit.

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
    > For details about other debugging operations, see [Importing Debugging Information](../user_guide/msdebug_user_guide.md#tool-usage), [Memory and Variable Printing](../user_guide/msdebug_user_guide.md#memory-and-variable-printing), [Debugging Information Display](../user_guide/msdebug_user_guide.md#debugging-information-display), and [Core Switching](../user_guide/msdebug_user_guide.md#core-switching).

8. Delete the breakpoint. For details, see [Setting Breakpoints](../user_guide/msdebug_user_guide.md#breakpoint-setting).
9. After the debugging is complete, run the `q` command and enter `Y` or `y` to end the debugging.

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```

## Debugging a Template Library Operator on the Board

**Overview**

This section shows how to use msDebug to debug a template library operator (matmul) on the board. The operator can multiply two matrices and output the result.

**Preparations** <a id="preparations"></a>

- Obtain the [sample project](https://gitcode.com/cann/catlass).
- Configure related environment variables. For details, see the [MindStudio Debugger User Guide](../user_guide/msdebug_user_guide.md).

**Procedure**

1. Compile the operator based on the sample project obtained in [Preparations](#preparations) to generate the executable file `00_basic_matmul`.

    Run the following command to compile the operator. After the compilation is complete, the executable file `00_basic_matmul` is generated in the `output/bin` directory.

    ```bash
    bash ./scripts/build.sh 00_basic_matmul --debug --msdebug
    ```

2. Start msDebug to start the operator program and enter the debugging page.

    ```bash
    msdebug ./output/bin/00_basic_matmul 256 512 1024 0
    (msdebug) target create "./output/bin/00_basic_matmul"
    Current executable set to '/home/mindstudio/projects/ascendc-templates/output/bin/00_basic_matmul' (aarch64).
    (msdebug)
    ```

3. Set a breakpoint.

    In this test case, the code implementation of the kernel function is located in `basic_matmul.hpp`. In this file, set an NPU breakpoint for the required code line.

    ```bash
    (msdebug) b basic_matmul.hpp:121
    Breakpoint 1: 2 locations.
    (msdebug)
    ```

4. Run the operator program and wait until the breakpoint is hit.

    The program starts to run until the first breakpoint (`basic_matmul.hpp:121`) is hit. msDebug detects that the NPU core function starts to run on device 0.

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
    > `_ZN7Catlass13KernelAdapterINS_4Gemm6Kernel11BasicMatmulINS1_5Blo` indicates the kernel name of the template library. Only the first 64 characters are displayed in the example.

5. Review information.

    - Run the `ascend info cores` command to query NPU core information.

        ```bash
        (msdebug) ascend info cores
          CoreId Type Device Stream Task Block               PC    stop reason Filename Line
        *    21  aic      0    48    0     0  0x12c0c00d6c38  breakpoint 1.1       NA   NA
             22  aic      0    48    0     1  0x12c0c00d6c38  breakpoint 1.1       NA   NA
             23  aic      0    48    0     2  0x12c0c00d6c38  breakpoint 1.1       NA   NA
             24  aic      0    48    0     3  0x12c0c00d6c38  breakpoint 1.1       NA   NA
        (msdebug)
        ```

    - Run the `print` command to print the `gmA` variable information.

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

    - Run the `memory read` command to print the values stored in the `gmA` variable.
        - Print the data stored in `gmA` in the GM.

            ```bash
            (msdebug) memory read -m GM 0x12c0c0013000 -f float16[] -s 256 -c 1
            0x12c0c0013000: {3.40234 -1.05664 2.83008 2.98438 4.11719 -3.02539 -1.64746 2.68164 -2.22266 0.539551 -0.226074 1.28906 -1.35254 0.134033 4.52344 4.16016 1.35742 2.17383 -3.58398 1.06934 -4.83594 -2.57031 -3.62695 3.04102 -3.43359 -0.990723 -3.70117 -3.91211 4.98828 -2.81836 0.129272 3.39062 1.12598 -2.03906 1.37598 0.24292 -0.0641479 4.72656 -2.07422 2.71289 0.267334 2.69922 -0.997559 3.91602 -2.16602 -1.47559 3.07812 4.19141 -4.30078 4.49219 0.26001 -4.14062 -3.07812 1.63184 3.90234 -1.51074 -4.35938 -4.80078 -0.423096 -4.36719 -2.61719 4.70703 4.02344 3.50977 -2.33398 0.397705 -1.24805 2.60156 0.125366 1.67676 0.316162 -4.60547 -0.623535 4.31641 4.30859 2.20898 -2.15625 2.38477 1.39941 -1.45996 1.87891 -3.33984 -0.599121 3.80078 3.29297 -1.69629 -2.71094 3.93359 -1.49609 1.86621 4.56641 0.88623 1.57324 3.58594 -0.604492 4.23828 -1.01562 3.14844 1.8418 4.10938 -0.175049 -2.8418 4.50391 4.20312 -3.52344 3.81055 1.41113 -0.680664 1.19629 -2.18945 2.85938 -1.92578 -0.529785 -2.73828 -3.125 -2.23828 0.564453 -0.834961 -3.30469 4.06641 -3.96875 -3.73828 -0.0455627 2.60547 4.84766 4.35156 1.84473 -1.16797}
            (msdebug)
            ```

    - Switch to another AIC core and print the required information.

        ```cpp
        (msdebug) ascend aic 24  // Select the core ID corresponding to block 3 in ascend info cores. In this example, the core ID is 24
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

    - View the disassembly code near the current breakpoint.

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
    > For details about other debugging operations, see [Memory and Variable Printing](../user_guide/msdebug_user_guide.md#memory-and-variable-printing), [Debugging Information Display](../user_guide/msdebug_user_guide.md#debugging-information-display), and [Core Switching](../user_guide/msdebug_user_guide.md#core-switching).

6. Query and delete breakpoints to resume program execution.

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

7. After the debugging is complete, run the `q` command and enter `Y` or `y` to end the debugging.

    ```bash
    (msdebug) q
    ```
