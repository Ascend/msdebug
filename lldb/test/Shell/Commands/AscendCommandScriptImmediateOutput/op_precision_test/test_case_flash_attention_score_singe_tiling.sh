#!/bin/bash

# for flash_attention_score singe_tiling aclnn

echo 'start running '$(basename "$0")
source $HOME/Ascend_Daily/ascend-toolkit/set_env.sh

PREVIOUS_DIR=$(pwd)
OPSADV_REPO_PATH=''
PY_FILE_BACKUP_PATH=''
PY_FILE_PATH=''

# check cann env
Check_CANN_Env() {
    if [[ -z "${ASCEND_TOOLKIT_HOME}" ]]; then
        echo "======= ERROR: ASCEND_TOOLKIT_HOME is null. Please source cann bash script before running test cases"
        return 1
    else
        echo "CANN ENV checked"
    fi

    if ! [ -x "$(command -v ascendebug)" ]; then
        echo "try add hisi cann env"
        # 若找不到ascendebug工具，则source 分包环境变量
        source ~/Ascend_Daily/ascend-toolkit/latest/bin/setenv.bash

        if ! [ -x "$(command -v ascendebug)" ]; then
            echo "ascendebug could not be found"
            return 1
        fi
    fi

    return 0
}

Get_Opsadv_Dir() {
    lines=$(find $PREVIOUS_DIR/ -name ops_adv | wc -l)

    if [ $lines -eq 0 ];then
        lines=$(find $PREVIOUS_DIR/ -name ops_adv-br_hisi_trunk_ai | wc -l)
        if [ $lines -eq 0 ];then
            echo "cannot find ops_adv or ops_adv-br_hisi_trunk_ai directory"
            return 2
        else
            OPSADV_REPO_PATH=$(find $PREVIOUS_DIR/ -name ops_adv-br_hisi_trunk_ai)
            echo "OPSADV_REPO_PATH is "$OPSADV_REPO_PATH
        fi
    else
        OPSADV_REPO_PATH=$(find $PREVIOUS_DIR/ -name ops_adv)
        echo "OPSADV_REPO_PATH is "$OPSADV_REPO_PATH
    fi
    return 0
}

Modify_Compile_option() {
    # 修改 flash_attention_score.py 增加编译选项 -O0 -g --cce-ignore-always-inline=true
    PY_FILE_PATH=$(find $ASCEND_TOOLKIT_HOME/.. -name flash_attention_score.py)
    if [[ -e $PY_FILE_PATH ]]; then
        echo "found PY_FILE_PATH in "$PY_FILE_PATH
    else
        echo $PY_FILE_PATH " is not found"
        return 3
    fi

    # 备份
    PY_FILE_BACKUP_PATH='./flash_attention_score.py'
    cp $PY_FILE_PATH $PY_FILE_BACKUP_PATH

    # 检查O0是否已经替换成功
    if grep -Fq "O0" $PY_FILE_PATH
    then
        echo "replacing compile options O0 done"
        return 0
    else
        echo "need to modify compile option in "$PY_FILE_PATH
    fi

    chmod +w $PY_FILE_PATH
    chmod +w $(dirname $PY_FILE_PATH)

    COMPILE_OPTION_LINE_NUM=$(grep -Fn 'custom_compile_options ='  $PY_FILE_PATH | cut -d':' -f1)
    COMPILE_OPTION=$(awk 'NR=='$COMPILE_OPTION_LINE_NUM $PY_FILE_PATH)
    COMPILE_OPTION_truncated=${COMPILE_OPTION::-3}
    COMPILE_OPTION_O0=$COMPILE_OPTION_truncated', '\''-g'\'', '\''-O0'\'', '\''--cce-ignore-always-inline=true'\'']},'
    sed -i "${COMPILE_OPTION_LINE_NUM} c\\${COMPILE_OPTION_O0}" $PY_FILE_PATH

    # 检查O0是否替换成功
    if grep -Fq "O0" $PY_FILE_PATH
    then
        echo "replacing compile options O0 done"
    else
        echo "replacing compile options O0 failed. Exiting"
        cp $PY_FILE_BACKUP_PATH $PY_FILE_PATH
        return 4
    fi

    return 0
}

Run_FA() {
    JSON_FILE_PATH=$(find $ASCEND_TOOLKIT_HOME/.. -name flash_attention_score_demo.json)
    if [ -e $JSON_FILE_PATH ]; then
        echo "found JSON_FILE_PATH in "$JSON_FILE_PATH
    else
        return 5
    fi

    echo "####### start ascendebug #######"
    ascendebug kernel --backend npu --json-file $JSON_FILE_PATH --chip-version Ascend910B1  --core-type MixCore --repo-type ops_adv --repo-path $OPSADV_REPO_PATH > ./run_ascendebug.log

    RESULT_FILE='debug_workspace/FlashAttentionScore/npu/output/attention_out.txt'

    echo $(pwd)

    if grep -Fq "Success Success Success Success Success" $RESULT_FILE
    then
        echo "####### operator precision ok #######"
    else
        echo "operator precision error"
        return 6
    fi


    # 确认算子二进制包含debug_info段
    OP_OBJ_FILE='debug_workspace/FlashAttentionScore/npu/build/gen_FlashAttentionScore.o'

    # check include debug_info
    llvm-objdump -h $OP_OBJ_FILE > temp_objdump.txt

    if grep -Fq ".debug_info" temp_objdump.txt
    then
        echo ".debug_info exists"
    else
        echo ".debug_info does not exist in operator object file"
        return 7
    fi

    return 0
}

echo "test_case_flash_attention_score_singe_tiling.sh started"

Check_CANN_Env

RET=$?
if [ $RET -eq 0 ];then
    echo "Check_CANN_Env done"
else
    echo "Check_CANN_Env failed"
    exit $RET
fi

# clean temp files
TEMP_DIR="_debug_temp_files_fa"
if [ -d "$TEMP_DIR" ]; then
    echo "$TEMP_DIR exists. remove it now"
    rm -rf $TEMP_DIR
fi

mkdir $TEMP_DIR
cd $TEMP_DIR

Get_Opsadv_Dir

RET=$?
if [ $RET -eq 0 ];then
    echo "Get_Opsadv_Dir done"
else
    echo "Get_Opsadv_Dir failed"
    exit $RET
fi

Modify_Compile_option

RET=$?
if [ $RET -eq 0 ];then
    echo "Modify_Compile_option done"
else
    echo "Modify_Compile_option failed. ret="$RET
    exit $RET
fi

Run_FA

RET=$?
if [ $RET -eq 0 ];then
    echo "Run_FA done"
else
    echo "Run_FA failed"
    # cp $PY_FILE_BACKUP_PATH $PY_FILE_PATH
    exit $RET
fi

# 恢复原始py文件
cp $PY_FILE_BACKUP_PATH $PY_FILE_PATH

# rm -rf $TEMP_DIR

echo "################ This test case PASSED ################ "

cd $PREVIOUS_DIR
exit 0
