#!/bin/bash

# for Add <<<>>>

echo 'start running '$(basename "$0")
source $HOME/Ascend_Daily/ascend-toolkit/set_env.sh

PREVIOUS_DIR=$(pwd)
cd $HOME/samples/operator/AddCustomSample/KernelLaunch/AddKernelInvocation

# clean temp files
TEMP_DIR="_debug_temp_files"
if [ -d "$TEMP_DIR" ]; then
    echo "$TEMP_DIR exists. remove it now"
    rm -rf $TEMP_DIR
fi

mkdir $TEMP_DIR

# 修改-O0
# 备份原始cmake文件
cp cmake/npu/CMakeLists.txt $TEMP_DIR/CMakeLists.txt.old
sed -i 's/-O2/-O0\n    -g\n    --cce-ignore-always-inline=true/g' cmake/npu/CMakeLists.txt
sed -i 's/-O3/-O0\n    -g\n    --cce-ignore-always-inline=true/g' cmake/npu/CMakeLists.txt

# 检查O0是否替换成功
if grep -Fq "O0" cmake/npu/CMakeLists.txt
then
    echo "replacing compile options O0 done"
else
    echo "replacing compile options O0 failed. Exiting"
    # 恢复原始cmake文件
    cp $TEMP_DIR/CMakeLists.txt.old cmake/npu/CMakeLists.txt
    exit -1
fi

# 检查精度是否正常
echo "####### start run.sh #######"
bash run.sh -r npu -v Ascend910B1 > $TEMP_DIR/temp_run.txt
if [ $? -eq 0 ];then
    echo "operator running done"
else
    echo "run.sh failed. Exiting"
    exit -2
fi

if grep -Fq "test pass" $TEMP_DIR/temp_run.txt
then
    echo "####### operator precision ok #######"
else
    echo "operator precision error"
    exit -3
fi

# 确认算子二进制包含debug_info段
# extract .o
llvm-objcopy -j .aicore_binary -O binary ./add_npu $TEMP_DIR/temp_kernel.o

# check include debug_info
llvm-objdump -h $TEMP_DIR/temp_kernel.o > $TEMP_DIR/temp_objdump.txt

if grep -Fq ".debug_info" $TEMP_DIR/temp_objdump.txt
then
    echo ".debug_info exists"
else
    echo ".debug_info does not exist in operator object file"
    exit -4
fi

# 恢复原始cmake文件
cp $TEMP_DIR/CMakeLists.txt.old cmake/npu/CMakeLists.txt

rm -rf $TEMP_DIR

echo "################ This test case PASSED ################ "

cd $PREVIOUS_DIR
exit 0
