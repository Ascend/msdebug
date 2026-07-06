# **Typical Cases**

## Debugging a Vector Operator on the Board

**Overview**

This section shows how to use msDebug to debug a vector operator on the board. The vector operator can add two vectors and output the result.

**Preparations**

- Obtain the [sample project](https://gitcode.com/Ascend/mstt/tree/master/sample).
- Configure related environment variables. For details, see [MindStudio Debugger User Guide](../user_guide/msdebug_user_guide.md).

**Procedure**

1. Compile the operator based on the sample project and obtain the executable file `add.fatbin`.
    1. Modify the `COMPILER\_FLAG` option in the `sample/normal\_sample/vec\_only/Makefile` file, that is, change `-O2` to `-O0 -g --cce-ignore-always-inline=true` to enable the compiler debugging function.

        ```bash
        # Makefile
        ...
        COMPILER            := $(ASCEND_HOME_PATH)/compiler/ccec_compiler/bin/ccec
        COMPILER_FLAG       := -xcce -O0 -g --cce-ignore-always-inline=true -std=c++17 # Enable compiler debugging.
        ```

    2. Compile the operator.

        > [!NOTE]NOTE
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

    2. In this sample, the implementation code of the kernel function is stored in `add\_kernel.cpp`. Set NPU breakpoints in this file for required code lines.

        ```bash
        (msdebug) b add_kernel.cpp:69
        Breakpoint 1: where = device_debugdata`::add_custom(uint8_t *, uint8_t *, uint8_t *) + 18804 [inlined]
        KernelAdd::Compute(int) + 5144 at add_kernel.cpp:69:9, address = 0x0000000000004974
        (msdebug)
        ```

3. Run the operator program.

    The program starts to run until the first breakpoint (`add\_kernel.cpp:69`) is hit. msDebug detects that the NPU core function `add\_custom` starts to run on device 0.

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
    -> 69              outQueueZ.EnQue<int16_t>(zLocal);  # Breakpoint position
       70              // free input tensors for reuse
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
        - Print the data stored in LocalTensor in the UB memory.

            > [!NOTE]NOTE
            > For details about the start address for printing the UB memory, see the `bufferAddr` parameter in the `address\_` field of the LocalTensor variable. The following uses the `xLocal` variable as an example. The start address of the memory is `0`.

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

        - Print the data stored in GlobalTensor in the GM.

            > [!NOTE]NOTE
            > For details about the start address for GM memory printing, see the `address\_` field of the GlobalTensor variable. The following uses the `xGm` variable as an example. The start address of the memory is `0x00001240c0015000`.

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
        (msdebug) ascend aiv 24  // Select the core ID corresponding to block 7 in ascend info cores. In this example, the core ID is 24.
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

Obtain the [sample project](https://gitee.com/ascend/samples/tree/master/operator/ascendc/0_introduction/1_add_frameworklaunch/AddCustom).

> [!NOTE]NOTE
>
>- This sample project does not support <term>Atlas A3 training products</term>.
>- When downloading the code sample, run the following command to specify the branch version:
>
>    ```bash
>    git clone https://gitee.com/ascend/samples.git -b v0.2-8.0.0.beta1
>    ```

**Procedure**

1. Go to the directory where the msOpGen script **install.sh** is located.

    ```bash
    cd ${git_clone_path}/samples/operator/ascendc/0_introduction/1_add_frameworklaunch
    ```

2. Run the following command to generate a custom operator project and implement the operator on the host and kernel:

    ```bash
    bash install.sh -v Ascendxxxyy    # xxxyy indicates the type of the chip used by the user.
    ```

3. In the `$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/CustomOp` directory, change the value of `"Release"` to `"Debug"` in `cacheVariables` of the `CMakePresets.json` file.

    ```bash
    "cacheVariables": {
           "CMAKE_BUILD_TYPE": {
                "type": "STRING",
                "value": "Debug"
      },
    ...
    }
    ```

4. Compile and deploy the operator by referring to [Operator Compilation and Deployment](https://www.hiascend.com/document/detail/zh/mindstudio/82RC1/ODtools/Operatordevelopmenttools/atlasopdev_16_0024.html). <a id="step-4-operator-compilation"></a>
5. Go to the directory where the msOpGen script `install.sh` is located, compile the single-operator calling application by referring to [README](https://gitee.com/ascend/samples/blob/master/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation/README.md), and obtain the executable file `execute\_add\_op`. <a id="step-5"></a>

    ```bash
    cd ${git_clone_path}/samples/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation
    ```

6. Import the operator dynamic loading path.

    Import the path of the `.o` file output in the `build\_out` directory on the kernel after the custom operator project is compiled to environment variables.

    ```bash
    export LAUNCH_KERNEL_PATH=/{path_to_kernel}/kernel_name.o  # {path_to_kernel} indicates the path of the operator binary file *.o generated after the operator kernel is built. Replace it with the actual path.
    ```

    > [!NOTE]NOTE
    > Multiple `.o` files may be generated for multiple dtypes of the operator on the kernel. Import the `.o` file called in [Step 4](#step-4-operator-compilation).

7. Use msDebug to load the single-operator executable file `execute\_add\_op` obtained in [5](#step-5).

    ```bash
    export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/opp/vendors/customize/op_api/lib:$LD_LIBRARY_PATH
    cd AclNNInvocation/output
    msdebug execute_add_op
    (msdebug) target create "execute_add_op"
    Current executable set to '/home/AclNNInvocation/output/execute_add_op' (aarch64).
    (msdebug)
    ```

8. Set a breakpoint.

    ```bash
    b add_custom.cpp:55
    ```

9. Run the operator program and wait until the breakpoint is hit.

    ```bash
    (msdebug) r
    Process 1385976 launched: '$HOME/shelltest/test/samples/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocationNaive/build/execute_add_op' (aarch64)
    [Launch of Kernel anonymous on Device 0]
    Process 1385976 stopped
    [Switching to focus on Kernel anonymous, CoreId 24, Type aiv]
    * thread #1, name = 'execute_add_op', stop reason = breakpoint 1.1
        frame #0: 0x0000000000001564 AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o`KernelAdd::Compute(this=0x000000000028f8a8, progress=0) (.vector) at add_custom.cpp:55:19
       52           LocalTensor<DTYPE_Y> yLocal = inQueueY.DeQue<DTYPE_Y>();
       53           LocalTensor<DTYPE_Z> zLocal = outQueueZ.AllocTensor<DTYPE_Z>();
       54           Add(zLocal, xLocal, yLocal, this->tileLength);
    -> 55           outQueueZ.EnQue<DTYPE_Z>(zLocal);
       56           inQueueX.FreeTensor(xLocal);
       57           inQueueY.FreeTensor(yLocal);
       58       }
    (msdebug)
    ```

    > [!NOTE]NOTE
    > For details about the subsequent debugging process, see "[Importing Debugging Information](../user_guide/msdebug_user_guide.md#tool-usage)", "[Memory and Variable Printing](../user_guide/msdebug_user_guide.md#memory-and-variable-printing)", and "[Core Switching](../user_guide/msdebug_user_guide.md#core-switching)".

## Debugging the Operators Called by a PyTorch Interface

**Overview**

This section shows how to use msDebug to debug the add operator called by a PyTorch interface on the board. The add operator can add two vectors and output the result.

**Preparations**

- Obtain the [sample project](https://gitee.com/ascend/samples/tree/master/operator/ascendc/0_introduction/1_add_frameworklaunch/AddCustom).

    > [!NOTE]NOTE
    >
    > - This sample project supports only Python 3.9. If you want to run the project in other Python versions, change the Python version in the `run\_op\_plugin.sh` file in the `$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/PytorchInvocation` directory.
    > - This sample project does not support <term>Atlas A3 training products</term>.
    > - When downloading the code sample, run the following command to specify the branch version:
>
    >    ```bash
    >    git clone https://gitee.com/ascend/samples.git -b v0.2-8.0.0.beta1
    >    ```

- Install the PyTorch framework and torch_npu plugin by referring to [Ascend Extension for PyTorch Software Installation Guide](https://www.hiascend.com/document/detail/zh/Pytorch/720/configandinstg/instg/insg_0001.html).
- Configure related environment variables. For details, see [MindStudio Debugger User Guide](../user_guide/msdebug_user_guide.md).

**Procedure**

1. Run the following command to generate a custom operator project and implement the operator on the host and kernel:

    ```bash
    bash install.sh -v Ascendxxxyy    # xxxyy indicates the type of the chip used by the user.
    ```

2. In the `$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/CustomOp` directory, change the value of `"Release"` to "Debug" in `cacheVariables` of the `CMakePresets.json` file.

    ```bash
    "cacheVariables": {
           "CMAKE_BUILD_TYPE": {
               "type": "STRING",
               "value": "Debug"
           },
    ...
    }
    ```

3. Compile and deploy the operator by referring to [Operator Compilation and Deployment](https://www.hiascend.com/document/detail/zh/mindstudio/82RC1/ODtools/Operatordevelopmenttools/atlasopdev_16_0024.html).
4. Go to the sample directory and download the sample code in CLI mode. Use PyTorch to call the AddCustom operator project and complete the compilation by referring to [README](https://gitee.com/ascend/samples/blob/master/operator/ascendc/0_introduction/1_add_frameworklaunch/README.md).

    ```bash
    cd ${git_clone_path}/samples/operator/ascendc/0_introduction/1_add_frameworklaunch/PytorchInvocation
    ```

    > [!NOTE]NOTE
    > The sample project directory is as follows:
>
    > ```text
    > PytorchInvocation
    > ├── op_plugin_patch
    > ├── README.md        // Registration sample of calling the AddCustom operator project in PyTorch mode
    > ├── run_op_plugin.sh      // Required for executing the sample
    > └── test_ops_custom.py    // Required for starting the tool
    > └── test_ops_custom_register_in_graph.py  // Executes the test case script in torch.compile mode.
    > ```

5. Execute the sample. During the sample execution, test data is automatically generated. Run the PyTorch sample, and verify the running result.

    ```bash
    bash run_op_plugin.sh
    -- CMAKE_CCE_COMPILER: ${INSTALL_DIR}/toolkit/tools/ccec_compiler/bin/ccec
    -- CMAKE_CURRENT_LIST_DIR: ${INSTALL_DIR}/AddKernelInvocation/cmake/Modules
    -- ASCEND_PRODUCT_TYPE:
      Ascendxxxyy
    -- ASCEND_CORE_TYPE:
      VectorCore
    -- ASCEND_INSTALL_PATH:
      /usr/local/Ascend/cann
    -- The CXX compiler identification is GNU 10.3.1
    -- Detecting CXX compiler ABI info
    -- Detecting CXX compiler ABI info - done
    -- Check for working CXX compiler: /usr/bin/c++ - skipped
    -- Detecting CXX compile features
    -- Detecting CXX compile features - done
    -- Configuring done
    -- Generating done
    -- Build files have been written to: ${INSTALL_DIR}/AddKernelInvocation/build
    Scanning dependencies of target add_npu
    ...
    [100%] Built target add_npu
    INFO: Ascend C Add Custom SUCCESS
    ...
    INFO: Ascend C Add Custom  in torch.compile graph SUCCESS
    ```

6. Manually import the operator debugging information. The following is an example.

    > [!NOTE]NOTE
    >
    > - Replace `${INSTALL_DIR}` with the actual CANN installation directory. For example, if the installation is performed by the `root` user, the path is `/usr/local/Ascend/cann`.
    > - For servers other than the <term>Atlas A3 training products/Atlas A3 inference products</term>: Run the `npu-smi info` command on the server where the Ascend AI Processor is installed to obtain the chip name. Note that the actual value is represented by `AscendChip name`. For example, if the chip name is `xxxyy`, the actual value is `Ascendxxxyy`. If `Ascendxxxyy` is the path of the code sample, set this parameter to `ascendxxxyy`.
    > - For the <term>Atlas A3 training products/Atlas A3 inference products</term>, run the `npu-smi info -t board -i id -c chip_id` command on the server where the Ascend AI Processor is installed to obtain the chip name and NPU name. The actual value is represented by `Chip name_NPU name`. For example, if the chip name is `Ascendxxx` and the NPU name is `1234`, the actual value is `Ascendxxx_1234`. If `Ascendxxx_1234` is the path of the code sample, set this parameter to `ascendxxx_1234`.
    >    Note that:
    >    - `id`: device ID, which is the NPU ID obtained by running the `npu-smi info -l` command.
    >    - `chip\_id`: chip ID, which is the same as the chip ID obtained by running the `npu-smi info -m` command.

    ```bash
    export LAUNCH_KERNEL_PATH=${INSTALL_DIR}/opp/vendors/customize/op_impl/ai_core/tbe/kernel/SOC_VERSION/add_custom/AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o
    ```

7. Start msDebug to start the Python program and enter the debugging page.

    ```bash
    msdebug python3 test_ops_custom.py
    (msdebug) target create "python3"
    Current executable set to '/home/mindstudio/miniconda3/envs/py39/bin/python3' (aarch64).
    (msdebug) settings set -- target.run-args  "test_ops_custom.py"
    (msdebug)
    ```

8. Set a breakpoint.

    Set an NPU breakpoint in the kernel function based on the specified source code file and corresponding line number.

    ```bash
    (msdebug) b add_custom.cpp:60
    Breakpoint 1: where = AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o`::AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b_1(uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *) + 9912 [inlined] KernelAdd::Compute(int) + 3400 at add_custom.cpp:60:9, address = 0x00000000000026b8
    ```

9. Run the program and wait until the breakpoint is hit.

    ```bash
    (msdebug) r
    Process 197189 launched: '/home/miniconda3/envs/py39/bin/python3' (aarch64)
    Process 197189 stopped and restarted: thread 1 received signal: SIGCHLD
    ...
    [Launch of Kernel anonymous on Device 0]
    Process 197189 stopped
    [Switching to focus on Kernel anonymous, CoreId 8, Type aiv]
    * thread #1, name = 'python3', stop reason = breakpoint 2.1
        frame #0: 0x00000000000026b8 AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o`::AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b_1(uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *) [inlined] KernelAdd::Compute(this=0x000000000020efb8, progress=1) at add_custom.cpp:60:9
       57              LocalTensor<DTYPE_Y> yLocal = inQueueY.DeQue<DTYPE_Y>();
       58              LocalTensor<DTYPE_Z> zLocal = outQueueZ.AllocTensor<DTYPE_Z>();
       59              Add(zLocal, xLocal, yLocal, this->tileLength);
    -> 60              outQueueZ.EnQue<DTYPE_Z>(zLocal);
       61              inQueueX.FreeTensor(xLocal);
       62              inQueueY.FreeTensor(yLocal);
       63          }
    (msdebug)
    ```

    > [!NOTE]NOTE
    > For details about other debugging operations, see "[Importing Debugging Information](../user_guide/msdebug_user_guide.md#tool-usage)", "[Memory and Variable Printing](../user_guide/msdebug_user_guide.md#memory-and-variable-printing)", "[Debugging Information Display](../user_guide/msdebug_user_guide.md#debugging-information-display)", and "[Core Switching](../user_guide/msdebug_user_guide.md#core-switching)".

10. Delete the breakpoint. For details, see "[Setting Breakpoints] (../user_guide/msdebug_user_guide.md#setting-breakpoints)".
11. After the debugging is complete, run the `q` command and enter `Y` or `y` to end the debugging.

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```

## Debugging a Template Library Operator on the Board

**Overview**

This section shows how to use msDebug to debug a template library operator (matmul) on the board. The operator can multiply two matrices and output the result.

**Preparations** <a id="Preparations"></a>

- Obtain the [sample project](https://gitcode.com/cann/catlass).
- Configure related environment variables. For details, see [MindStudio Debugger User Guide](../user_guide/msdebug_user_guide.md).

**Procedure**

1. Compile the operator based on the sample project obtained in "Preparations" to generate the executable file `00\_basic\_matmul`.

    Run the following command to compile the operator. After the compilation is complete, the executable file `00\_basic\_matmul` is generated in the `build/bin` directory.

    ```bash
    bash ./scripts/build.sh 00_basic_matmul --debug --msdebug
    ```

2. Start msDebug to start the operator program and enter the debugging page.

    ```bash
    msdebug ./build/bin/00_basic_matmul 256 512 1024 0
    (msdebug) target create "./build/bin/00_basic_matmul"
    Current executable set to '/home/mindstudio/projects/ascendc-templates/build/bin/00_basic_matmul' (aarch64).
    (msdebug)
    ```

3. Sets a breakpoint.

    In this test case, the code implementation of the kernel function is located in `basic\_matmul.hpp`. In this file, set an NPU breakpoint for the required code line.

    ```bash
    (msdebug) b basic_matmul.hpp:121
    Breakpoint 1: 2 locations.
    (msdebug)
    ```

4. Run the operator program and wait until the breakpoint is hit.

    The program starts to run until the first breakpoint (`basic\_matmul.hpp:127`) is hit. msDebug detects that the NPU core function starts to run on device 0.

    ```cpp
    (msdebug) run
    Process 3344307 launched: '/home/mindstudio/projects/ascendc-templates/build/bin/00_basic_matmul' (aarch64)
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

    > [!NOTE]NOTE
    > `\_ZN7Catlass13KernelAdapterINS\_4Gemm6Kernel11BasicMatmulINS1\_5Blo` indicates the kernel name of the template library. Only the first 64 characters are displayed in the example.

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
        (msdebug) ascend aic 24  // Select the core ID corresponding to block 3 in ascend info cores. In this example, the core ID is 24.
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

    > [!NOTE]NOTE
    > For details about other debugging operations, see "[Memory and Variable Printing](../user_guide/msdebug_user_guide.md#memory-and-variable-printing)", "[Debugging Information Display](../user_guide/msdebug_user_guide.md#debugging-information-displaying)", and "[Core Switching](../user_guide/msdebug_user_guide.md#core-switching)".

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
