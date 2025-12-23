#!/bin/bash

# for Add ACLNN

source $HOME/Ascend_Daily/ascend-toolkit/set_env.sh
# check cann env
Check_CANN_Env() {
    if [[ -z "${ASCEND_TOOLKIT_HOME}" ]]; then
        echo "======= ERROR: ASCEND_TOOLKIT_HOME is null. Please source cann bash script before running test cases"
        return 1
    else
        echo "CANN ENV checked"
    fi
}


Compile_Add_Framework() {
    PREVIOUS_DIR=$(pwd)
    cd $HOME/samples/operator/AddCustomSample/FrameworkLaunch/AddCustom

    # clean temp files
    TEMP_DIR="_debug_temp_files"
    if [ -d "$TEMP_DIR" ]; then
        echo "$TEMP_DIR exists. remove it now"
        rm -rf $TEMP_DIR
    fi

    mkdir $TEMP_DIR

    CURRENT_ARCH=$(uname -m)

    # todo
    PACKEAGE_NAME="custom_opp_ubuntu_"$CURRENT_ARCH".run"
    echo $PACKEAGE_NAME

    # 清理已存在的run包
    if [ -f "build_out/"$PACKEAGE_NAME ]; then
        echo "removing previous operator package"
        rm "build_out/"$PACKEAGE_NAME
    fi

    if [ -f "./build_out/op_kernel/binary/ascend910b/bin/add_custom/AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o" ]; then
        echo "removing previous kernel object"
        rm "./build_out/op_kernel/binary/ascend910b/bin/add_custom/AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o"
    fi

    # 修改-O0
    # 备份原始cmake文件
    cp CMakePresets.json $TEMP_DIR/CMakePresets.json.old
    cp op_kernel/CMakeLists.txt $TEMP_DIR/CMakeLists.txt.old

    sed -i 's/add_ops_compile_options(ALL OPTIONS -g -O0)/add_ops_compile_options(ALL OPTIONS -g -O0 --cce-ignore-always-inline=true)/g' op_kernel/CMakeLists.txt
    sed -i 's/Release/Debug/g' CMakePresets.json

    ASCEND_COMPUTE_UNIT_LINE_NUM=$(grep -Fn ASCEND_COMPUTE_UNIT CMakePresets.json | cut -d':' -f1)
    ASCEND_COMPUTE_UNIT_REPLACE_LINE_NUM=$(echo $ASCEND_COMPUTE_UNIT_LINE_NUM | awk '{print $1 + 2}')
    sed -i $ASCEND_COMPUTE_UNIT_REPLACE_LINE_NUM'c\                    \"value\": \"ascend910b\"' CMakePresets.json

    ASCEND_CANN_PACKAGE_PATH_LINE_NUM=$(grep -Fn ASCEND_CANN_PACKAGE_PATH CMakePresets.json | cut -d':' -f1)
    ASCEND_CANN_PACKAGE_PATH_REPLACE_LINE_NUM=$(echo $ASCEND_CANN_PACKAGE_PATH_LINE_NUM | awk '{print $1 + 2}')
    sed -i $ASCEND_CANN_PACKAGE_PATH_REPLACE_LINE_NUM'c\                    \"value\": \"'$ASCEND_TOOLKIT_HOME'\"' CMakePresets.json

    # 检查编译选项是否替换成功
    if grep -Fq "ignore-always-inline" op_kernel/CMakeLists.txt
    then
        echo "replacing compile options done"
    else
        echo "replacing compile options failed. Exiting"
        # 恢复原始cmake文件
        cp $TEMP_DIR/CMakePresets.json.old CMakePresets.json
        cp $TEMP_DIR/CMakeLists.txt.old op_kernel/CMakeLists.txt
        return 1
    fi

    # 启动编译
    echo "####### start build.sh #######"
    bash build.sh > $TEMP_DIR/temp_run.txt
    if [ $? -eq 0 ];then
        echo "operator running done"
    else
        echo "run.sh failed. Exiting"
        # 恢复原始cmake文件
        cp $TEMP_DIR/CMakePresets.json.old CMakePresets.json
        cp $TEMP_DIR/CMakeLists.txt.old op_kernel/CMakeLists.txt
        return 2
    fi

    # 检查run包是否生成
    if [ -f "build_out/"$PACKEAGE_NAME ]; then
        echo "run package "$PACKEAGE_NAME" generated"
    else
        echo "run package not generated. exiting"
        # 恢复原始cmake文件
        cp $TEMP_DIR/CMakePresets.json.old CMakePresets.json
        cp $TEMP_DIR/CMakeLists.txt.old op_kernel/CMakeLists.txt
        return 3
    fi

    # 检查编译产物
    # 确认算子二进制包含debug_info段

    llvm-objdump -h ./build_out/op_kernel/binary/ascend910b/bin/add_custom/AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b.o > $TEMP_DIR/temp_objdump.txt

    if grep -Fq ".debug_info" $TEMP_DIR/temp_objdump.txt
    then
        echo ".debug_info exists"
    else
        echo ".debug_info does not exist in operator object file"
        # 恢复原始cmake文件
        cp $TEMP_DIR/CMakePresets.json.old CMakePresets.json
        cp $TEMP_DIR/CMakeLists.txt.old op_kernel/CMakeLists.txt
        return 4
    fi

    # 恢复原始cmake文件
    cp $TEMP_DIR/CMakePresets.json.old CMakePresets.json
    cp $TEMP_DIR/CMakeLists.txt.old op_kernel/CMakeLists.txt

    rm -rf $TEMP_DIR

    echo "################ This test case PASSED ################ "

    ./"build_out/"$PACKEAGE_NAME

    echo "####### Kernel has been installed in cann #######"

    cd $PREVIOUS_DIR
    return 0

}

Run_Add_ACLNN() {
    PREVIOUS_DIR=$(pwd)
    cd $HOME/samples/operator/AddCustomSample/FrameworkLaunch/AclNNInvocationNaive

    # clean temp files
    TEMP_DIR="_debug_temp_files"
    if [ -d "$TEMP_DIR" ]; then
        echo "$TEMP_DIR exists. remove it now"
        rm -rf $TEMP_DIR
    fi

    mkdir $TEMP_DIR

    export NPU_HOST_LIB=$ASCEND_TOOLKIT_HOME"/lib64"

    # 修改CMakeList
    # 备份原始cmake文件
    cp CMakeLists.txt $TEMP_DIR/CMakeLists.txt.old

    sed -i 's|/usr/local/Ascend/ascend-toolkit/latest|'$ASCEND_TOOLKIT_HOME'|g' CMakeLists.txt

    # 检查编译选项是否替换成功
    if grep -Fq $ASCEND_TOOLKIT_HOME CMakeLists.txt
    then
        echo "replacing cann path done"
    else
        echo "replacing cann path failed. Exiting"
        # 恢复原始cmake文件
        cp $TEMP_DIR/CMakeLists.txt.old CMakeLists.txt
        return 1
    fi

    # 启动编译
    echo "####### compiling #######"
    if [ -d "build" ];then
        echo "build exists. removing"
        rm -rf ./build
    fi
    mkdir build

    cd build
    cmake ..;make

    if [ $? -eq 0 ];then
        echo "make done"
    else
        echo "make failed. Exiting"
        # 恢复原始cmake文件
        cd ..
        cp $TEMP_DIR/CMakeLists.txt.old CMakeLists.txt
        return 2
    fi

    # 检查二进制是否生成
    if [ -f "execute_add_op" ]; then
        echo "execute_add_op generated"
    else
        echo "execute_add_op not generated. exiting"
        # 恢复原始cmake文件
        cd ..
        cp $TEMP_DIR/CMakeLists.txt.old CMakeLists.txt
        return 3
    fi

    # 检查运行结果
    ./execute_add_op > ../$TEMP_DIR/temp_exec.txt

    cd ..
    if grep -Fq "result[0] is: 3.000000" $TEMP_DIR/temp_exec.txt
    then
        echo "results ok"
    else
        echo "results not ok"
        # 恢复原始cmake文件
        cp $TEMP_DIR/CMakeLists.txt.old CMakeLists.txt
        return 4
    fi

    # 恢复原始cmake文件
    cp $TEMP_DIR/CMakeLists.txt.old CMakeLists.txt

    rm -rf $TEMP_DIR

    echo "################ This test case PASSED ################ "

    cd $PREVIOUS_DIR
    return 0

}

echo 'start running '$(basename "$0")

Check_CANN_Env

RET=$?
if [ $RET -eq 0 ];then
    echo "Check_CANN_Env done"
else
    echo "Check_CANN_Env failed"
    exit $RET
fi

Compile_Add_Framework

RET=$?
if [ $RET -eq 0 ];then
    echo "Compile_Add_Framework done"
else
    echo "Compile_Add_Framework failed"
    exit $RET
fi

Run_Add_ACLNN

RET=$?
if [ $RET -eq 0 ];then
    echo "Run_Add_ACLNN done"
else
    echo "Run_Add_ACLNN failed. ret="$RET
    exit $RET
fi
