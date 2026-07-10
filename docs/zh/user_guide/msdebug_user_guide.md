# **MindStudio Debugger工具用户指南**

## 简介

MindStudio Debugger（算子调试工具，msDebug）是一款面向昇腾设备的算子调试工具，用于调试NPU侧运行的算子程序，为算子开发人员提供调试手段。调试手段包括了读取昇腾设备内存与寄存器、暂停与恢复程序运行状态等。用户使用其他拉起算子的方式或msOpST工具在真实的硬件环境中对算子的功能进行测试后，可根据实际测试情况选择是否使用msDebug工具进行功能调试。

**调用场景**

支持如下调用算子的场景：

- Kernel直调算子开发：Kernel直调。

    Kernel直调的场景，详细信息可参考《Ascend C算子开发指南》中“[基于样例工程完成Kernel直调](https://www.hiascend.com/document/detail/zh/canncommercial/850/opdevg/Ascendcopdevg/atlas_ascendc_10_0056.html)”章节。具体操作请参见[上板调试Vector算子](../best_practices/msdebug_basic_cases.md#上板调试vector算子)。

- 工程化算子开发：单算子API调用。

    单算子API调用的场景，详细信息可参考《Ascend C算子开发指南》中“工程化算子开发 \>  [单算子API调用](https://www.hiascend.com/document/detail/zh/canncommercial/850/opdevg/Ascendcopdevg/atlas_ascendc_10_0070.html)”章节。具体操作请参见[调用Ascend CL单算子](../best_practices/msdebug_basic_cases.md#调用ascend-cl单算子)。

- AI框架算子适配：PyTorch框架。

    通过PyTorch框架进行单算子调用的场景，详细信息可参考《Ascend Extension for PyTorch 套件与三方库支持清单》中“[昇腾自研插件](https://www.hiascend.com/document/detail/zh/Pytorch/720/modthirdparty/modparts/thirdpart_0009.html)”章节中OpPlugin插件。具体操作请参见[调试PyTorch接口调用的算子](../best_practices/msdebug_basic_cases.md#调试pytorch接口调用的算子)。

**补充说明**

msDebug工具还提供了以下扩展程序，具体请参考[**表 1**  扩展程序说明](#扩展程序说明)。

**表 1**  扩展程序说明<a id="扩展程序说明"></a>

<table><thead align="left"><tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row113563566121"><th class="cellrowborder" valign="top" width="50%" id="mcps1.2.3.1.1"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p456182519111">程序名称</p>
</th>
<th class="cellrowborder" valign="top" width="50%" id="mcps1.2.3.1.2"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p13356156171210">说明</p>
</th>
</tr>
</thead>
<tbody><tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row535645615122"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p180175116563">msdebug-mi（msDebug Machine Interface）</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p796410302155">提供机机交互接口用于实现数据解析，用户无需关注。</p>
</td>
</tr>
</tbody>
</table>

## 使用前准备

**环境准备**

- 请参考[MindStudio Debugger安装指南](../install_guide/msdebug_install_guide.md)安装msDebug工具。
- 若要使能msDebug工具，需通过以下两种方法安装NPU驱动固件（CANN 8.1.RC1之后的版本且驱动为25.0.RC1之后的版本，推荐使用方法一）：
    - 方法一：驱动安装时指定`--full`参数，然后再使用root用户执行`echo 1 > /proc/debug_switch`命令启用调试通道，msDebug工具便可正常使用。

        ```bash
        ./Ascend-hdk-<chip_type>-npu-driver_<version>_linux-<arch>.run --full
        ```

    - 方法二：驱动安装时指定`--debug`参数，具体安装操作请参见《CANN 软件安装指南》中的“[安装NPU驱动固件](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/softwareinst/instg/instg_0005.html?Mode=PmIns&InstallType=netconda&OS=openEuler&Software=cannToolKit)”章节。

        ```bash
        ./Ascend-hdk-<chip_type>-npu-driver_<version>_linux-<arch>.run --debug
        ```

**约束**

- 调试通道权限较大，存在安全风险，请谨慎使用，生产环境不推荐使用，使用本调试工具即代表认可并接受该风险。
- 单Device仅支持使用单个msDebug工具进行调试，且不推荐同时运行其他算子程序。
- 当被调试程序调用多个算子时，msDebug工具仅支持对指定的单个算子进行调试。
- 调试算子时，溢出检测功能会关闭。

## 产品支持情况

支持的产品形态如下：

- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品
- Atlas 推理系列产品
- Ascend 950 系列产品

> [!NOTE]
>
>- 昇腾产品的具体型号，请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。
>- 具体功能支持范围可前往对应功能模块资料进行查看。

## 注意事项

- 通过键入help命令可查看msDebug工具支持的所有命令。[命令参考](#命令参考)之外的命令属于开源调试器lldb实现，使用需注意相关风险，详细使用方法可参考[lldb官方文档](https://lldb.llvm.org/)。
- 用户需自行保证可执行文件或用户程序（_application_）执行的安全性。
    - 建议限制对可执行文件或用户程序（_application_）的操作权限，避免提权风险。
    - 不建议进行高危操作（删除文件、删除目录、修改密码及提权命令等），避免安全风险。
- 工具运行过程中涉及从LD_LIBRARY_PATH加载so，用户在使用前需要确保LD_LIBRARY_PATH环境变量内容安全可信，指向路径不涉及软链接，且权限及属主符合安全预期，无法被第三方篡改，否则有任意代码注入风险。

## 命令参考

**表 2**  命令参考说明

<table><thead align="left"><tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row416221055015"><th class="cellrowborder" valign="top" width="20.75%" id="mcps1.2.5.1.1"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p5638288509">命令</p>
</th>
<th class="cellrowborder" valign="top" width="11.4%" id="mcps1.2.5.1.2"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1263182817506">命令缩写</p>
</th>
<th class="cellrowborder" valign="top" width="28.84%" id="mcps1.2.5.1.3"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p15631828135015">描述</p>
</th>
<th class="cellrowborder" valign="top" width="39.01%" id="mcps1.2.5.1.4"><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1863628175011">示例</p>
</th>
</tr>
</thead>
<tbody><tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row716251013503"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p12635289500">breakpoint set -f<em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i8429754192116"> filename -</em>l<em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i84292054182112"> </em><em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i1429195415214">linenum</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1363192845018">b</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p863132855016">增加断点，<em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i677164519161">f</em><em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i21605617108">ilename</em>为算子实现代码文件*.cpp，<em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i1905837141113">linenum</em>为代码文件对应的具体行号。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen87724714399">b add_custom.cpp:85</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1414712217501"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p18631928125019">run</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p16332812505">r</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1063122845019">运行程序。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen11659617133917">r</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row198351923195014"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1063112865018">continue</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p263142812504">c</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1563182816509">继续运行。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen84911931153920">c</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row137942018105019"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1463182895018">print <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i15430135452120">variable</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p26372815500">p</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p76312286505">打印变量。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen1131323811710">p zLocal</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row7313919153715"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p17313819113717">frame <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i15430155482119">variable</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p731319195374">var</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p28508147519">显示当前作用域内的所有局部变量。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen15115195553710">var</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row171637200508"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p66392813505">memory read</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p064182855014">x</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p191651552183115">读内存。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen138271438203917">x -m GM -f float16[] 0x00001240c0037000 -c 2 -s 128 -E 0</pre>
<ul id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_ul13312958144010"><li>-m：指定内存位置，支持GM/UB/L0A/L0B/L0C/L1/FB/STACK/DCACHE/ICACHE，其中STACK/DCACHE/ICACHE仅在“解析异常算子dump文件”时使用。</li><li>-s：指定每行打印字节数。</li><li>-c：指定打印的行数。</li><li>-f：指定打印的数据类型。</li><li>-E/--offset：指定打印时跳过前x个元素。</li><li><em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i15451171161210">0x00001240c0037000</em>：需要读取的内存地址，请根据实际环境进行替换。</li></ul>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row19163171018502"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p6645289509">ascend info devices</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p76415289506">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p76492865010">查询Device信息。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen11996142164015">ascend info devices</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row416391017509"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p17647285509">ascend info cores</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p6532172723215">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p564142820502">查询算子所运行的aicore相关信息。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen1251785402">ascend info cores</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1535845412457"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p11358125415450">ascend info tasks</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p435865434511">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p735955412457">查询算子所运行的task相关信息。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen5372121319404">ascend info tasks</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1535845412458"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p11358125415451">ascend info threads</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p435865434512">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p735955412458">查询上板调试时simt_vf内的线程信息和生成的coredump文件的线程信息。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen5372121319405">ascend info threads</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1535845412458"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p11358125415459">ascend thread &lt;thread-id&gt;</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p435865434526">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p735955412485">查询当前聚焦的核所用的simt线程相关信息，支持上板调试simt_vf下使用。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen638314455">ascend thread &lt;thread-id&gt; </pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1535845412459"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p11358125415466">ascend thread &lt;(thread_id_x, thread_id_y, thread_id_z)&gt;</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p435865434578">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p735955412488">切换调试所聚焦的simt线程信息，支持上板调试simt_vf下使用。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen5372121319416">ascend thread &lt;(thread_id_x, thread_id_y, thread_id_z)&gt;</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1535845412459"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p11358125415452">frame select &lt;frame-id&gt;</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p435865434513">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p735955412458">展示某一个栈帧对应的代码。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen5372121319406">frame select &lt;frame-id&gt;</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1325915420409"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1825944210405">ascend info stream</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p725911421405">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p152591742124019">查询算子所运行的stream相关信息。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen836515149411">ascend info stream</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row185235467"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p118533154618">ascend info blocks</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p685536466">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1885133164612">查询算子所运行的block相关信息。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><div class="p" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p15705153812317">打印所运行的blocks相关信息：<pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen638314402">ascend info blocks </pre>
</div>
<div class="p" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1186153124616">打印所运行的blocks在当前中断处的代码：<pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen11980104664011">ascend info blocks -d</pre>
</div>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row191631910175017"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p464202825015">ascend aic <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i11431754192114">id</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p18987203011321">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p4641728175011">切换调试器所聚焦的Cube核。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen17877195684013">ascend aic 1</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row416331019502"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p2641428205015">ascend aiv <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i15432165412217">id</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p18989163043219">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p16422812509">切换调试器所聚焦的Vector核。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen145218134118">ascend aiv 5</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row3181194810592"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p8182648185913"><span class="uicontrol" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_uicontrol44351721535">“CTRL+C”</span></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p793245317597">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p118214814593">手动中断算子运行程序并回显中断位置信息。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p71828486594">通过键盘输入。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1458617378436"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p4621164015341">register read</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p18621740153413">re r</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p6621240193419">读取寄存器值；-a读取所有寄存器值；$REG_NAME读取指定名称的寄存器值；</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen114593337362">register read -a
re r $PC</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row7330039457"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p8508131103816">thread step-over</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p19509121153810">next或n</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p165091193813">在同一个调用栈中，移动到下一个可执行的代码行。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen119943933810">n</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row14803122884719"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p10804328114711">thread step-in</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p1780413283471">step或s</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p58041528154713"><span>使用step in命令可进入到函数内部进行调试</span>。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen1807121513487">s</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row18161192214"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p10195617397">thread step-out</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p2797181610496">finish</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p4557175745411">使用finish命令会执行完函数内剩余部分，并返回主程序继续执行。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen0245644174915">finish</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_row37091156121913"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p250710395193">thread backtrace</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p11231112672717">bt</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_p18231426172718">用于展示此时代码调用栈信息。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_screen12311326172716">bt</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row19151716201"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p7928842101620">target modules add &lt;kernel.o&gt;</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p49121720200">image add [kernel.o]</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p139121792010">用于PyTorch框架调用算子时，导入算子调试信息。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen1796915154101">image add <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i1476443615177">xx</em>.o       </pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row456124511010"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p18615264176">target modules load --file &lt;kernel.o&gt; --slide &lt;address&gt;</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p9571845161015">image load -f &lt;kernel.o&gt; -s &lt;address&gt;</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p1487619221194">用于PyTorch框架调用算子时，加载算子调试信息，使导入的调试信息生效。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen6924823141118">image load -f <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i181231253141710">xx</em>.o -s 0</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_row202489445542"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p169431537151114">msdebug --core corefile [kernel.o|fatbin]</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p132495441546">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><ul id="zh-cn_topic_0000001979357376_ul414333141315"><li>用于加载Core dump文件。</li><li>第二个参数可选，如需使用，可传入以下任一形式的文件：通过-g编译生成的kernel.o格式的算子文件，或-g编译生成的算子二进制的可执行文件或动态库文件，用于展示代码行调用栈。</li></ul>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="screen" id="zh-cn_topic_0000001979357376_screen129583121143">msdebug --core corefile xx.o
msdebug --core corefile</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_row43201348105417"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_p10320248205416">ascend info summary</p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_p33205488544">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_p771932814511">用于查看Core dump文件信息。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="screen" id="zh-cn_topic_0000001979357376_screen919875915414">ascend info summary</pre>
</td>
</tr>
<tr id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_row1814314103411"><td class="cellrowborder" valign="top" width="20.75%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p101438110349">help <em id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_i1814810249351">msdebug_command</em></p>
</td>
<td class="cellrowborder" valign="top" width="11.4%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p6852233193516">-</p>
</td>
<td class="cellrowborder" valign="top" width="28.84%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_p14144912346">输出对应工具命令的帮助信息。打印信息将会展示该命令的功能描述、使用语法以及参数选项。</p>
</td>
<td class="cellrowborder" valign="top" width="39.01%" headers="mcps1.2.5.1.4 "><pre class="code_wrap" id="zh-cn_topic_0000001979357376_zh-cn_topic_0000001831604757_screen176871718193719">help run</pre>
<div class="p" id="zh-cn_topic_0000001979357376_p6453741123513"><strong id="zh-cn_topic_0000001979357376_b14453341143519">核切换</strong>命令的帮助信息如下所示：<pre class="code_wrap" id="zh-cn_topic_0000001979357376_screen9453174153515">(msdebug) <strong id="zh-cn_topic_0000001979357376_b84531641183514">help ascend aic</strong>
change the id of the focused ascend aicore.
Syntax: ascend aic &lt;id&gt;</pre>
</div>
<div class="p" id="zh-cn_topic_0000001979357376_p10464515123611"><strong id="zh-cn_topic_0000001979357376_b172605212363">ascend info blocks</strong>命令的帮助信息如下所示：<pre class="code_wrap" id="zh-cn_topic_0000001979357376_screen17464115103613">(msdebug) <strong id="zh-cn_topic_0000001979357376_b4963114083620">help ascend info blocks</strong>
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

> [!NOTE]
>
> - bt命令当前只适用于coredump特性场景，调用栈信息仅在stop_reason为以下error时：CUBE_ERROR、CCU_ERROR、MTE_ERROR、VEC_ERROR、FIXP_ERROR，保证准确性。
> - 对于bt的展示，如果函数名过长，可以参考[链接](https://lldb.llvm.org/use/formatting.html)进行设置：
>
>    ```bash
>    setting set frame-format "frame #${frame.index}: ${frame.pc}{ ${module.file.basename}{{${frame.no-debug}${function.pc-offset}}}}{ at ${line.file.basename}:${line.number}{:${line.column}}}{${function.is-optimized} [opt]}{${frame.is-artificial} [artificial]}\n"
>    ```
>
> - 当程序执行run命令后，需先执行image add命令导入调试信息。然后再执行image load命令使导入的调试信息生效。

## 工具使用

**导入调试信息**

算子调试前，需先启用调试`-g -O0`编译选项重新编译，使算子二进制带上调试信息，具体方法可参考[基于样例工程编译算子](../best_practices/msdebug_basic_cases.md#上板调试vector算子)。算子调试信息会自动被导入msDebug工具。

**启动工具**

msDebug工具支持以下两种启动方式：

> [!NOTE]
> 若工具弹出**Cannot read termcap database; using dumb terminal settings.**  的提示信息，可以通过配置`export TERMINFO=xx`消除提示，xx为本地TERMINFO路径：
>
> ```bash
> export TERMINFO=xx    # xx信息可通过infocmp -D命令查询，可以选择符合当前终端配置的路径作为TERMINFO值
> ```

- 加载可执行文件application。
    1. 算子编译后可获取NPU侧可执行文件application。
    2. 输入如下命令，使用msDebug工具加载可执行文件。

        ```bash
        $ msdebug ./application
        ```

        > [!NOTE]
        >
        > - 基于Ascend C算子的Kernel侧框架执行一键式编译运行，可生成NPU侧可执行文件application，具体操作可参考《Ascend C算子开发指南》中的“Kernel直调算子开发 \>  [Kernel直调](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0056.html)”章节。
        > - 若可执行文件有其他入参，则按照如下形式传入入参：
        >
        >    ```bash
        >    msdebug -- ./application --flag1 arg1 --flag2 args2 ...
        >    ```

- 加载调用算子的Python脚本
    1. 完成了PyTorch框架的适配插件开发后，即可实现从PyTorch框架调用Ascend C自定义算子，可以通过自定义Python脚本`test_ops_custom.py`调用算子。
    2. 输入如下命令，使用msDebug工具加载Python脚本。

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

        > [!NOTE]
        > 通过PyTorch框架进行单算子调用的场景，详细信息可参考《Ascend Extension for PyTorch 套件与三方库支持清单》中“[昇腾自研插件](https://www.hiascend.com/document/detail/zh/Pytorch/720/modthirdparty/modparts/thirdpart_0009.html)”章节中OpPlugin插件。

**调试退出**

输入以下命令，退出调试器。

```bash
(msdebug) q
[localhost add_ascendc_sample]$
```

> [!NOTE]
> 该调试通道无法单独关闭，若要关闭调试通道，需要通过覆盖安装方式，具体请参见对应的NPU驱动和固件安装文档。

**指定Device ID（通算融合算子场景）**

用户在调试单进程多线程类型的通算融合算子时，根据自身需求执行**ascend device ID**命令（ID为Device ID的数字）指定Device ID，实现在特定的Device上进行调试。这种调试方式具有以下优点：

- 提高调试效率：通过选择特定的Device，可以更高效地利用硬件资源，加快调试过程。
- 针对性强：能够针对特定设备进行调试，有助于发现和解决与该设备相关的性能瓶颈或兼容性问题。
- 便于隔离问题：当遇到性能或功能问题时，可以通过指定不同的设备ID来确定问题是否由特定设备引起，从而更容易定位问题所在。

> [!NOTE]
>
> - 如果不指定，则仅对用户程序运行时首次设置的Device ID进行调试。
> - Hccl接口不支持单步调试功能，具体接口明细请参见《Ascend C算子开发接口》中的“高阶API \> Hccl \>  [Hccl Kernel侧接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/ascendcopapi/atlasascendc_api_07_0869.html)”章节。

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

## 断点设置功能介绍

### 功能说明

使用msDebug工具调试算子时，可在算子的运行程序上设置行断点，即在算子代码文件的特定行号上设置断点。

### 注意事项

- 如果Host侧和Kernel侧存在同名的算子实现文件，在设置断点时，推荐采用绝对路径进行设置，确保断点打在预期的文件上。
- 在对源码文件进行打点时，可能会出现找不到实际位置的告警，类似如下提示，在算子运行后，会自动找到实际位置，并自动设置断点。

    ```bash
    (msdebug) b /home/xx/op_kernel/matmul_leakyrelu_kernel.cpp:24
    Breakpoint 1: no locations (pending on future shared library load).
    WARNING:  Unable to resolve breakpoint to any actual locations.
    (msdebug)
    ```

- 若算子代码被编译进动态库中，通过算子调用符加载，当在运行run命令前设置断点时，回显会告知暂时未找到断点位置（pending on future shared library load），动态库在程序运行后才会被加载，算子调试信息在运行run命令后完成解析，此时断点会重新更新并完成设置。

    ```bash
    (msdebug) b matmul_leakyrelu_kernel.cpp:55
    Breakpoint 1: no locations (pending on future shared library load).
    WARNING:  Unable to resolve breakpoint to any actual locations.
    (msdebug) run
    ...
    1 location added to breakpoint 1
    ...
    ```

- Ascend 950 系列产品中的场景中，simd vf函数及其子函数必须inline，导致大量代码行信息丢失，无法解析出断点信息。可通过在simd vf内添加`__asm__("NOP")`语句，并在该行设置断点。

    ```cpp
    __simd_vf__ inline void funcA () {
        for(uint16_t i = 0; i < repeatTimes; ++i) {
            AscendC::Reg::LoadAlign<>(xxxx);
            __asm__("NOP"); // 增加这一行后，断点可以设置这行
        }
    }
    ```

### 使用示例

**设置行断点**

1. 输入以下命令，在核函数实现文件matmul_leakyrelu的第114行增加断点，出现回显显示成功添加1个断点，如下所示。

    ```bash
    (msdebug) b matmul_leakyrelu_kernel.cpp:114
    Breakpoint 1: where = device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE7CopyOutEj_mix_aiv + 240 at matmul_leakyrelu_kernel.cpp:114:14, address = 0x000000000000ff88
    ```

    关键信息说明如下表：

    **表 3**  信息说明

    <table><thead align="left"><tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row2416102584614"><th class="cellrowborder" valign="top" width="36.63%" id="mcps1.2.3.1.1"><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p641642504618">字段</p>
    </th>
    <th class="cellrowborder" valign="top" width="63.370000000000005%" id="mcps1.2.3.1.2"><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p5416142510461">释义</p>
    </th>
    </tr>
    </thead>
    <tbody><tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row2556152517514"><td class="cellrowborder" valign="top" width="36.63%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p12416122512469">device_debugdata</p>
    </td>
    <td class="cellrowborder" valign="top" width="63.370000000000005%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p341614250468">设备侧<strong id="zh-cn_topic_0000002015877393_b389113443">.o</strong>文件名。</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row25566251454"><td class="cellrowborder" valign="top" width="36.63%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p1241672564613">matmul_leakyrelu_kernel.cpp</p>
    </td>
    <td class="cellrowborder" valign="top" width="63.370000000000005%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p12458527174814">断点所在的Kernel函数名。</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row16331041191215"><td class="cellrowborder" valign="top" width="36.63%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p534541161215">CopyOut</p>
    </td>
    <td class="cellrowborder" valign="top" width="63.370000000000005%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p113412411126">当前函数。</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row1245714571212"><td class="cellrowborder" valign="top" width="36.63%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p17581491317">240</p>
    </td>
    <td class="cellrowborder" valign="top" width="63.370000000000005%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p19839191017138">本次断点地址相对CopyOut函数的地址偏移量，即当前断点地址（0xff88）相对CopyOut函数所在地址的偏移量是240。</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_row19416152514619"><td class="cellrowborder" valign="top" width="36.63%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p15492921194918">address = 0x000000000000ff88</p>
    </td>
    <td class="cellrowborder" valign="top" width="63.370000000000005%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000002015877393_zh-cn_topic_0000001831604765_p7272133144916">断点的地址，即逻辑相对地址。</p>
    </td>
    </tr>
    </tbody>
    </table>

2. 输入如下命令，运行算子程序，等待直到命中断点。“0x000000000000ff88”代表该断点所在的pc地址。

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

**打印断点**

输入以下命令，将会打印所有已设置的断点位置以及序号。

```bash
(msdebug) breakpoint list
Current breakpoints:
1: file = 'add_custom.cpp', line = 85, exact_match = 0, locations = 1, resolved = 1, hit count = 1
  1.1: where = device_debugdata`::add_custom(uint8_t *__restrict, uint8_t *__restrict, uint8_t *__restrict) + 14348 [inlined] KernelAdd::CopyOut(int) + 1700 at add_custom.cpp:85:9, address = 0x000000000000380c, resolved, hit count = 1
```

**删除断点**

1. 输入以下命令，删除对应序号的断点。

    ```bash
    (msdebug) breakpoint delete 1
    1 breakpoints deleted; 0 breakpoint locations disabled.
    ```

2. 输入以下命令，恢复程序运行，因断点已被删除，则程序会一直运行直至结束。

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

## 内存与变量打印功能介绍

### 功能说明

根据变量类型和用法，变量可以存储在寄存器中或存储在Local Memory、Global Memory内存中。用户可以通过打印变量地址确定存储位置，并进一步查看关联的内存内容。

### 注意事项

目前msDebug工具不支持直接通过变量名打印模板参数的值，需要以打印`p`模板参数对应的对象方式进行，在打印后的类型里展示模板参数的值。例如COMPUTE_LENGTH为模板参数，this为该模板参数所属的对象指针，若要打印该参数的值，可以在使用该参数的位置，通过命令`p this`进行打印，示例如下：

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

### 使用示例

**打印变量**

命中断点后，使用 p variable_name 的命令形式可打印指定的变量的值，比如：

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

**打印GlobalTensor**

GlobalTensor一般用来存放Global Memory（外部存储）的全局数据。

输入以下命令，进行GlobalTensor变量打印。以cGlobal为例，zGm所在内存地址请参考`address_`字段，此处为“0x000012c045400000”。

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

因GlobalTensor类型变量实际的值保存在GM内存中，输入以下命令，打印GM内存中位于地址“0x000012c045400000”上的值，打印格式设置为：打印1行，每行256字节，按照float32格式打印。

```bash
(msdebug) x -m GM -f float32[] 0x000012c045400000 -s 256 -c 1
0x12c045400000: {4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096}
```

> [!NOTE]
>
> - 若需要打印其他自定义地址，用户需自行保证该自定义地址的合法性，否则可能会导致算子运行出错。
> - 若需要以自定义地址为起始进行内存打印，可基于address_字段作为起始地址增加偏移，偏移量单位为字节数，得到偏移后的GM内存地址后，传入内存打印命令即可。
> - 当前支持断点停在Kernel侧和Host侧时读取并展示GM内存。

**打印LocalTensor**

LocalTensor一般用于存放AI Core中Local Memory（内部存储）的数据。

输入以下命令，进行LocalTensor变量打印，以reluOutLocal为例，reluOutLocal所在内存地址请参考address_字段中的bufferAddr参数，此处位于0上，长度为131072。

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

该Tensor变量的实际内容保存在UB内存中，输入以下命令，打印UB内存中位于地址0上的值，打印格式设置为：打印1行，每行256字节，按照float32格式打印。

```bash
(msdebug) x -m UB -f float32[] 0 -s 256 -c 1
0x00000000: {4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096 4096}
```

> [!NOTE]
>
> - 本用例中，Tensor变量的实际内容保存在UB上，但LocalTensor不一定都保存在UB中，也可能在L1/L0A/L0B上，需要用户根据代码自行判断，然后在打印命令的`-m`选项中选择正确的内存类型。
> - 若需要以自定义地址为起始进行内存打印，可基于address_字段作为起始地址增加偏移，偏移量单位为字节数，得到偏移后的GM内存地址后，传入内存打印命令即可。

**打印所有局部变量**

输入以下命令，打印当前作用域所有局部变量。

```bash
(msdebug) var
(MatmulLeakyKernel<__fp16, __fp16, float, float> *__stack__) this = 0x0000000000167b60
(uint32_t) count = 0
(const uint32_t) roundM = 2
(const uint32_t) roundN = 5
(uint32_t) startOffset = 0
(AscendC::DataCopyParams) copyParam = (blockCount = 256, blockLen = 16, srcStride = 0, dstStride = 64)
```

## 单步调试功能介绍

### 功能说明

用户需要了解代码执行具体情况时，可使用`thread step-over`命令使用示例逐行执行以进行单步调试，或执行`step in`命令可进入函数内部进行调试，或可执行`finish`命令返回函数调用点的下一行继续调试。

### 注意事项

算子编译时，使用`--cce-ignore-always-inline=true`的编译选项。

### 使用示例

**thread step-over命令使用示例**

1. 将断点设置在需要调试的位置，并运行。断点设置的具体操作请参见[断点设置功能介绍](#断点设置功能介绍)。

    ```cpp
    (msdebug) r       // 运行
    Process 177943 launched: '${INSTALL_DIR}/projects/mix/matmul_leakyrelu.fatbin' (aarch64)
    [Launch of Kernel matmul_leakyrelu_custom on Device 1]
    Process 177943 stopped
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 44, Type aiv]
    * thread #1, name = 'matmul_leakyrelu', stop reason = breakpoint 1.2
        frame #0: 0x000000000000f01c device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE10CalcOffsetEiiRK11TCubeTilingRiS4_S4_S4__mix_aiv(this=0x0000000000217b60, blockIdx=0, usedCoreNum=2, tiling=0x0000000000217e28, offsetA=0x00000000002175c8, offsetB=0x00000000002175c4, offsetC=0x00000000002175c0, offsetBias=0x00000000002175bc) at matmul_leakyrelu_kernel.cpp:129:15
       126
       127      offsetA = mCoreIndx * tiling.Ka * tiling.singleCoreM;
       128      offsetB = nCoreIndx * tiling.singleCoreN;
    -> 129      offsetC = mCoreIndx * tiling.N * tiling.singleCoreM + nCoreIndx * tiling.singleCoreN;        //断点位置
       130      offsetBias = nCoreIndx * tiling.singleCoreN;
       131  }
       132
    (msdebug)
    ```

2. 输入next或n命令后，开始单步执行。

    ```cpp
    (msdebug) n
    Process 177943 stopped
    [Switching to focus on Kernel matmul_leakyrelu_custom, CoreId 44, Type aiv]
    * thread #1, name = 'matmul_leakyrelu', stop reason = step over   //   通过回显可查看pc的位置，表示单步成功
        frame #0: 0x000000000000f048 device_debugdata`_ZN17MatmulLeakyKernelIDhDhffE10CalcOffsetEiiRK11TCubeTilingRiS4_S4_S4__mix_aiv(this=0x0000000000217b60, blockIdx=0, usedCoreNum=2, tiling=0x0000000000217e28, offsetA=0x00000000002175c8, offsetB=0x00000000002175c4, offsetC=0x00000000002175c0, offsetBias=0x00000000002175bc) at matmul_leakyrelu_kernel.cpp:130:18
       127      offsetA = mCoreIndx * tiling.Ka * tiling.singleCoreM;
       128      offsetB = nCoreIndx * tiling.singleCoreN;
       129      offsetC = mCoreIndx * tiling.N * tiling.singleCoreM + nCoreIndx * tiling.singleCoreN;
    -> 130      offsetBias = nCoreIndx * tiling.singleCoreN;
       131  }
    ```

3. 输入`ascend info cores`命令，查看所有核的PC信息和停止原因。

    ```cpp
    (msdebug) ascend info cores
      CoreId Type Device Stream Task Block               PC    stop reason Filename Line
          12  aic      1     3    0     0  0x12c0c00f03b0  breakpoint 1.2       NA   NA
    *     44  aiv      1     3    0     0  0x12c0c00f8048       step over       NA   NA               //* 代表当前正在运行的核
          45  aiv      1     3    0     0  0x12c0c00f801c  breakpoint 1.2       NA   NA
    ```

    > [!NOTE]
    >
    > - 当前核的停止原因既有单步调试又有断点时，将展示为breakpoint。
    > - 若运行程序出现卡顿的现象，可以通过键盘输入“CTRL+C”中断运行程序。运行卡顿的原因可能是以下情况：
    >    - 用户程序本身存在死循环，需要通过修复程序解决。
    >    - 算子使用了同步类指令。

4. 调试完以后，执行`q`命令并输入Y或y结束调试。

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```

**thread step-in和thread step-out命令使用示例**

1. 将断点设置在需要调试的位置，并运行。断点设置的具体操作请参见[断点设置功能介绍](#断点设置功能介绍)。

    ```cpp
    (msdebug) r                // 运行
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

2. 用户输入step或s后，开始进入函数内部进行执行。

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

3. 输入`ascend info cores`命令，查看所有核的PC信息和停止原因。

    ```cpp
    (msdebug) ascend info cores
      CoreId Type Device Stream Task Block               PC    stop reason Filename Line
          13  aic      1     3    0     0  0x12c0c00f1f88  breakpoint 1.1       NA   NA
    *     46  aiv      1     3    0     0  0x12c0c00f8ebc         step in       NA   NA          //*代表当前正在运行的核
          47  aiv      1     3    0     0  0x12c0c00f8d3c  breakpoint 1.1       NA   NA
    ```

    > [!NOTE]
    > 当前核的停止原因既有调试函数又有断点时，将展示为breakpoint。

4. 调试完CopyOut函数后，运行`finish`命令退出CopyOut函数，并返回主程序继续执行。

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

## 中断运行功能介绍

### 功能说明

当算子运行程序卡顿时，手动中断算子运行程序并回显中断位置信息。

### 注意事项

- 若运行程序出现卡顿的现象，可以通过键盘输入“CTRL+C”中断运行程序。运行卡顿的原因可能是以下情况：
    - 用户程序本身存在死循环，需要通过修复程序解决。
    - 算子使用了同步类指令。

- 此功能仅支持调试在msDebug工具内启动的算子程序，无法调试在msDebug工具外启动的应用程序。
- 中断生效后，支持调试信息展示和核切换功能，暂不支持单步调试、读取寄存器、内存与变量打印功能和`continue`命令。

### 使用示例

1. Host侧或Device侧的算子运行程序卡顿时，用户可通过键盘输入“CTRL+C”，可手动中断算子运行程序并回显中断位置信息。

    ```cpp
    (msdebug) r
    Process 173221 launched: '${INSTALL_DIR}/projects/mix/matmul_leakyrelu.fatbin' (aarch64)
    [Launch of Kernel matmul_leakyrelu_custom on Device 1]
    //  键盘输入CTRL+C命令
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

2. 调试完以后，执行q命令并输入Y或y结束调试。

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```

## 核切换功能介绍

### 功能说明

将当前聚焦的核切换至指定的核，切核后会自动展示指定核代码中断处的位置。

### 使用示例

- 如果当前运行的核为aiv的“core 2”，指定切换的核为aiv的“core 3”。

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

    完成切换后，再次查询核信息可看到已切换至新指定的核id所在行。

    ```bash
    (msdebug) ascend info cores
      CoreId Type Device Stream Task Block               PC    stop reason Filename Line
          17  aic      1     3    0     0  0x12c0c00f1f88  breakpoint 1.1       NA   NA
           2  aiv      1     3    0     0  0x12c0c00f8fbc  breakpoint 1.1       NA   NA
    *      3  aiv      1     3    0     0  0x12c0c00f8d3c  breakpoint 1.1       NA   NA
    ```

- 如果当前运行的核为aiv的“core 3”，指定切换的核为aic的“core 17”。

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

    完成切换后，再次查询核信息可看到已切换至新指定的核id所在行。

    ```bash
    (msdebug) ascend info cores
      CoreId Type Device Stream Task Block               PC    stop reason Filename Line
    *    17  aic      1     3    0     0  0x12c0c00f1f88  breakpoint 1.1       NA   NA
          2  aiv      1     3    0     0  0x12c0c00f8fbc  breakpoint 1.1       NA   NA
          3  aiv      1     3    0     0  0x12c0c00f8d3c  breakpoint 1.1       NA   NA
    ```

## 检查程序状态功能介绍

### 功能说明

当用户使用msDebug调起算子后，可以通过命令行读取当前断点所在设备的寄存器值，检查程序状态。

### 使用示例

- 输入`register read -a`后，返回当前设备上所有可用的寄存器值。

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

- 输入register read $\{变量名\}，返回当前设备上该寄存器值。一次性读取多个寄存器时，需用空格隔开。

    - 当变量名在当前设备上可用时，返回该寄存器值。
    - 当变量名在当前设备上不可用时，返回Invalid register name '变量名'。

    ```bash
    (msdebug) register read $PC $test $GPR30
                      PC = 0x12C0C00F1F88
    Invalid register name 'test'.
                   GPR30 = 0x147640
    ```

## 调试信息展示功能介绍

### 功能说明

查询算子运行的设备信息。

### 使用示例

**ascend info devices**

输入以下命令查询算子运行的设备信息，\*所在行代表当前聚焦的设备。

```bash
(msdebug) ascend info devices
  Device Aic_Num Aiv_Num Aic_Mask Aiv_Mask
*    1      1       2      0x10000     0x3
```

> [!NOTE]
> 通算融合算子场景将会显示多个Device ID。

关键信息说明如下表：

**表 4**  信息说明

<table><thead align="left"><tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row8128154771415"><th class="cellrowborder" valign="top" width="30.97%" id="mcps1.2.3.1.1"><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p31281547101416">字段</p>
</th>
<th class="cellrowborder" valign="top" width="69.03%" id="mcps1.2.3.1.2"><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p0128114751411">释义</p>
</th>
</tr>
</thead>
<tbody><tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row0128147171410"><td class="cellrowborder" valign="top" width="30.97%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p20128947151413">Device</p>
</td>
<td class="cellrowborder" valign="top" width="69.03%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1012812472145">设备逻辑ID。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row1212810471141"><td class="cellrowborder" valign="top" width="30.97%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p121282478148">Aic_Num</p>
</td>
<td class="cellrowborder" valign="top" width="69.03%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p141281547191410">使用的Cube核数量。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row2128147111412"><td class="cellrowborder" valign="top" width="30.97%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1212874713149">Aiv_Num</p>
</td>
<td class="cellrowborder" valign="top" width="69.03%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1112815471142">使用的Vector核数量。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row0128104771414"><td class="cellrowborder" valign="top" width="30.97%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p13128147131417">Aic_Mask</p>
</td>
<td class="cellrowborder" valign="top" width="69.03%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p912844717141">实际使用的Cube的mask码，用64 bit位表示，如果第n位bit为1，表示使用了Cube n。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row143818398182"><td class="cellrowborder" valign="top" width="30.97%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1038273961817">Aiv_Mask</p>
</td>
<td class="cellrowborder" valign="top" width="69.03%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p193821939111815">实际使用的Vector的mask码，用64 bit位表示，如果第n位bit为1，表示使用了Vector n。</p>
</td>
</tr>
</tbody>
</table>

**ascend info cores**

输入以下命令查询算子运行的核信息，\*所在行代表当前聚焦的核。如下所示当前聚焦的核为aiv的“core 0”。

```bash
(mdebug) ascend info cores
  CoreId Type Device Stream Task Block               PC    stop reason Filename Line
      16  aic      1     3    0     0  0x12c0c00f1fc0  breakpoint 1.1       NA   NA
*      0  aiv      1     3    0     0  0x12c0c00f8fcc  breakpoint 1.1       NA   NA
       1  aiv      1     3    0     0  0x12c0c00f8d3c  breakpoint 1.1       NA   NA
```

关键信息说明如下表：

**表 5**  信息说明

<table><thead align="left"><tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row2096593410286"><th class="cellrowborder" valign="top" width="30.8%" id="mcps1.2.3.1.1"><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p92283663716">字段</p>
</th>
<th class="cellrowborder" valign="top" width="69.19999999999999%" id="mcps1.2.3.1.2"><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p12223643716">释义</p>
</th>
</tr>
</thead>
<tbody><tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row8965173412815"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p13965153414286">CoreId</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p89651234152813">aiv或aic的核id，从0开始。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row696514346288"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p149651344286">Type</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p169658340287">核类型，包括aic或aiv。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row13965153432813"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p19651534102819">Device</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p7965834152816">设备逻辑id。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row9965834132814"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p11965434142819">Stream</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p10965183411286">当前Kernel函数下发的Stream ID，Stream由一系列的task组成。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row1496518344285"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1096510345286">Task</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p16965113420289">当前Stream里的Task ID。Task表示下发给Task scheduler处理的任务。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row496515347287"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p3965134102811">Block</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p096593492811">表示核函数将会在几个核上执行。每个执行该核函数的核会被分配一个逻辑ID，即block_id。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row109651834122811"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p29657347281">PC</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1196533442812">当前核上的PC逻辑绝对地址。</p>
</td>
</tr>
<tr id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_row14765174518331"><td class="cellrowborder" valign="top" width="30.8%" headers="mcps1.2.3.1.1 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p15765145103310">Stop Reason</p>
</td>
<td class="cellrowborder" valign="top" width="69.19999999999999%" headers="mcps1.2.3.1.2 "><p id="zh-cn_topic_0000001979517160_zh-cn_topic_0000001757581488_p1765345183310">表示程序执行停止原因，有breakpoint、step in、 step over和ctrl+c等。</p>
</td>
</tr>
</tbody>
</table>

**ascend info tasks**

输入以下命令查询算子运行的Task信息，\*所在行代表当前聚焦的Task，包括Device ID、Stream ID、Task ID、Invocation（被调用的核函数名称）。

```bash
(msdebug) ascend info tasks
  Device Stream Task Invocation
*   1       3     0  matmul_leakyrelu_custom
```

**ascend info stream**

输入以下命令查询算子运行的Stream信息，\*所在行代表当前聚焦的Stream，包括Device ID、Stream ID、Type（核类型，包括aic或aiv）。

```bash
(msdebug) ascend info stream
  Device Stream Type
*   1      3    aiv
```

**ascend info blocks**

输入以下命令查询算子运行的Block信息，\*所在行代表当前聚焦的Block，包括Device ID、Stream ID、Task ID、Block ID。

```bash
(msdebug) ascend info blocks
  Device Stream Task Block
    1      3     0     0
*   1      3     0     0
    1      3     0     0
```

输入以下命令，打印所运行的Block在当前中断处的代码。

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

## SIMT线程切换功能介绍

### 功能说明

用于查询线程间信息，支持上板调试simt_vf下使用。

### 使用示例

**ascend info threads**

输入以下命令查询线程信息，\*所在行代表当前聚焦的设备。

```bash
(msdebug) ascend info threads
 ThreadIdx To ThreadIdx ActiveCount             PC ActiveMask   Filename Line
*  (0,0,0)     (15,1,0)          32 0x12004ca00808 0xffffffff kernel.cpp   36
   (0,2,0)     (15,3,0)          32 0x12004ca00808 0xffffffff kernel.cpp   36
   (0,4,0)     (15,5,0)          32 0x12004ca00808 0xffffffff kernel.cpp   36
   (0,6,0)     (15,7,0)          32 0x12004ca00808 0xffffffff kernel.cpp   36
   (0,8,0)     (15,9,0)          32 0x12004ca00808 0xffffffff kernel.cpp   36
```

关键信息说明如下表：

**表 6**  信息说明

|字段|释义|
|----|----|
| ThreadIDx To ThreadIDx | 从thread到thread，处于活跃的线程|
| ActiveCount | 线程数 |
| PC | 程序计数器 |
| ActiveMask | 表示ActiveCount线程里哪些是活跃的线程，bit=1的表示活跃的线程 |
| Filename | 表示这个线程所处的代码文件 |
| Line | 表示这个线程所处的行号 |

**ascend thread &lt;thread id&gt;**

查询当前聚焦的核所用的simt线程相关信息。

```bash
(msdebug) ascend thread 100
[Switching to focus on Kernel vec_add, CoreId 36, Type aiv, Thread (4, 6, 0)]
* thread #1, name = 'test.fatbin', stop reason = breakpoint 1.1
    frame #0: 0x000012004ca00808 device_debugdata_0`void SimtCompute<unsigned int>(dst=<unavailable>, src0=<unavailable>, src1=<unavailable>) (.vector) at kernel.cpp:36:15
   33       //uint64_t arr[3];
   34       //arr[0] = dst[0];
   35       int idx = Simt::GetThreadIdx<0>();
-> 36       int idy = Simt::GetThreadIdx<1>();
   37       int idz = Simt::GetThreadIdx<2>();
   38       int blockIdx = Simt::GetBlockIdx();
   39       int tmp = 10;
```

**ascend thread &lt;(thread_id_x, thread_id_y, thread_id_z)&gt;**

切换调试所聚焦的simt线程信息。

```bash
(msdebug) ascend thread (0,8,0)
[Switching to focus on Kernel vec_add, CoreId 36, Type aiv, Thread (0, 8, 0)]
* thread #1, name = 'test.fatbin', stop reason = breakpoint 1.1
    frame #0: 0x000012004ca00808 device_debugdata_0`void SimtCompute<unsigned int>(dst=<unavailable>, src0=<unavailable>, src1=<unavailable>) (.vector) at kernel.cpp:36:15
   33       //uint64_t arr[3];
   34       //arr[0] = dst[0];
   35       int idx = Simt::GetThreadIdx<0>();
-> 36       int idy = Simt::GetThreadIdx<1>();
   37       int idz = Simt::GetThreadIdx<2>();
   38       int blockIdx = Simt::GetBlockIdx();
   39       int tmp = 10;
```

## 解析异常算子dump文件功能介绍

### 功能说明

客户现场发生硬件异常时，需要反复压测复现问题，定位效率低。为了解决该问题，系统检测到潜在的硬件异常时，会自动触发一个dump操作，捕获当前的状态信息。msDebug工具通过对异常算子dump文件的解析，即使在没有主动压测的情况下也能收集到足够的数据用于问题分析。通过上述功能，不仅提高了硬件异常问题的定位效率，还减少因反复压测给用户带来的不便。

### 注意事项

若开启异常算子dump功能，配置后将不能使用msDebug的其他功能。

### 使用示例

1. 参见《应用开发指南 (C&C++)》的"acl API参考（C） \> 系统配置 \>  [aclInit](https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/910beta3/API/runtimeapi/aclcppdevg_03_0022.html#ZH-CN_TOPIC_0000002594788866__section1939018362581)"章节的配置文件示例（异常算子Dump配置），开启生成异常算子core文件的功能。

    > [!NOTE]
    > 以下两种方法二选一即可：

    1. 执行如下命令，将dump_scene参数设置为aic_err_detail_dump，并配置dump_path参数设置导出异常算子core文件的路径。

        ```bash
        export ASCEND_DUMP_SCENE="aic_err_detail_dump"
        export ASCEND_DUMP_PATH="output"
        ```

    2. 在acl.json配置文件中，将dump_scene参数设置为aic_err_detail_dump，并配置dump_path参数设置导出异常算子core文件的路径。

2. 算子运行出现aic_error异常时（如内存越界访问），触发生成异常算子core文件，文件名以.core结尾。
3. 使用msDebug工具执行以下命令，加载异常算子core文件。

    ```bash
    msdebug --core output2/extra-info/data-dump/0/xxx.core
    msdebug(MindStudio Debugger) is part of MindStudio Operator-dev Tools.
    The tool provides developers with a mechanism for debugging Ascend kernels running on actual hardware.
    This enables developers to debug Ascend kernels without being affected by potential changes brought by simulation and emulation environments.
    (msdebug) target create --core "output2/extra-info/data-dump/0/xxx.core"
    Core file '/home/xxx/coredump_test/output2/extra-info/data-dump/0/xxx.core' (hiipu64) was loaded.
    [Switching to focus on Kernel add_custom, CoreId 1, Type aiv]
    ```

    > [!NOTE]
    > 如果需要查看调用栈，需使用`-O2/O3 + -g`选项编译生成包含调试信息的kernel.o文件，或者生成fatbin结构的ELF文件。
    >
    > 原因：在算子执行过程中，若因指令执行导致硬件异常，硬件通常会继续执行若干条指令后再上报异常并生成core文件。因此，core文件中的内存和寄存器数据可能并不准确。不过，PC寄存器的值通常会被修正。
    >
    > 在O2/O3优化下默认inline，不需要栈内存数据，仍可以回溯准确的调用栈；而在O0优化下会强制no inline，且栈内存数据不准确，通常只有0栈帧是准确的。

4. 查看异常算子core文件信息。

    ```bash
    msdebug --core output2/extra-info/data-dump/0/xxx.core

    msdebug(MindStudio Debugger) is part of MindStudio Operator-dev Tools.
    The tool provides developers with a mechanism for debugging Ascend kernels running on actual hardware.
    This enables developers to debug Ascend kernels without being affected by potential changes brought by simulation and emulation environments.
    (msdebug) target create --core "output2/extra-info/data-dump/0/xxx.core"
    Core file '/home/xxx/coredump_test/output2/extra-info/data-dump/0/xxx.core' (hiipu64) was loaded.
    [Switching to focus on Kernel add_custom, CoreId 1, Type aiv]
    * thread #1, stop reason = MTE_ERROR
        frame #0: 0x000012c041200420 device_debugdata`::add_custom(unit8_t *__gm__, unit8_t *__gm__, unit8_t *__gm__) [inlined] void AscendC::DataCopyGM2UBImpl<short>(dst=0x0000000000000000, src=0, intriParams=<unavailable>) at kernel_operator_data_copy_impl.h:59:9
       56               __gm__ unit8_t* workSpace = GetSysWorkSpacePtr();
       57               AscendCUtils::CheckGmMemOverflowNormal(src, workSpace, true, false, intriParams);
       58           }
    -> 59           copy_gm_to_ubuf((__ubuf__ void*)dst, (__gm__ void*)src, 0, intriParams.blockCount, intriParams.blockLen,
       60               intriParams.srcStride, intriParams.dstStride);
       61       }
       62   }

    (msdebug) bt
    * thread #1, stop reason = MTE_ERROR
      * frame #0: 0x000012c041200420 device_debugdata`::add_custom(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__) [inlined] void AscendC::DataCopyGM2UBImpl<short>(dst=0x0000000000000000, src=0, intriParams=<unavailable>) at kernel_operator_data_copy_impl.h:59:9
        frame #1: 0x000012c041200420 device_debugdata`::add_custom(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__) [inlined] void AscendC::DataCopy<short>(dst=<unavailable>, src=<unavailable>, repeatParams=<unavailable>) at kernel_operator_data_copy_intf_impl.h:76:9
        frame #2: 0x000012c041200420 device_debugdata`::add_custom(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__) [inlined] void AscendC::DataCopy<short>(dst=<unavailable>, src=<unavailable>, count=128) at kernel_operator_data_copy_intf_impl.h:782:5
        frame #3: 0x000012c041200420 device_debugdata`::add_custom(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__) [inlined] KernelAdd::CopyIn(this=0x0000000000000000, progress=<unavailable>) at add_kernel.cpp:59:9
        frame #4: 0x000012c04120030c device_debugdata`::add_custom(uint8_t *__gm__, uint8_t *__gm__, uint8_t *__gm__) [inlined] KernelAdd::Process(this=0x0000000000000000) at add_kernel.cpp:47:13
        frame #5: 0x000012c04120029c device_debugdata`add_custom(x=<unavailable>, y=<unavailable>, z=<unavailable>) at add_kernel.cpp:101:8

    (msdebug) ascend info summary
      CoreId  CoreType        PC         DeviceId    ChipType
         0       AIV    0x12c041200360       0        A2/A3
     *   1       AIV    0x12c041200420       0        A2/A3
         2       AIV    0x12c041200420       0        A2/A3
         3       AIV    0x12c041200420       0        A2/A3
         4       AIV    0x12c041200420       0        A2/A3
         5       AIV    0x12c041200420       0        A2/A3
         6       AIV    0x12c041200420       0        A2/A3
        47       AIV    0x12c041200420       0        A2/A3

      Id           DataType                   MemType                     Addr                       Size             CoreId    CoreType    Dim
       0    DEVICE_KERNEL_OBJECT                GM                   0x12c041200000                 189432             NA          NA        NA
       1            STACK                    GM/DCACHE                0xff0000c8000                  32768             0          AIV        NA
       2            STACK                    GM/DCACHE                0xff0000d0000                  32768             1          AIV        NA
       3            STACK                    GM/DCACHE                0xff0000d8000                  32768             2          AIV        NA
       4            STACK                    GM/DCACHE                0xff0000e0000                  32768             3          AIV        NA
       5            STACK                    GM/DCACHE                0xff0000e8000                  32768             4          AIV        NA
       6            STACK                    GM/DCACHE                0xff0000f0000                  32768             5          AIV        NA
       7            STACK                    GM/DCACHE                0xff0000f8000                  32768             6          AIV        NA
       8            STACK                    GM/DCACHE                0xff000240000                  32768             47         AIV        NA
       9            ARGS                     GM/DCACHE               0x12c100000000                   24               NA          NA        NA

     (msdebug)

     ```

5. 请参考[核切换功能介绍](#核切换功能介绍)、[检查程序状态功能介绍](#检查程序状态功能介绍)、[内存与变量打印功能介绍](#内存与变量打印功能介绍)以及[SIMT线程切换功能介绍](#simt线程切换功能介绍)章节的内存打印相关操作定位硬件异常。
6. 调试完以后，执行`q`命令并输入Y或y结束调试。

    ```bash
    (msdebug) q
    Quitting LLDB will kill one or more processes. Do you really want to proceed: [Y/n] y
    ```
