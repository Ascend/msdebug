# 本脚本用于生成 msdebug/lldb 的断点验证shell test用例文件

######### 用例参数 #########
# 1. [cann_env]  cann环境变量脚本路径
# 2. [target]    被运行的二进制名字
# 3. [exec_path] 二进制运行目录路径
# 4. [file_name] 设置断点的代码文件名
# 5. [lines] 设置断点的行号顺序集合
# 6. [output_file] 输出用例文件名

test_case_params_add_kernel_invocation = {
    'cann_env': '~/Ascend_RC2_alpha002/ascend-toolkit/set_env.sh',
    'target': 'add_npu',
    'run_env': '',
    'exec_path': '~/samples/operator/AddCustomSample/KernelLaunch/AddKernelInvocation/',
    'file_name': 'add_custom.cpp',
    'lines': [77, 19, 78, 22, 23, 24, 25, 26, 27, 28, 79, 31, 33, 42, 43, 44, 45, 46, 47,\
        48, 34, 51, 52, 53, 54, 55, 56, 57, 58, 35, 61, 62, 63, 64],
    'output_file': 'AddKernelInvocation/command-ascend-breakpoint.test',
    'is_dwarf_in_dylib': False
}

test_case_params_add_kernel_invocation_neo = {
    'cann_env': '~/Ascend_RC2_alpha002/ascend-toolkit/set_env.sh',
    'target': 'ascendc_kernels_bbit',
    'run_env': 'LD_LIBRARY_PATH=$(pwd)/out/lib:$LD_LIBRARY_PATH ',
    'exec_path': '~/samples/operator/AddCustomSample/KernelLaunch/AddKernelInvocationNeo/',
    'file_name': 'add_custom.cpp',
    'lines': [77, 19, 78, 22, 23, 24, 25, 26, 27, 28, 79, 31, 32, 33, 42, 43, 44, 45, 46, 47,\
        48, 34, 51, 52, 53, 54, 55, 56, 57, 58, 35, 61, 62, 63, 64],
    'output_file': 'AddKernelInvocationNeo/command-ascend-breakpoint.test',
    'is_dwarf_in_dylib': True
}

test_case_params_matmul_invocation_neo = {
    'cann_env': '~/Ascend_RC2_alpha002/ascend-toolkit/set_env.sh',
    'target': 'ascendc_kernels_bbit',
    'run_env': 'LD_LIBRARY_PATH=$(pwd)/out/lib:$LD_LIBRARY_PATH ',
    'exec_path': '~/samples/operator/MatMulCustomSample/KernelLaunch/MatMulInvocationNeo/',
    'file_name': 'matmul_custom.cpp',
    'lines': [54, 55, 56, 58, 59, 60, 61, 63, 65, 68, 69, 70, 71, 72, 73, 75, 13, 14, 15, 17,\
        18, 19, 23, 24, 27, 28, 31, 32, 33, 77, 78, 79, 80, 85, 93, 94, 95, 96, 97],
    'output_file': 'MatMulInvocationNeo/command-ascend-breakpoint.test',
    'is_dwarf_in_dylib': True
}

test_case_params_matmul_leakyrelu_invocation_neo = {
    'cann_env': '~/Ascend_RC2_alpha002/ascend-toolkit/set_env.sh',
    'target': 'ascendc_kernels_bbit',
    'run_env': 'LD_LIBRARY_PATH=$(pwd)/out/lib:$LD_LIBRARY_PATH ',
    'exec_path': '~/samples/operator/MatMulLeakyReluCustomSample/KernelLaunch/MatMulLeakyReluInvocation/',
    'file_name': 'matmul_leakyrelu_custom.cpp',
    'lines': [125, 11, 126, 127, 36, 37, 42, 43, 44, 45, 47, 48, 114, 115, 116, 118, 119, 120, 121, 122,\
        49, 50, 51, 52, 53, 54, 55, 128, 129, 63, 72, 73, 74, 75, 77, 88, 89, 78, 94, 95, 79, 101, 102,\
        103, 104, 105, 108, 109, 110, 80, 83, 84, 130],
    'output_file': 'MatMulLeakyReluInvocation/command-ascend-breakpoint.test',
    'is_dwarf_in_dylib': True
}

###############################
######### 选择生成用例 #########

test_case_params = test_case_params_matmul_leakyrelu_invocation_neo

###############################
###############################

######### 用例模板定义 #########

set_br_and_run_template = 'b {2}:{0}\n' + \
                          '# CHECK-LABEL: b {2}:{0}\n' + \
                          "# CHECK-NEXT: Breakpoint {{{{[0-9]+}}}}:\n" + \
                          "\nrun\n" + \
                          "# CHECK-LABEL: run\n" + \
                          "# CHECK: [Launch of Kernel {{{{[0-9a-zA-Z_]+}}}} on Device {{{{[0-9]+}}}}]\n" + \
                          "# CHECK-NEXT: Process {{{{[0-9]+}}}} stopped\n" + \
                          "# CHECK-NEXT: [Switching to focus on Kernel {{{{[0-9a-zA-Z_]+}}}}, CoreId {{{{[0-9]+}}}}, Type {{{{[aivc]+}}}}]\n" + \
                          "# CHECK-NEXT: * thread #1, name = '{1}', stop reason = breakpoint 1.{{{{[1-9]}}}}\n" + \
                          "# CHECK-NEXT:    frame #0: 0x{{{{[0-9a-f]+}}}} {{{{[^(]+\(.*\)}}}} at {2}:{0}:{{{{[0-9]+}}}}\n"


set_br_and_continue_template  = "\nb {0}\n" + \
                                "# CHECK-LABEL: b {0}\n" + \
                                "# CHECK-NOT: no locations\n" + \
                                "# CHECK-NEXT: Breakpoint {{{{[0-9]+}}}}:\n" + \
                                "\ncontinue\n" + \
                                "# CHECK-LABEL: continue\n" + \
                                "# CHECK-NEXT: Process {{{{[0-9]+}}}} resuming\n" + \
                                "# CHECK-NEXT: Process {{{{[0-9]+}}}} stopped\n" + \
                                "# CHECK-NEXT: [Switching to focus on Kernel {{{{[0-9a-zA-Z_]+}}}}, CoreId {{{{[0-9]+}}}}, Type {{{{[aivc]+}}}}]\n" + \
                                "# CHECK-NEXT: * thread #1, name = '{1}', stop reason = breakpoint {{{{[0-9]+}}}}.{{{{[0-9]+}}}}\n" + \
                                "# CHECK-NEXT:    frame #0: 0x{{{{[0-9a-f]+}}}} {{{{[^(]+\(.*\)}}}} at {2}:{0}:{{{{[0-9]+}}}}\n" + \
                                "br delete\n" + \
                                "y\n"

# for dylib

set_br_and_run_dylib_template = 'b {2}:{0}\n' + \
                                '# CHECK-LABEL: b {2}:{0}\n' + \
                                "# CHECK-NEXT: Breakpoint {{{{[0-9]+}}}}:\n" + \
                                "\nrun\n" + \
                                "# CHECK-LABEL: run\n" + \
                                "# CHECK: [Launch of Kernel {{{{[0-9a-zA-Z_]+}}}} on Device {{{{[0-9]+}}}}]\n" + \
                                "# CHECK-NEXT: Process {{{{[0-9]+}}}} stopped\n" + \
                                "# CHECK-NEXT: [Switching to focus on Kernel {{{{[0-9a-zA-Z_]+}}}}, CoreId {{{{[0-9]+}}}}, Type {{{{[aivc]+}}}}]\n" + \
                                "# CHECK-NEXT: * thread #1, name = '{1}', stop reason = breakpoint 1.{{{{[1-9]}}}}\n" + \
                                "# CHECK-NEXT:    frame #0: 0x{{{{[0-9a-f]+}}}} {{{{[^(]+\(.*\)}}}} at {2}:{0}:{{{{[0-9]+}}}}\n"


######### 设置用例参数 #########

cann_env = test_case_params['cann_env']
target = test_case_params['target']
run_env = test_case_params['run_env']
exec_path = test_case_params['exec_path']
file_name = test_case_params['file_name']
lines = test_case_params['lines']
output_file = test_case_params['output_file']
is_dwarf_in_dylib = test_case_params['is_dwarf_in_dylib']

exec_cmds = ["cd " + exec_path,
            run_env + "msdebug " + target]

######### 开始构造用例 #########

test_suite_str  = "# REQUIRES: ascend-debugger\n"
test_suite_str += "# RUN: source " + cann_env + "\n"

for cmd in exec_cmds:
    test_suite_str += "# RUN: " + cmd + "\n"

test_suite_str = test_suite_str[:-1]
test_suite_str += " -s %s | FileCheck %s -dump-input=always" + "\n\n"

is_first = True
for line in lines:
    if (is_first):
        if (is_dwarf_in_dylib):
            test_suite_str += set_br_and_run_dylib_template.format(line, 'ascendc_kernels', file_name)
        else:
            test_suite_str += set_br_and_run_template.format(line, target, file_name)
        is_first = False
        continue
    if (is_dwarf_in_dylib):
        test_suite_str += set_br_and_continue_template.format(line, 'ascendc_kernels', file_name)
    else:
        test_suite_str += set_br_and_continue_template.format(line, target, file_name)
    test_suite_str += "\n"

# 删除所有断点后continue至结束
test_suite_str += "\nc\n" + \
                  "# CHECK: Process {{[0-9]+}} resuming\n" + \
                  "# CHECK: Process {{[0-9]+}} exited\n" + \
                  "\nquit\n"

with open(output_file,"w") as file:
    file.write(test_suite_str)

