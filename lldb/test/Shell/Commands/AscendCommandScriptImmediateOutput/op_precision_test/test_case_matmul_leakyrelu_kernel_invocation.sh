#!/bin/bash

# for Matmul Leakyrelu ACLRT_LAUNCH_KERNEL

echo 'start running '$(basename "$0")
source $HOME/Ascend_Daily/ascend-toolkit/set_env.sh

PREVIOUS_DIR=$(pwd)
cd $HOME/samples/operator/MatMulLeakyReluCustomSample/KernelLaunch/MatMulLeakyReluInvocation

# clean temp files
TEMP_DIR="_debug_temp_files"
if [ -d "$TEMP_DIR" ]; then
    echo "$TEMP_DIR exists. remove it now"
    rm -rf $TEMP_DIR
fi

mkdir $TEMP_DIR

# 修改-O0
# 备份原始cmake文件
cp cmake/npu_lib.cmake $TEMP_DIR/npu_lib.cmake.old

echo "
ascendc_compile_options(ascendc_kernels_\${RUN_MODE} PRIVATE
  -g
  -O0
  --cce-ignore-always-inline=true
)" >> cmake/npu_lib.cmake

# 检查O0是否替换成功
if grep -Fq "O0" cmake/npu_lib.cmake
then
    echo "replacing compile options O0 done"
else
    echo "replacing compile options O0 failed. Exiting"
    # 恢复原始cmake文件
    cp $TEMP_DIR/npu_lib.cmake.old cmake/npu_lib.cmake
    exit -1
fi

# 检查精度是否正常
echo "####### start run.sh #######"
bash run.sh -r npu -v Ascend910B1 > $TEMP_DIR/temp_run.txt
if [ $? -eq 0 ];then
    echo "operator running done"
else
    echo "run.sh failed. Exiting"
    # 恢复原始cmake文件
    cp $TEMP_DIR/npu_lib.cmake.old cmake/npu_lib.cmake
    exit -2
fi

if grep -Fq "test pass" $TEMP_DIR/temp_run.txt
then
    echo "####### operator precision ok #######"
else
    echo "operator precision error"
    # 恢复原始cmake文件
    cp $TEMP_DIR/npu_lib.cmake.old cmake/npu_lib.cmake
    exit -3
fi

# 确认算子二进制包含debug_info段
# extract .o
llvm-objcopy -j .ascend.kernel.ascend910b1.ascendc_kernels_npu -O binary ./out/lib*/libascendc_kernels_npu.so $TEMP_DIR/section.o
dd if=$TEMP_DIR/section.o of=$TEMP_DIR/temp_kernel.o bs=1 skip=20

# check include debug_info
llvm-objdump -h $TEMP_DIR/temp_kernel.o > $TEMP_DIR/temp_objdump.txt

if grep -Fq ".debug_info" $TEMP_DIR/temp_objdump.txt
then
    echo ".debug_info exists"
else
    echo ".debug_info does not exist in operator object file"
    # 恢复原始cmake文件
    cp $TEMP_DIR/npu_lib.cmake.old cmake/npu_lib.cmake
    exit -4
fi

# 恢复原始cmake文件
cp $TEMP_DIR/npu_lib.cmake.old cmake/npu_lib.cmake

rm -rf $TEMP_DIR

echo "################ This test case PASSED ################ "

cd $PREVIOUS_DIR
exit 0
