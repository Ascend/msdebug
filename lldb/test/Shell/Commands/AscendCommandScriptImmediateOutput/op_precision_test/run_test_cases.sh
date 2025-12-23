#!/bin/bash

bash ./prepare_env.sh

if [ $? -eq 0 ];then
    echo "prepare_env done"
else
    echo "prepare_env failed"
    exit 1

fi

TEST_CASE_NUM=$(ls | find ./ -maxdepth 1 -name "test_case_*.sh" | wc -l)

echo "test case:"$TEST_CASE_NUM

files=$(ls | find ./ -maxdepth 1 -name "test_case_*.sh")

i=1
for item in ${files[*]}
do

    # run test case
    bash $item

    if [ $? -eq 0 ];then
        echo "============ " $i "/" $TEST_CASE_NUM " tests done ============"
        ((i++))
    else
        echo "test aborted."
        echo "============ running " $i "/" $TEST_CASE_NUM " tests faled ============"
        exit -1
    fi
done
