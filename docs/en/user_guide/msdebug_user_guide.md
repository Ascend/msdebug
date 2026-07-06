# **MindStudio Debugger User Guide**

## Overview

MindStudio Debugger (msDebug for short) is an operator debugging tool for Ascend devices. It is used to debug operator programs running on NPUs and provides debugging methods for operator developers. The debugging methods include reading the memory and register of an Ascend device, and pausing and resuming the running status of a program. After testing the operator functions in a real-world hardware environment by starting operators or using the msOpST tool, you can determine whether to use the msDebug tool for function debugging based on the actual test situation.

**Scenarios**

The following operator call scenarios are supported:

- Kernel launch operator development: kernel launch

    For details about the kernel launch scenario, see section "[Completing Kernel Launch Based on the Sample Project](https://www.hiascend.com/document/detail/zh/canncommercial/850/opdevg/Ascendcopdevg/atlas_ascendc_10_0056.html)" in *Ascend C Operator Development Guide*. For details about the operation, see "[Debugging a Vector Operator on the Board](../best_practices/basic_cases.md#debugging-a-vector-operator-on-the-board)".

- Project-based operator development: single-operator API calling

    For details about the single-operator API execution scenario, see "Project-based Operator Development" > "[Single-Operator API Execution](https://www.hiascend.com/document/detail/zh/canncommercial/850/opdevg/Ascendcopdevg/atlas_ascendc_10_0070.html)" in *Ascend C Operator Development Guide*. For details about the operation, see "[Calling AscendCL Single-Operator](../best_practices/basic_cases.md#calling-ascendcl-single-operator)".

- AI framework operator adaptation: PyTorch framework

    For details about the single-operator calling scenario through the PyTorch framework, see "OpPlugin in [Ascend-developed Plugins](https://www.hiascend.com/document/detail/zh/Pytorch/720/modthirdparty/modparts/thirdpart_0009.html)" in *Ascend Extension for PyTorch Suite and Third-party Library Support List*. For details about the operation, see "[Debugging the Operators Called by a PyTorch Interface](../best_practices/basic_cases.md#debugging-the-operators-called-by-a-pytorch-interface)".

**Additional Information**

msDebug also provides the following extension program. For details, see [**Table 1** Extension program](#extension-program).

**Table 1** Extension program <a id="extension-program"></a>

<table><thead align="left"><tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row113563566121"><th class="cellrowborder" valign="top" width="50%" id="mcps1.2.3.1.1"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p456182519111">Program Name</p>
</th>
<th class="cellrowborder" valign="top" width="50%" id="mcps1.2.3.1.2"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p13356156171210">Description</p>
</th>
</tr>
</thead>
<tbody><tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row535645615122"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p180175116563">msdebug-mi(msDebug Machine Interface)</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p796410302155">Provides machine-to-machine interaction interfaces for data parsing, which users do not need to pay attention to.</p>
</td>
</tr>
</tbody>
</table>

## Preparations

**Environment Setup**

- Install msDebug by referring to [MindStudio Debugger Installation Guide](../install_guide/msdebug_install_guide.md).
- To enable msDebug, install the NPU driver and firmware using either of the following methods (method 1 is recommended for CANN 8.1.RC1 and later, and driver 25.0.RC1 and later):
    - Method 1: Specify the `--full` option during driver installation, and then run the `echo 1 > /proc/debug_switch` command as the `root` user to enable the debugging channel. Then the msDebug tool can be used.

        ```bash
        ./Ascend-hdk-<chip_type>-npu-driver_<version>_linux-<arch>.run --full
        ```

    - Method 2: Specify the `--debug` option during driver installation. For details, see "[Installing the NPU Driver and Firmware](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/softwareinst/instg/instg_0005.html?Mode=PmIns&InstallType=netconda&OS=openEuler&Software=cannToolKit)" in *CANN Software Installation Guide*.

        ```bash
        ./Ascend-hdk-<chip_type>-npu-driver_<version>_linux-<arch>.run --debug
        ```

**Constraints**

- The debugging channel has high permissions, which causes security risks. Exercise caution when using this tool. This tool is not recommended in the production environment. If you use this tool, you implicitly accept the risks involved.
- For a single device, only one msDebug tool can be used for debugging. You are not advised to run other operator programs at the same time.
- When the program to be debugged calls multiple operators, the msDebug tool can debug only a specified operator.
- During operator debugging, the overflow/underflow detection function is disabled.

## Supported Products

The following products are supported:

- Atlas A3 training products/Atlas A3 inference products
- Atlas A2 training products/Atlas A2 inference products

> [!NOTE]NOTE
>
>- For details about Ascend product models, see [Ascend Product Models](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html).
>- For details about the supported functions, see the documentation of the corresponding function module.

## Precautions

- You can run the `help` command to view all the commands supported by msDebug. Commands excluded in [command reference](#command-reference) are implemented by the open-source debugger LLDB. Pay attention to related risks when using LLDB. For details about how to use LLDB, see its [official document](https://lldb.llvm.org/).
- You need to ensure the execution security of executable files or applications.
    - You are advised to restrict the operation permission on executable files or applications to avoid privilege escalation risks.
    - Avoid high-risk operations (such as deleting files, deleting directories, changing passwords, and running privilege escalation commands) to prevent security risks.

## Command Reference

**Table 1** Command reference

<table><thead align="left"><tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row416221055015"><th class="cellrowborder" valign="top" width="20.75%" id="mcps1.2.5.1.1"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p5638288509">Command</p>
</th>
<th class="cellrowborder" valign="top" width="11.4%" id="mcps1.2.5.1.2"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1263182817506">Command Abbreviation</p>
</th>
<th class="cellrowborder" valign="top" width="28.84%" id="mcps1.2.5.1.3"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p15631828135015">Description</p>
</th>
<th class="cellrowborder" valign="top" width="39.01%" id="mcps1.2.5.1.4"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1863628175011">Example</p>
</th>
</tr>
</thead>
<tbody><tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row716251013503"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p12635289500">breakpoint set -f<em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i8429754192116"> filename -</em>l<em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i84292054182112"> </em><em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i1429195415214">linenum</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1363192845018">b</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p863132855016">Adds breakpoints.<em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i677164519161">f</em><em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i21605617108">ilename</em> indicates the operator implementation code file *.cpp.<em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i1905837141113">linenum</em> indicates the specific line number of the code file.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen87724714399">b add_custom.cpp:85</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1414712217501"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p18631928125019">run</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p16332812505">r</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1063122845019">Runs the program.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen11659617133917">r</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row198351923195014"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1063112865018">continue</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p263142812504">c</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1563182816509">Continues to run.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen84911931153920">c</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row137942018105019"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1463182895018">print <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i15430135452120">variable</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p26372815500">p</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p76312286505">Prints variables.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen1131323811710">p zLocal</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row7313919153715"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p17313819113717">frame <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i15430155482119">variable</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p731319195374">var</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p28508147519">Displays all local variables in the current scope.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen15115195553710">var</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row171637200508"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p66392813505">memory read</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p064182855014">x</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p191651552183115">Reads memory.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen138271438203917">x -m GM -f float16[] 0x00001240c0037000 -c 2 -s 128 -E 0</pre>
<ul id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_ul13312958144010"><li>-m: specifies the memory location. GM, UB, L0A, L0B, L0C, L1, FB, STACK, DCACHE, and ICACHE are supported. STACK, DCACHE, and ICACHE are used only when the dump file of an abnormal operator is parsed. </li><li>-s: specifies the number of bytes to be printed in each line. </li><li>-c: specifies the number of lines to be printed. </li><li>-f: specifies the type of data to be printed. </li><li>-E or --offset: skips the first x elements during printing. </li><li><em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i15451171161210">0x00001240c0037000</em>: indicates the memory address to be read. Replace it with the actual address.</li></ul>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row19163171018502"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p6645289509">ascend info devices</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p76415289506">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p76492865010">Queries device information.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen11996142164015">ascend info devices</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row416391017509"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p17647285509">ascend info cores</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p6532172723215">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p564142820502">Queries the AI Core information of the operator.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen1251785402">ascend info cores</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1535845412457"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p11358125415450">ascend info tasks</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p435865434511">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p735955412457">Queries information about the task where the operator runs.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen5372121319404">ascend info tasks</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1325915420409"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1825944210405">ascend info stream</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p725911421405">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p152591742124019">Queries information about the stream where the operator runs.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen836515149411">ascend info stream</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row185235467"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p118533154618">ascend info blocks</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p685536466">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1885133164612">Queries information about the block where the operator runs.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><div class="p" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p15705153812317">Prints information about the running blocks.<pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen638314402">ascend info blocks </pre>
</div>
<div class="p" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1186153124616">Prints the code of the running blocks at the current interrupt.<pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen11980104664011">ascend info blocks -d</pre>
</div>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row191631910175017"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p464202825015">ascend aic <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i11431754192114">id</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p18987203011321">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p4641728175011">Switches the Cube core focused by the debugger.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen17877195684013">ascend aic 1</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row416331019502"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p2641428205015">ascend aiv <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i15432165412217">id</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p18989163043219">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p16422812509">Switches the vector core focused by the debugger.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen145218134118">ascend aiv 5</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row3181194810592"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p8182648185913"><span class="uicontrol" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_uicontrol44351721535">"CTRL+C"</span></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p793245317597">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p118214814593">Manually interrupts the operator running program and displays the interruption location information.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p71828486594">Enter a value using the keyboard.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1458617378436"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p4621164015341">register read</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p18621740153413">re r</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p6621240193419">Reads the value of a register. `-a` reads the values of all registers. `$REG_NAME` reads the value of a register with a specified name.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen114593337362">register read -a
re r $PC</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row7330039457"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p8508131103816">thread step-over</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p19509121153810">next or n</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p165091193813">Moves to the next executable line of code in the same call stack.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen119943933810">n</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row14803122884719"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p10804328114711">thread step-in</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p1780413283471">step or s</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p58041528154713"><span>Enters the function for debugging.</span></p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen1807121513487">s</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row18161192214"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p10195617397">thread step-out</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p2797181610496">finish</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p4557175745411">Executes the remaining part of the function and returns to the main program to continue execution.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen0245644174915">finish</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_row37091156121913"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p250710395193">thread backtrace</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p11231112672717">bt</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_p18231426172718">Displays the code call stack information.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_screen12311326172716">bt</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row19151716201"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p7928842101620">target modules add &lt;kernel.o&gt;</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p49121720200">image add [kernel.o]</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p139121792010">Imports operator debugging information when the PyTorch framework calls operators.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen1796915154101">image add <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i1476443615177">xx</em>.o       </pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row456124511010"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p18615264176">target modules load --file &lt;kernel.o&gt; --slide &lt;address&gt;</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p9571845161015">image load -f &lt;kernel.o&gt; -s &lt;address&gt;</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1487619221194">Loads operator debugging information when the PyTorch framework calls operators to make the imported debugging information take effect.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen6924823141118">image load -f <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i181231253141710">xx</em>.o -s 0</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_row202489445542"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p169431537151114">msdebug --core corefile [kernel.o|fatbin]</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p132495441546">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><ul id="zh-cn_topic_0000001979357376_ul414333141315"><li>Loads the coredump file. </li><li>The second parameter is optional. If you need to use it, you can pass a file in any of the following formats: an operator file in kernel.o format generated by using the `-g` compilation, or an executable file or dynamic library file of the operator binary generated by using the `-g` compilation. This parameter is used to display the call stack of code lines.</li></ul>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="screen" id="zh-cn_topic_0000001979357376_screen129583121143">msdebug --core corefile xx.o
msdebug --core corefile</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_row43201348105417"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p10320248205416">ascend info summary</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p33205488544">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_p771932814511">Displays information about the coredump file.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="screen" id="zh-cn_topic_0000001979357376_screen919875915414">ascend info summary</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1814314103411"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p101438110349">help <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i1814810249351">msdebug_command</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p6852233193516">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p14144912346">Displays the help information about the tool command. The command output displays the function, syntax, and options of a command.</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen176871718193719">help run</pre>
<div class="p" id="zh-cn_topic_0000001979357376_p6453741123513"><strong id="zh-cn_topic_0000001979357376_b14453341143519">The help information about the core switching command</strong> is as follows:<pre class="code_wrap" id="zh-cn_topic_0000001979357376_screen9453174153515">(msdebug) <strong id="zh-cn_topic_0000001979357376_b84531641183514">help ascend aic</strong>
change the id of the focused ascend aicore.
Syntax: ascend aic &lt;id&gt;</pre>
</div>
<div class="p" id="zh-cn_topic_0000001979357376_p10464515123611"><strong id="zh-cn_topic_0000001979357376_b172605212363"></strong>The help information about the `ascend info blocks` command is as follows:<pre class="code_wrap" id="zh-cn_topic_0000001979357376_screen17464115103613">(msdebug) <strong id="zh-cn_topic_0000001979357376_b4963114083620">help ascend info blocks</strong>
show blocks overall info.
Syntax: ascend info blocks
Command Options Usage:
  ascend info blocks [-d]
       -d ( --details )
            Show stopped states for all blocks.</pre>
</div>
</td>
</tr>
</tbody>
</table>

> [!NOTE]NOTE
>
> - Currently, the `bt` command applies only to the coredump feature scenario. The call stack information is accurate only when `stop\_reason` is `CUBE\_ERROR`, `CCU\_ERROR`, `MTE\_ERROR`, `VEC\_ERROR`, and `FIXP\_ERROR`.
> - If the function name displayed in the `bt` command is too long, you can set it by referring to [formatting](https://lldb.llvm.org/use/formatting.html).
>
>    ```bash
>    setting set frame-format "frame #${frame.index}: ${frame.pc}{ ${module.file.basename}{{${frame.no-debug}${function.pc-offset}}}}{ at ${line.file.basename}:${line.number}{:${line.column}}}{${function.is-optimized} [opt]}{${frame.is-artificial} [artificial]}\n"
>    ```
>
> - After the `run` command is executed, run the `image add` command to import the debugging information. Then, run the `image load` command for the imported debugging information to take effect.

## Tool Usage

**Importing Debugging Information**

Before debugging an operator, enable the debugging `-g -O0` option and recompile the operator to include debugging information in the operator binary. For details, see [Compiling Operators Based on the Sample Project](../best_practices/basic_cases.md#debugging-a-vector-operator-on-the-board). The operator debugging information is automatically imported to the msDebug tool.

**Starting the tool**

The msDebug tool can be started in either of the following ways.

> [!NOTE]NOTE
> If `Cannot read termcap database; using dumb terminal settings` is displayed, configure `export TERMINFO=xx` to eliminate the message. `xx` indicates the local TERMINFO path.
>
> ```bash
> export TERMINFO=xx    # You can run the infocmp -D command to query the value of xx. You can select a path that meets the current terminal configuration as the value of TERMINFO.
> ```

- Load the executable file `application`.
    1. After the operator is built, the executable file `application` on the NPU can be obtained.
    2. Use msDebug to load the executable file.

        ```bash
        $ msdebug ./application
        ```

        > [!NOTE]NOTE
        >
        > - Perform one-click compilation and running based on the kernel framework of the Ascend C operator to generate the executable file `application` on the NPU. For details, see "Kernel Launch Operator Development" \> "[Kernel Launch](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0056.html)" in *Ascend C Operator Development Guide*.
        > - If the executable file has other input parameters, pass them as follows:
        >
        >    ```bash
        >    msdebug -- ./application --flag1 arg1 --flag2 args2 ...
        >    ```

- Load the Python script for operator calling.
    1. After plugins of the PyTorch framework are developed, you can directly call Ascend C custom operators from PyTorch through the custom Python script `test_ops_custom.py`.
    2. Use msDebug to load the Python script.

        ```bash
        $ msdebug python3 test_ops_custom.py
        msdebug(MindStudio Debugger) is part of MindStudio Operator-dev Tools.
        The tool provides developers with a mechanism for debugging Ascend kernels running on actual hardware.
        This enables developers to debug Ascend kernels without being affected by potential changes brought by simulation and emulation environments.
        (msdebug) target create "python3"
        Current executable set to '${INSTALL_DIR}/projects/application' (aarch64).
        (msdebug) settings set -- target.run-args  "test_ops_custom.py"
        (msdebug)
        ```

        > [!NOTE]NOTE
        > For details about the single-operator calling scenario through the PyTorch framework, see "OpPlugin in [Ascend-developed Plugins](https://www.hiascend.com/document/detail/zh/Pytorch/720/modthirdparty/modparts/thirdpart_0009.html)" in *Ascend Extension for PyTorch Suite and Third-party Library Support List*.

**Exiting Debugging**

Exit the debugger.

```bash
(msdebug) q
[localhost add_ascendc_sample]$
```

> [!NOTE]NOTE
> The debugging channel cannot be disabled independently. To disable the debugging channel, you need to enable the overwrite mode. For details, see the NPU driver and firmware installation documents.

**Specifying a Device ID (MC2 Operator Scenario)**

When debugging a single-process multi-thread MC2 operator, you can run the `ascend device ID` command (`ID` indicates the device ID) to specify the device ID to debug the operator on a specific device. This debugging mode has the following advantages:

- Higher debugging efficiency: By selecting a specific device, you can use hardware resources more efficiently and accelerate the debugging process.
- Well targeted: You can debug a specific device to detect and resolve performance bottlenecks or compatibility issues related to the device.
- Issue isolation: If a performance or function issue occurs, you can specify different device IDs to check whether the issue is caused by a specific device, thereby making it easier to locate the issue.

> [!NOTE]NOTE
>
> - If no device ID is specified, only the device ID set for the first time during program running is debugged.
> - The HCCL APIs do not support step-by-step debugging. For details about the APIs, see "High-Level APIs" \> "HCCL" \> > "[HCCL Kernel APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/ascendcopapi/atlasascendc_api_07_0869.html)" in *Ascend C Operator Development API Reference*.

```tex
py38) [root@localhost MC2-master]# msdebug /home/xxx/MC2-master/bin/alltoall_custom_aarch64
msdebug(MindStudio Debugger) is part of MindStudio Operator-dev Tools.
The tool provides developers with a mechanism for debugging Ascend kernels running on actual hardware.
This enables developers to debug Ascend kernels without being affected by potential changes brought by simulation and emulation environments.
(msdebug) target create "/home/xxx/MC2-master/bin/alltoall_custom_aarch64"
Current executable set to '/home/xxx/MC2-master/bin/alltoall_custom_aarch64' (aarch64).
(msdebug) b all_to_all_custom_v3.cpp:58
Breakpoint 1: 2 locations.
(msdebug) ascend device 1
(msdebug) run --x1_shape 72,17 --input_tensor_format ND --input_tensor_dtype fp16 --output_shape 72,17 --output_dtype fp16 --output_format ND --n_dev 2 --bin_path feature/aclnn/AllToAllCustom_fp16_ND_fuzz_000010 --loop_cnt 1 --platform 1971 --version 3 --tileM 128 | tee /home/shelltest/MC2-master/feature/aclnn/AllToAllCustom_fp16_ND_fuzz_000010/mc2_memory.log
Process 2625643 launched: '/home/xxx/MC2-master/bin/alltoall_custom_aarch64' (aarch64)
[INFO] rank 0 hcom: xx.xx.xx.xxx%enp189s0f0_60000_0_1747739573633567 stream: 0xaaaac9e14610, context : 0xaaaac9daeda0
[INFO] rank 1 hcom: xx.xx.xx.xxx%enp189s0f0_60000_0_1747739573633567 stream: 0xaaaaca8c8380, context : 0xaaaaca88f280
 before RunGraph : free :29837 M,  total:30196 M, used :358 M, ret :0
 before RunGraph : free :29835 M,  total:30196 M, used :360 M, ret :0
Process 2625643 stopped and restarted: thread 19 received signal: SIGCHLD
[INFO]  M is 72, K is 17, tileM is 128, tileNum is 0, tailM is 36, tailNum is 1, useBufferType is 0
[INFO]  M is 72, K is 17, tileM is 128, tileNum is 0, tailM is 36, tailNum is 1, useBufferType is 0
[Launch of Kernel AllToAllCustomV3_f1974b24a4ace3957d571b2712b3eadf_1000 on Device 1]
[Launch of Kernel AllToAllCustomV3_f1974b24a4ace3957d571b2712b3eadf_1000 on Device 1]
Process 2625643 stopped
[Switching to focus on Kernel AllToAllCustomV3_f1974b24a4ace3957d571b2712b3eadf_1000, CoreId 0, Type aiv]
* thread #1, name = 'alltoall_custom', stop reason = breakpoint 1.2
    frame #0: 0x0000000000004e0c AllToAllCustomV3_f1974b24a4ace3957d571b2712b3eadf.o`all_to_all_custom_v3_1000_tilingkey.vector(aGM="\x8b2d3+\xb5Ӫ\xbe\xb7\xa94\x87\xba;\xb6\xf68\U0000000e9\xc1\xa9", cGM="", workspaceGM="", tilingGM="d") at all_to_all_custom_v3.cpp:58:28
   55       auto &&cfg = tilingData.param;
   56       const uint8_t tileNum = cfg.tileNum;
   57       const uint8_t tailNum = cfg.tailNum;
-> 58       const uint64_t tileM = cfg.tileM;
   59       const uint64_t tailM = cfg.tailM;
   60       const uint64_t M = cfg.M;
   61       const uint64_t K = cfg.K;
```

## Breakpoint Setting

### Function

When using msDebug to debug an operator, you can set line breakpoints on the execution program of the operator, that is, set breakpoints at a specific line in the operator code file.

### Precautions

- If an operator implementation file with the same name exists on both the host and kernel, you are advised to use an absolute path to set a breakpoint to ensure that the breakpoint is set on the target file.
- When a breakpoint is set on the source code file, an alarm indicating that the actual location cannot be found may be displayed, as shown in the following. After the operator is executed, the actual location is automatically found and the breakpoint is automatically set.

    ```bash
    (msdebug) b /home/xx/op_kernel/matmul_leakyrelu_kernel.cpp:24
    Breakpoint 1: no locations (pending on future shared library load).
    WARNING:  Unable to resolve breakpoint to any actual locations.
    (msdebug)
    ```

- If the operator code is compiled into the dynamic library and loaded by using the operator launch symbol, when a breakpoint is set before the `run` command is executed, the command output indicates that the breakpoint position is not found (pending on future shared library load). The dynamic library is loaded only after the program is executed. The operator debugging information is parsed after the `run` command is executed, and then the breakpoint is updated and reset.

    ```bash
    (msdebug) b matmul_leakyrelu_kernel.cpp:55
    Breakpoint 1: no locations (pending on future shared library load).
    WARNING:  Unable to resolve breakpoint to any actual locations.
    (msdebug) run
    ...
    1 location added to breakpoint 1
    ...
    ```

### Example

**Setting a Line Breakpoint**

1. Add a breakpoint in line 114 of the kernel function implementation file `matmul\_leakyrelu`. If the following information is displayed, the breakpoint is successfully added:

    ```bash
    (msdebug) b matmul_leakyrelu_kernel.cpp:114
    Breakpoint 1: where = device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE7CopyOutEj_mix_aiv + 240 at matmul_leakyrelu_kernel.cpp:114:14, address = 0x000000000000ff88
    ```

    For details about the command output, see the following table.

    **Table 1** Information description

    <table><thead align="left"><tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row2416102584614"><th class="cellrowborder" valign="top" width="36.63%" id="mcps1.2.3.1.1"><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p641642504618">Field</p>
    </th>
    <th class="cellrowborder" valign="top" width="63.370000000000005%" id="mcps1.2.3.1.2"><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p5416142510461">Description</p>
    </th>
    </tr>
    </thead>
    <tbody><tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row2556152517514"><td class="cellrowborder" valign="top" width="36.63%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p12416122512469">device_debugdata</p>
    </td>
    <td class="cellrowborder" valign="top" width="63.370000000000005%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p341614250468">Name of the <strong id="zh-cn_topic_0000002015877393_b389113443">.o</strong> file on the device.</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row25566251454"><td class="cellrowborder" valign="top" width="36.63%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p1241672564613">matmul_leakyrelu_kernel.cpp</p>
    </td>
    <td class="cellrowborder" valign="top" width="63.370000000000005%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p12458527174814">Name of the kernel function where the breakpoint is located.</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row16331041191215"><td class="cellrowborder" valign="top" width="36.63%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p534541161215">CopyOut</p>
    </td>
    <td class="cellrowborder" valign="top" width="63.370000000000005%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p113412411126">Current function.</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row1245714571212"><td class="cellrowborder" valign="top" width="36.63%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p17581491317">240</p>
    </td>
    <td class="cellrowborder" valign="top" width="63.370000000000005%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p19839191017138">Offset of the breakpoint address relative to the address of the CopyOut function. In this example, the offset of 0xff88 relative to the address of the CopyOut function is 240.</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row19416152514619"><td class="cellrowborder" valign="top" width="36.63%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p15492921194918">address = 0x000000000000ff88</p>
    </td>
    <td class="cellrowborder" valign="top" width="63.370000000000005%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p7272133144916">Breakpoint address, that is, logical relative address.</p>
    </td>
    </tr>
    </tbody>
    </table>

2. Run the operator program and wait until the breakpoint is hit. `0x000000000000ff88` indicates the address of the PC where the breakpoint is located.

    ```bash
    (msdebug) run
    Process 165366 launched: '${INSTALL_DIR}/projects/normal_sample/mix/matmul_leakyrelu.fatbin' (aarch64)
    [Launch of Kernel matmul_leakyrelu_custom on Device 1]
    Process 165366 stopped
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 14, Type aiv]
    * thread #1, name = 'matmul_leakyrelu', stop reason = breakpoint 1.1
        frame #0: 0x000000000000ff88 device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE7CopyOutEj_mix_aiv(this=0x000000000019fb60, count=0) at matmul_leakyrelu_kernel.cpp:114:14
       111          (uint16_t)(tiling.baseN * sizeof(cType) / DEFAULT_C0_SIZE),
       112          0,
       113          (uint16_t)((tiling.N - tiling.baseN) * sizeof(cType) / DEFAULT_C0_SIZE)};
    -> 114      DataCopy(cGlobal[startOffset], reluOutLocal, copyParam);
       115      reluOutQueue_.FreeTensor(reluOutLocal);
       116  }
       117
    (msdebug)
    ```

**Printing Breakpoints**

Run the following command to print the positions and sequence numbers of all breakpoints that have been set.

```bash
(msdebug) breakpoint list
Current breakpoints:
1: file = 'add_custom.cpp', line = 85, exact_match = 0, locations = 1, resolved = 1, hit count = 1
  1.1: where = device_debugdata`::add_custom(uint8_t *__restrict, uint8_t *__restrict, uint8_t *__restrict) + 14348 [inlined] KernelAdd::CopyOut(int) + 1700 at add_custom.cpp:85:9, address = 0x000000000000380c, resolved, hit count = 1
```

**Deleting Breakpoints**

1. Delete the breakpoint with a specific line number.

    ```bash
    (msdebug) breakpoint delete 1
    1 breakpoints deleted; 0 breakpoint locations disabled.
    ```

2. Resume the running of the program. Due to breakpoint deletion, the program keeps running to the last minute.

    ```bash
    (msdebug) c
    Process 165366 resuming
    4096.00 4096.00 4096.00 4096.00 4096.00 4096.00 4096.00 4096.00
    4096.00 4096.00 4096.00 4096.00 4096.00 4096.00 4096.00 4096.00
    4096.00 4096.00 4096.00 4096.00 4096.00 4096.00 4096.00 4096.00
    4096.00 4096.00 4096.00 4096.00 4096.00 4096.00 4096.00 4096.00
    Process 165366 exited with status = 0 (0x00000000)
    (msdebug)
    ```

## Memory and Variable Printing

### Function

Based on the variable type and usage, a variable can be stored in a register or in the local memory or global memory. You can determine the storage location by printing the variable address and further view the associated memory content.

### Precautions

Currently, the msDebug tool cannot directly print the value of a template parameter by variable name. You need to print the value of the template parameter using the `p *Template_parameter_object*`. The value of the template parameter is displayed after printing. For example, `COMPUTE\_LENGTH` is a template parameter, and `this` is the object pointer to which the template parameter belongs. If you want to print the value of the parameter, run the `p this` command where the parameter is used. An example is provided as follows:

```bash
   22   template<class ArchTag_, class ElementAccumulator_, class ElementOut_, uint32_t COMPUTE_LENGTH>
   23   struct ReduceAdd {
   24       ReduceAdd(Arch::Resource<ArchTag> &resource)
   25       {
 -> 26            for (uint32_t i = 0; i < BUFFER_NUM; i++) {
   27               inputBuffer[i] = resource.ubBuf.template GetBufferByByte<ElementAccumulator>(bufferOffset);
   28               bufferOffset += COMPUTE_LENGTH * sizeof(ElementAccumulator);
(msdebug) p this
(Catlass::Gemm::Kernel::ReduceAdd<Catlass::Arch::AtlasA2, float, __fp16, 32> *) $0 = 0x00000000001cf838
```

### Example

**Printing Variables**

After a breakpoint is hit, you can run the `p variable\_name` command to print the value of a specified variable. For example:

```bash
(msdebug) p alpha
(float) $0 = 0.00100000005
(msdebug) p tiling
(const TCubeTiling) $1 = {
  usedCoreNum = 2
  M = 1024
  N = 640
  Ka = 256
  ...
}
```

**Printing GlobalTensor**

`GlobalTensor` is used to store the global data of the global memory (external storage).

You can run the following commands to print `GlobalTensor`. The following takes `cGlobal` as an example. The `address_` field specifies the memory address of `zGm`. In this example, the value is `0x000012c045400000`.

```bash
(msdebug) p cGlobal
(AscendC::GlobalTensor<float>) $0 = {
  AscendC::BaseGlobalTensor<float> = {
    address_ = 0x000012c045400000
    oriAddress_ = 0x000012c045400000
  }
  bufferSize_ = 655360
  shapeInfo_ = {
    shapeDim = '\0'
    originalShapeDim = '\0'
    shape = ([0] = 0, [1] = 0, [2] = 0, [3] = 0, [4] = 0, [5] = 0, [6] = 0, [7] = 0)
    originalShape = ([0] = 0, [1] = 0, [2] = 0, [3] = 0, [4] = 0, [5] = 0, [6] = 0, [7] = 0)
    dataFormat = ND
  }
  cacheMode_ = CACHE_MODE_NORMAL
}
```

The actual values of `GlobalTensor` variables are stored in the GM. Run the following command to print the values at `0x000012c045400000` in the GM. The example printing format contains the following information: one line to be printed, 256 bytes in each line, in float32 format.

```bash
(msdebug) x -m GM -f float32[] 0x000012c045400000 -s 256 -c 1
0x12c045400000: {4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096}
```

> [!NOTE]NOTE
>
> - If you want to print other custom addresses, ensure the validity of the custom addresses. Otherwise, errors may occur during operator running.
> - If you want to print the memory starting from a custom address, you can add an offset based on the `address\_` field as the start address. The unit of the offset is byte. After the offset GM memory address is obtained, enter it into the memory printing command.

**Printing LocalTensor**

`LocalTensor` is used to store the data in the local memory (internal storage) of the AI Core.

Run the following command to print the `LocalTensor` variable. `reluOutLocal` is used as an example. For the memory address of `reluOutLocal`, refer to the `bufferAddr` parameter in the `address\_` field. In this example, the address is `0`, and the length is `131072`.

```bash
(msdebug) p reluOutLocal
(AscendC::LocalTensor<float>) $2 = {
  AscendC::BaseLocalTensor<float> = {
    address_ = (dataLen = 131072, bufferAddr = 0, bufferHandle = "", logicPos = '\n')
  }
  shapeInfo_ = {
    shapeDim = '\0'
    originalShapeDim = '\0'
    shape = ([0] = 0, [1] = 1092616192, [2] = 4800, [3] = 1473680, [4] = 0, [5] = 1473888, [6] = 0, [7] = 1471968)
    originalShape = ([0] = 0, [1] = 3222199212, [2] = 4800, [3] = 1, [4] = 0, [5] = 1473376, [6] = 0, [7] = 1473376)
    dataFormat = ND
  }
}
```

The actual content of the tensor is stored in the UB memory. You can run the following command to print the value at address `0` in the UB memory. The example printing format contains the following information: one line to be printed, 256 bytes in each line, in float32 format.

```bash
(msdebug) x -m UB -f float32[] 0 -s 256 -c 1
0x00000000: {4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096}
```

> [!NOTE]NOTE
>
> - In this sample, the actual content of the tensor variables is stored in the UB. However, the local tensor may be stored in the UB, L1, L0A, or L0B. You need to determine store location based on the code, and select the correct memory type for the `-m` option of the printing command.
> - If you want to print the memory starting from a custom address, you can add an offset based on the `address\_` field as the start address. The unit of the offset is byte. After the offset GM memory address is obtained, enter it into the memory printing command.

**Printing All Local Variables**

Print all local variables in the current scope:

```bash
(msdebug) var
(MatmulLeakyKernel<__fp16, __fp16, float, float> *__stack__) this = 0x0000000000167b60
(uint32_t) count = 0
(const uint32_t) roundM = 2
(const uint32_t) roundN = 5
(uint32_t) startOffset = 0
(AscendC::DataCopyParams) copyParam = (blockCount = 256, blockLen = 16, srcStride = 0, dstStride = 64)
```

## Single-Step Debugging

### Function

To understand the code execution details, you can run the `thread step-over` command to execute the code line by line for single-step debugging, or run the `step in` command to enter the function for debugging, or run the `finish` command to return to the next line of the function call point to continue debugging.

### Precautions

During operator build, the build option of `--cce-ignore-always-inline=true` is used.

### Example

**Example for Using the thread step-over Command**

1. Set a breakpoint to the position to be debugged and run the program. For details about how to set a breakpoint, see [Breakpoint Setting](#breakpoint-setting).

    ```cpp
    (msdebug) r       // Running
    Process 177943 launched: '${INSTALL_DIR}/projects/mix/matmul_leakyrelu.fatbin' (aarch64)
    [Launch of Kernel matmul_leakyrelu_custom on Device 1]
    Process 177943 stopped
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 44, Type aiv]
    * thread #1, name = 'matmul_leakyrelu', stop reason = breakpoint 1.2
        frame #0: 0x000000000000f01c device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE10CalcOffsetEiiRK11TCubeTilingRiS4_S4_S4__mix_aiv(this=0x0000000000217b60, blockIdx=0, usedCoreNum=2, tiling=0x0000000000217e28, offsetA=0x00000000002175c8, offsetB=0x00000000002175c4, offsetC=0x00000000002175c0, offsetBias=0x00000000002175bc) at matmul_leakyrelu_kernel.cpp:129:15
       126
       127      offsetA = mCoreIndx * tiling.Ka * tiling.singleCoreM;
       128      offsetB = nCoreIndx * tiling.singleCoreN;
    -> 129      offsetC = mCoreIndx * tiling.N * tiling.singleCoreM + nCoreIndx * tiling.singleCoreN;        // Breakpoint position
       130      offsetBias = nCoreIndx * tiling.singleCoreN;
       131  }
       132
    (msdebug)
    ```

2. Enter the `next` or `n` command for step-by-step execution.

    ```cpp
    (msdebug) n
    Process 177943 stopped
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 44, Type aiv]
    * thread #1, name = 'matmul_leakyrelu', stop reason = step over   // If the PC location is displayed in the command output, the step-by-step execution is successful.
        frame #0: 0x000000000000f048 device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE10CalcOffsetEiiRK11TCubeTilingRiS4_S4_S4__mix_aiv(this=0x0000000000217b60, blockIdx=0, usedCoreNum=2, tiling=0x0000000000217e28, offsetA=0x00000000002175c8, offsetB=0x00000000002175c4, offsetC=0x00000000002175c0, offsetBias=0x00000000002175bc) at matmul_leakyrelu_kernel.cpp:130:18
       127      offsetA = mCoreIndx * tiling.Ka * tiling.singleCoreM;
       128      offsetB = nCoreIndx * tiling.singleCoreN;
       129      offsetC = mCoreIndx * tiling.N * tiling.singleCoreM + nCoreIndx * tiling.singleCoreN;
    -> 130      offsetBias = nCoreIndx * tiling.singleCoreN;
       131  }
    ```

3. Run the `ascend info cores` command to view the PC information and stop reason of all cores.

    ```cpp
    (msdebug) ascend info cores
      CoreId Type Device Stream Task Block               PC    stop reason Filename Line
          12  aic      1     3    0     0  0x12c0c00f03b0  breakpoint 1.2       NA   NA
    *     44  aiv      1     3    0     0  0x12c0c00f8048       step over       NA   NA               // * indicates the core that is currently running.
          45  aiv      1     3    0     0  0x12c0c00f801c  breakpoint 1.2       NA   NA
    ```

    > [!NOTE]NOTE
    >
    > - If the current core is stopped due to both step-by-step debugging and breakpoints, "breakpoint" is displayed.
    > - If the running program freezes, you can press "Ctrl+C" to interrupt the program. The possible causes of freezing are as follows:
    >    - The user program itself has an infinite loop, which needs to be rectified by repairing the program.
    >    - An operator uses synchronization instructions.

4. After the debugging is complete, run the `q` command and enter `Y` or `y` to end the debugging.

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```

**Example for Using the thread step-in and thread step-out Commands**

1. Set a breakpoint to the position to be debugged and run the program. For details about how to set a breakpoint, see [Breakpoint Setting](#breakpoint-setting).

    ```cpp
    (msdebug) r                // Running
    Process 180938 launched: '${INSTALL_DIR}/test/mstt/sample/normal_sample/mix/matmul_leakyrelu.fatbin' (aarch64)
    [Launch of Kernel matmul_leakyrelu_custom on Device 1]
    Process 180938 stopped
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 46, Type aiv]
    * thread #1, name = 'matmul_leakyrelu', stop reason = breakpoint 1.1
        frame #0: 0x000000000000e948 device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE7ProcessEPN7AscendC5TPipeE_mix_aiv(this=0x000000000021fb60, pipe=0x000000000021f6a8) at matmul_leakyrelu_kernel.cpp:83:9
       80       while (matmulObj.template Iterate<true>()) {
       81           MatmulCompute();
       82           LeakyReluCompute();
    -> 83           CopyOut(computeRound);
       84           computeRound++;
       85       }
       86       matmulObj.End();
    ```

2. Input `step` or `s` to enter the function for execution.

    ```bash
    (msdebug) s
    Process 180938 stopped
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 46, Type aiv]
    * thread #1, name = 'matmul_leakyrelu', stop reason = step in
        frame #0: 0x000000000000febc device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE7CopyOutEj_mix_aiv(this=0x000000000021fb60, count=0) at matmul_leakyrelu_kernel.cpp:106:5
       103  template <typename aType, typename bType, typename cType, typename biasType>
       104  __aicore__ inline void MatmulLeakyKernel<aType, bType, cType, biasType>::CopyOut(uint32_t count)
       105  {
    -> 106      reluOutQueue_.DeQue<cType>();
       107      const uint32_t roundM = tiling.singleCoreM / tiling.baseM;
       108      const uint32_t roundN = tiling.singleCoreN / tiling.baseN;
       109      uint32_t startOffset = (count % roundM * tiling.baseM * tiling.N + count / roundM * tiling.baseN);
    ```

3. Run the `ascend info cores` command to view the PC information and stop reason of all cores.

    ```cpp
    (msdebug) ascend info cores
      CoreId Type Device Stream Task Block               PC    stop reason Filename Line
          13  aic      1     3    0     0  0x12c0c00f1f88  breakpoint 1.1       NA   NA
    *     46  aiv      1     3    0     0  0x12c0c00f8ebc         step in       NA   NA
          47  aiv      1     3    0     0  0x12c0c00f8d3c  breakpoint 1.1       NA   NA
          // * indicates the core that is currently running.
          47  aiv      1     3    0     0  0x12c0c00f8d3c  breakpoint 1.1       NA   NA
    ```

    > [!NOTE]NOTE
    > If the current core is stopped due to both function debugging and breakpoints, `breakpoint` is displayed.

4. After debugging the CopyOut function, run the `finish` command to exit the CopyOut function and return to the main program to continue execution.

    ```bash
    (msdebug) finish
    Process 180938 stopped
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 46, Type aiv]
    * thread #1, name = 'matmul_leakyrelu', stop reason = step out
        frame #0: 0x000000000000e950 device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE7ProcessEPN7AscendC5TPipeE_mix_aiv(this=0x000000000021fb60, pipe=0x000000000021f6a8) at matmul_leakyrelu_kernel.cpp:84:21
       81           MatmulCompute();
       82           LeakyReluCompute();
       83           CopyOut(computeRound);
    -> 84           computeRound++;
       85       }
       86       matmulObj.End();
       87   }
    ```

## Running Interrupting

### Function

When the operator execution program freezes, manually interrupt the operator execution program and display the interrupted location information.

### Precautions

- If the running program freezes, you can press "Ctrl+C" to interrupt the program. The possible causes of freezing are as follows:
    - The user program itself has an infinite loop, which needs to be rectified by repairing the program.
    - An operator uses synchronization instructions.

- This function can debug only the operator programs started in msDebug.
- After the interruption takes effect, the debugging information displaying and core switching functions are supported. Currently, single-step debugging, register reading, memory and variable printing, and `continue` command are not supported.

### Example

1. When the operator execution program on the host or device suspends, enter "CTRL+C" to manually interrupt the operator execution program and display the interrupted location.

    ```cpp
    (msdebug) r
    Process 173221 launched: '${INSTALL_DIR}/projects/mix/matmul_leakyrelu.fatbin' (aarch64)
    [Launch of Kernel matmul_leakyrelu_custom on Device 1]
    // Enter CTRL+C.
    Process 173221 stopped
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 35, Type aiv]
    * thread #1, name = 'matmul_leakyrelu', stop reason = signal SIGSTOP
        frame #0: 0x000000000000ef5c device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE10CalcOffsetEiiRK11TCubeTilingRiS4_S4_S4__mix_aiv(this=<unavailable>, blockIdx=<unavailable>, usedCoreNum=<unavailable>, tiling=<unavailable>, offsetA=<unavailable>, offsetB=<unavailable>, offsetC=<unavailable>, offsetBias=<unavailable>) at matmul_leakyrelu_kernel.cpp:127:5
       124      auto mCoreIndx = blockIdx % mSingleBlocks;
       125      auto nCoreIndx = blockIdx / mSingleBlocks;
       126
    -> 127      while(true) {
       128      }
       129      offsetA = mCoreIndx * tiling.Ka * tiling.singleCoreM;
       130      offsetB = nCoreIndx * tiling.singleCoreN;
    (msdebug)
    ```

2. After the debugging is complete, run the `q` command and enter `Y` or `y` to end the debugging.

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```

## Core Switching

### Function

Switch the current core to the specified core. After the core is switched, the position of the code interruption of the specified core is automatically displayed.

### Example

- Assume that the running core is core 2 of the AIV, and the core to be switched is core 3 of the AIV.

    ```bash
    (msdebug) ascend aiv 3
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 3, Type aiv]
    * thread #1, name = 'matmul_leakyrelu', stop reason = breakpoint 1.1
        frame #0: 0x000000000000fd3c device_debugdata`_ZN7AscendC13WaitEventImplEt_mix_aiv(flagId=1) at kernel_operator_sync_impl.h:142:5
       139
       140  __aicore__ inline void WaitEventImpl(uint16_t flagId)
       141  {
    -> 142      wait_flag_dev(flagId);
       143  }
       144
       145  __aicore__ inline void SetSyncBaseAddrImpl(uint64_t config)
    ```

    After the switchover is complete, query the core information again. You can see that the core is switched to the line where the new core ID is located.

    ```bash
    (msdebug) ascend info cores
      CoreId Type Device Stream Task Block               PC    stop reason Filename Line
          17  aic      1     3    0     0  0x12c0c00f1f88  breakpoint 1.1       NA   NA
           2  aiv      1     3    0     0  0x12c0c00f8fbc  breakpoint 1.1       NA   NA
    *      3  aiv      1     3    0     0  0x12c0c00f8d3c  breakpoint 1.1       NA   NA
          17  aic      1     3    0     0  0x12c0c00f1f88  breakpoint 1.1       NA   NA
           2  aiv      1     3    0     0  0x12c0c00f8fbc  breakpoint 1.1       NA   NA
    *      3  aiv      1     3    0     0  0x12c0c00f8d3c  breakpoint 1.1       NA   NA
    ```

- Assume that the running core is core 3 of the AIV, and the core to be switched is core 17 of the AIC.

    ```bash
    (msdebug) ascend aic 17
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 17, Type aic]
    * thread #1, name = 'matmul_leakyrelu', stop reason = breakpoint 1.1
        frame #0: 0x0000000000008f88 device_debugdata`_ZN7AscendC7BarrierEv_mix_aic at kfc_comm.h:39
       36
       37   namespace AscendC {
       38   __aicore__ inline void Barrier()
    -> 39   {
       40   #if defined(__CCE_KT_TEST__) && __CCE_KT_TEST__ == 1
       41       __asm__ __volatile__("" ::: "memory");
       42   #else
    ```

    After the switchover is complete, query the core information again. You can see that the core is switched to the line where the new core ID is located.

    ```bash
    (msdebug) ascend info cores
      CoreId Type Device Stream Task Block               PC    stop reason Filename Line
    *    17  aic      1     3    0     0  0x12c0c00f1f88  breakpoint 1.1       NA   NA
          2  aiv      1     3    0     0  0x12c0c00f8fbc  breakpoint 1.1       NA   NA
          3  aiv      1     3    0     0  0x12c0c00f8d3c  breakpoint 1.1       NA   NA
    ```

## Program Status Checking

### Function

After using msDebug to call an operator, you can read register values of the device where the current breakpoint is located to check the program status.

### Example

- After `register read -a` is entered, all available register values on the current device are returned.

    ```bash
    (msdebug) register read -a
                      PC = 0x12C0C00F1F88
                    COND = 0x0
                    CTRL = 0x100000000003C
                    GPR0 = 0x12C041200100
                    GPR1 = 0x146FD9
                    GPR2 = 0x146FC8
                    GPR3 = 0x8001000800
                    GPR4 = 0x80300000100
                    GPR5 = 0x80000000000
                    GPR6 = 0x0
                    GPR7 = 0x300000000
                    GPR8 = 0x3
                    GPR9 = 0x1000000
                   GPR10 = 0xFFF
                   GPR11 = 0xFC0
                   GPR12 = 0x0
                   GPR13 = 0x0
                   GPR14 = 0x0
                   GPR15 = 0x11
                   GPR16 = 0x7FFF
                   GPR17 = 0x7A0
                   GPR18 = 0x0
                   GPR19 = 0x0
                   GPR20 = 0x0
                   GPR21 = 0x0
                   GPR22 = 0x0
                   GPR23 = 0x0
                   GPR24 = 0x0
                   GPR25 = 0x0
                   GPR26 = 0x0
                   GPR27 = 0x0
                   GPR28 = 0x0
                   GPR29 = 0x146EE8
                   GPR30 = 0x147640
                   GPR31 = 0x12C0C00F1ED4
                   LPCNT = 0x0
                  STATUS = 0x0
                 SYS_CNT = 0x774E308602
           ICACHE_PRL_ST = 0x0
           SAFETY_CRC_EN = 0x0
           ST_ATOMIC_CFG = 0x5
          CALL_DEPTH_CNT = 0x5
          CONDITION_FLAG = 0x1
          FFTS_BASE_ADDR = 0xE7FFE044F000
        CUBE_EVENT_TABLE = 0x70000000000
        FIXP_EVENT_TABLE = 0x0
        MTE1_EVENT_TABLE = 0x700000000
        MTE2_EVENT_TABLE = 0x0
      SCALAR_EVENT_TABLE = 0x0
    ```

- After `register read $\{variable name\}` is entered, the register value on the current device is returned. Separate multiple registers with spaces.

    - The register value is returned when the variable name is available on the current device.
    - `Invalid register name 'variable name'` is returned when the variable name is not available on the current device.

    ```bash
    (msdebug) register read $PC $test $GPR30
                      PC = 0x12C0C00F1F88
    Invalid register name 'test'.
                   GPR30 = 0x147640
    ```

## Debugging Information Displaying

### Function

Query information about the device where the operator runs.

### Example

**ascend info devices**

Run the following command to query the information about the device where the operator is running. The line where `\*` is located indicates the target device.

```bash
(msdebug) ascend info devices
  Device Aic_Num Aiv_Num Aic_Mask Aiv_Mask
*    1      1       2      0x10000     0x3
```

> [!NOTE]NOTE
> In the MC2 operator scenario, multiple device IDs are displayed.

For details about the command output, see the following table.

**Table 1** Information description

<table><thead align="left"><tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row8128154771415"><th class="cellrowborder" valign="top" width="30.97%" id="mcps1.2.3.1.1"><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p31281547101416">Field</p>
</th>
<th class="cellrowborder" valign="top" width="69.03%" id="mcps1.2.3.1.2"><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p0128114751411">Description</p>
</th>
</tr>
</thead>
<tbody><tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row0128147171410"><td class="cellrowborder" valign="top" width="30.97%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p20128947151413">Device</p>
</td>
<td class="cellrowborder" valign="top" width="69.03%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1012812472145">Logical ID of the device.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row1212810471141"><td class="cellrowborder" valign="top" width="30.97%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p121282478148">Aic_Num</p>
</td>
<td class="cellrowborder" valign="top" width="69.03%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p141281547191410">Number of used Cube Cores.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row2128147111412"><td class="cellrowborder" valign="top" width="30.97%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1212874713149">Aiv_Num</p>
</td>
<td class="cellrowborder" valign="top" width="69.03%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1112815471142">Number of used Vector Cores.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row0128104771414"><td class="cellrowborder" valign="top" width="30.97%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p13128147131417">Aic_Mask</p>
</td>
<td class="cellrowborder" valign="top" width="69.03%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p912844717141">Mask code of the actually used Cube, which is represented by 64 bits. If the nth bit is 1, Cube n is used.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row143818398182"><td class="cellrowborder" valign="top" width="30.97%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1038273961817">Aiv_Mask</p>
</td>
<td class="cellrowborder" valign="top" width="69.03%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p193821939111815">Mask code of the actually used Vector, which is represented by 64 bits. If the nth bit is 1, Vector n is used.</p>
</td>
</tr>
</tbody>
</table>

**ascend info cores**

Run the following command to query the information about the core where the operator is running. The line where `*` is located indicates the target core. In the following example, the target core is core 0 of the AIV.

```bash
(mdebug) ascend info cores
  CoreId Type Device Stream Task Block               PC    stop reason Filename Line
      16  aic      1     3    0     0  0x12c0c00f1fc0  breakpoint 1.1       NA   NA
*      0  aiv      1     3    0     0  0x12c0c00f8fcc  breakpoint 1.1       NA   NA
       1  aiv      1     3    0     0  0x12c0c00f8d3c  breakpoint 1.1       NA   NA
```

For details about the command output, see the following table.

**Table 2** Information description

<table><thead align="left"><tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row2096593410286"><th class="cellrowborder" valign="top" width="30.8%" id="mcps1.2.3.1.1"><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p92283663716">Field</p>
</th>
<th class="cellrowborder" valign="top" width="69.19999999999999%" id="mcps1.2.3.1.2"><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p12223643716">Description</p>
</th>
</tr>
</thead>
<tbody><tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row8965173412815"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p13965153414286">CoreId</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p89651234152813">Core ID of the AIV or AIC, starting from 0.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row696514346288"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p149651344286">Type</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p169658340287">Core type, which can be AIC or AIV.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row13965153432813"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p19651534102819">Device</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p7965834152816">Logical device ID.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row9965834132814"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p11965434142819">Stream</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p10965183411286">Stream ID delivered by the current kernel function. A stream consists of a series of tasks.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row1496518344285"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1096510345286">Task</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p16965113420289">ID of the task in the current stream. Task indicates the task delivered to the task scheduler for processing.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row496515347287"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p3965134102811">Block</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p096593492811">Number of cores on which the kernel function will be executed. Each core that executes the kernel function is assigned a logical ID, that is, block ID.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row109651834122811"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p29657347281">PC</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1196533442812">Logical absolute address of the PC on the current core.</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row14765174518331"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p15765145103310">Stop Reason</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1765345183310">Reason why the program execution stops, such as breakpoint, step in, step over, and Ctrl+C.</p>
</td>
</tr>
</tbody>
</table>

**ascend info tasks**

Run the following command to query the task information of the operator. The line where `*` is located indicates the target task, including device ID, stream ID, task ID, and invocation (name of the called kernel function).

```bash
(msdebug) ascend info tasks
  Device Stream Task Invocation
*   1       3     0  matmul_leakyrelu_custom
```

**ascend info stream**

Run the following command to query the stream information of the operator. The line where `*` is located indicates the target stream, including device ID, stream ID, and type (kernel type, which can be AIC or AIV).

```bash
(msdebug) ascend info stream
  Device Stream Type
*   1      3    aiv
```

**ascend info blocks**

Run the following command to query the block information of the operator. The line where `*` is located indicates the target block, including device ID, stream ID, task ID, and block ID.

```bash
(msdebug) ascend info blocks
  Device Stream Task Block
    1      3     0     0
*   1      3     0     0
    1      3     0     0
```

Run the following command to print the code of the running block at the current breakpoint:

```bash
(msdebug) ascend info blocks -d
Current stop state of all blocks:

[CoreId 16, Block 0]
* thread #1, name = 'matmul_leakyrelu', stop reason = breakpoint 1.1
    frame #0: 0x0000000000008fc0 device_debugdata`_ZN7AscendC14KfcMsgGetStateEj_mix_aic(flag=0) at kfc_comm.h:188
   185      return static_cast<KFC_Enum>((flag & 0xffff0000) >> KFC_MSG_BYTE_OFFSET);
   186  }
   187  __aicore__ inline uint32_t KfcMsgGetState(uint32_t flag)
-> 188  {
   189      return (flag & 0x00008000);
   190  }
   191  __aicore__ inline uint32_t KfcMsgMakeFlag(KFC_Enum funID, uint16_t instID)

[* CoreId 0, Block 0]
* thread #1, name = 'matmul_leakyrelu', stop reason = breakpoint 1.1
    frame #0: 0x000000000000ffcc device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE7CopyOutEj_mix_aiv(this=0x0000000000167b60, count=0) at matmul_leakyrelu_kernel.cpp:116:1
   113          (uint16_t)((tiling.N - tiling.baseN) * sizeof(cType) / DEFAULT_C0_SIZE)};
   114      DataCopy(cGlobal[startOffset], reluOutLocal, copyParam);
   115      reluOutQueue_.FreeTensor(reluOutLocal);
-> 116  }
   117
   118  template <typename aType, typename bType, typename cType, typename biasType>
   119  __aicore__ inline void MatmulLeakyKernel<aType, bType, cType, biasType>::CalcOffset(int32_t blockIdx,

[CoreId 1, Block 0]
* thread #1, name = 'matmul_leakyrelu', stop reason = breakpoint 1.1
    frame #0: 0x000000000000fd3c device_debugdata`_ZN7AscendC13WaitEventImplEt_mix_aiv(flagId=1) at kernel_operator_sync_impl.h:142:5
   139
   140  __aicore__ inline void WaitEventImpl(uint16_t flagId)
   141  {
-> 142      wait_flag_dev(flagId);
   143  }
   144
   145  __aicore__ inline void SetSyncBaseAddrImpl(uint64_t config)
```

## Abnormal Operator Dump File Parsing

### Function

If a hardware issue happens onsite, repeated stress tests are needed to reproduce the issue, which slows down troubleshooting. To solve this problem, the system initiates a dump operation upon detecting a potential hardware issue, and captures the current status information. The msDebug tool parses the dump file of an abnormal operator. You can collect sufficient data for fault analysis even without a stress test. The above functions enhance hardware exception detection and minimize repetitive stress tests.

> [!NOTE]NOTE
> Currently, only the function of parsing dump files of abnormal operators is supported by Ascend 950 products. Other functions are not supported by Ascend 950 products.

### Precautions

After the `acl.json` file is configured, other functions of msDebug cannot be used.

### Example

1. Prepare the `acl.json` configuration file.
    - Project-based operator development (single-operator API calling scenario): Create the `acl.json` file by referring to "[Initialization and Deinitialization](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/appdevg/acldevg/aclcppdevg_000007.html)" in *Application Development Guide \(C&C++\)* and load the file using the `aclinit` API.
    - AI framework operator adaptation (PyTorch framework scenario): Search for the `acl.json` file in the installation directory of `torch_npu`.

2. Enable the function of generating dump files for abnormal operators by referring to the configuration file example (dump configuration for abnormal operators) in "acl API Reference (C)" \> "System Configuration" \> "[aclInit](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0022.html)" in *Application Development Guide \(C&C++\)*.
    1. In the `acl.json` configuration file, set `dump\_scene` to `aic\_err\_detail\_dump`.
    2. In the `acl.json` configuration file, set `dump\_path` to the path for exporting the dump file of the abnormal operator.

3. If the program crashes (for example, memory overflow or segmentation fault), a core file of the abnormal operator is generated. The file name ends with .core.
4. Run the following command with the msDebug tool to load the dump file of the abnormal operator:

    ```bash
    msdebug --core output2/extra-info/data-dump/0/xxx.core add.fatbin
    msdebug(MindStudio Debugger) is part of MindStudio Operator-dev Tools.
    The tool provides developers with a mechanism for debugging Ascend kernels running on actual hardware.
    This enables developers to debug Ascend kernels without being affected by potential changes brought by simulation and emulation environments.
    (msdebug) target create "add.fatbin" --core "output2/extra-info/data-dump/0/xxx.core"
    Core file '/home/xxx/coredump_test/output2/extra-info/data-dump/0/xxx.core' (aarch64) was loaded.
    [Switching to focus on CoreId 26, Type aiv]
    ```

    > [!NOTE]NOTE
    > To view the call stack, use the `-O2/O3 + -g` option to compile and generate the `kernel.o` file that contains debugging information, or generate the ELF file of the fatbin structure.
    >
    > Cause: During operator execution, if a hardware exception occurs due to instruction execution, the hardware usually continues to execute several instructions before reporting the exception and generating a core file. Therefore, the memory and register data in the core file may be inaccurate. However, the value of the PC register is usually corrected.
    >
    > At the O2/O3 optimization level, the inline function is used by default. Call stack can still be traced accurately without requiring stack memory data. At the O0 optimization level, no inline function is used forcibly, and the stack memory data is inaccurate. Generally, accurate data requires the 0 stack frame.

5. View the dump file information of the abnormal operator.

    ```bash
    msdebug --core output2/extra-info/data-dump/0/xxx.core /home/xxxxx/Ascend/cann/opp/vendors/customize/op_impl/ai_core/tbe/kernel/ascend910b/add_custom/AddCustom_xxxx.o

    msdebug(MindStudio Debugger) is part of MindStudio Operator-dev Tools.
    The tool provides developers with a mechanism for debugging Ascend kernels running on actual hardware.
    This enables developers to debug Ascend kernels without being affected by potential changes brought by simulation and emulation environments.
    (msdebug) target create "/home/xxx/Ascend/cann/opp/vendors/customize/op_impl/ai_core/tbe/kernel/ascend910b/add_custom/AddCustom_xxx.o" --core "output2/extra-info/data-dump/0/xxx.core"
    Core file '/home/xxx/output2
    /extra-info/data-dump/0/xxx.core' (hiipu64) was loaded.
    [Switching to focus on CoreId 34, Type aiv]

    (msdebug) ascend info summary
      CoreId  CoreType        PC         DeviceId    ChipType
        33       AIV    0x12c0412004c8       0        A2/A3
     *  34       AIV    0x12c0412007c0       0        A2/A3
        35       AIV    0x12c0412007c0       0        A2/A3
        36       AIV    0x12c0412007c0       0        A2/A3
        37       AIV    0x12c0412007c0       0        A2/A3
        38       AIV    0x12c0412007c0       0        A2/A3
        39       AIV    0x12c0412007c0       0        A2/A3
        40       AIV    0x12c0412007c0       0        A2/A3

      Id           DataType                   MemType                     Addr                       Size             CoreId    CoreType    Dim
       0    DEVICE_KERNEL_OBJECT                GM                   0x12c041200000                 167872             NA         AIV        NA
       1            STACK                    GM/DCACHE           0xff000108000(invalid)              32768             33         AIV        NA
       2            STACK                    GM/DCACHE           0xff000110000(invalid)              32768             34         AIV        NA
       3            STACK                    GM/DCACHE           0xff000118000(invalid)              32768             35         AIV        NA
       4            STACK                    GM/DCACHE           0xff000120000(invalid)              32768             36         AIV        NA
       5            STACK                    GM/DCACHE           0xff000128000(invalid)              32768             37         AIV        NA
       6            STACK                    GM/DCACHE           0xff000130000(invalid)              32768             38         AIV        NA
       7            STACK                    GM/DCACHE           0xff000138000(invalid)              32768             39         AIV        NA
       8            STACK                    GM/DCACHE           0xff000140000(invalid)              32768             40         AIV        NA
       9      WORKSPACE_TENSOR                  GM                         0x0                         0               NA          NA        NA
      10         TILING_DATA                 GM/DCACHE               0x12c100000038                   16               NA          NA        NA
      11        OUTPUT_TENSOR                   GM                   0x12c0c0024000                  32768             NA          NA        [8, 2048]
      12        INPUT_TENSOR                    GM                   0x12c0c0012000                  32768             NA          NA        [8, 2048]
      13        INPUT_TENSOR                    GM                   0x12c0c001b000                  32768             NA          NA        [8, 2048]
      14            ARGS                     GM/DCACHE               0x12c100000000                   96               NA          NA        NA

    (msdebug) bt
       * thread #1, stop reason = VEC_ERROR
         * frame #0: 0x000012c0412004c8 AddCustom_xxx.o`::AddCustom_xxx_0(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__, u
       int8_t *__gm__, uint8_t *__gm__) [inlined] void AscendC::TPipe::ReleaseEventID<(AscendC::HardEvent)5>(this=<unavailable>, id=<unavailable>) at kernel_tpipe_impl.h:454:24
           frame #1: 0x000012c0412004c8 AddCustom_xxx.o`::AddCustom_xxx_0(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__, u
       int8_t *__gm__, uint8_t *__gm__) [inlined] AscendC::TQueBind<(AscendC::TPosition)0, (AscendC::TPosition)9, 2, 0>::AllocBuffer(this=<unavailable>) at kernel_tquebind_impl.h:512:3
       6
           frame #2: 0x000012c041200474 AddCustom_xxx.o`::AddCustom_xxx_0(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__, u
       int8_t *__gm__, uint8_t *__gm__) [inlined] AscendC::LocalTensor<half> AscendC::TQueBind<(this=<unavailable>)0, (AscendC::TPosition)9, 2, 0>::AllocTensor<half>() at kernel_tquebi
       nd_impl.h:78:16
           frame #3: 0x000012c041200474 AddCustom_xxx.o`::AddCustom_xxx_0(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__, u
       int8_t *__gm__, uint8_t *__gm__) [inlined] KernelAdd::CopyIn(this=<unavailable>, progress=<unavailable>) at add_custom.cpp:42:57
           frame #4: 0x000012c041200474 AddCustom_xxx.o`::AddCustom_xxx_0(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__, u
       int8_t *__gm__, uint8_t *__gm__) at add_custom.cpp:33:13
           frame #5: 0x000012c04120039c AddCustom_xxx.o`::AddCustom_xxx_0(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__, u
       int8_t *__gm__, uint8_t *__gm__) [inlined] add_custom_0_tilingkey(x=<unavailable>, y=<unavailable>, z=<unavailable>, workspace=<unavailable>, tiling=<unavailable>) at add_custom
       .cpp:83:8
           frame #6: 0x000012c041200064 AddCustom_xxx.o`::AddCustom_xxx_0(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__, u
       int8_t *__gm__, uint8_t *__gm__) [inlined] ascendc_auto_gen_add_custom_kernel(x_in__=<unavailable>, y_in__=<unavailable>, z_out_=<unavailable>, workspace=<unavailable>, tiling=<
       unavailable>) at AddCustom_xxx_3800102_kernel.cpp:43:5
           frame #7: 0x000012c04120004c AddCustom_xxx.o`::AddCustom_xxx_0(x_in__=<unavailable>, y_in__=<unavailable>, z_out_=<
       unavailable>, workspace=<unavailable>, tiling=<unavailable>) at AddCustom_xxx_3800102_kernel.cpp:48:5

    ```

6. For details about how to locate hardware exceptions, see [Core Switching](#core-switching), [Program Status Checking](#program-status-checking), and [Memory and Variable Printing](#memory-and-variable-printing).
7. After the debugging is complete, run the `q` command and enter `Y` or `y` to end the debugging.

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```
