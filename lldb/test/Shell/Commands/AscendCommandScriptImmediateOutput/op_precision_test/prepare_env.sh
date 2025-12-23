#!/bin/bash

source $HOME/Ascend_Daily/ascend-toolkit/set_env.sh
# check cann env
if [[ -z "$ASCEND_TOOLKIT_HOME" ]]; then
    echo "======= ERROR: ASCEND_TOOLKIT_HOME is null. Please source cann bash script before running test cases"
    exit 1
else
    echo "==== CANN ENV: ASCEND_TOOLKIT_HOME="$ASCEND_TOOLKIT_HOME
fi

# 下载samples代码仓
if [ -d "samples" ]; then
    echo "git clone samples done"
else
    git clone https://gitee.com/ascend/samples.git --depth 5
    if [ $? -eq 0 ];then
        echo "git clone samples done"
    else
        echo "======= ERROR: git clone samples failed"
        exit 2
    fi
fi

# 下载ops_adv for flash_attention_score
if [ -d "ops_adv" ]||[ -d "ops_adv-br_hisi_trunk_ai" ]||[ -d "./hisi/asl/ops/ops_adv" ]||[ -d "./hisi/asl/ops/ops_adv-br_hisi_trunk_ai" ]; then
    echo "ops_adv repo is already downloaded"
else
    git clone LS_URL_REPLACE -b br_hisi_trunk_ai --depth 5
    if [ $? -eq 0 ];then
        echo "git clone ops_adv done"
    else
        echo "======= ERROR: git clone ops_adv failed"
        exit 3
    fi
fi

exit 0
