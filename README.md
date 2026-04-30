<h1 align="center">MindStudio Debugger</h1>

<div align="center">
<h2>昇腾 AI 算子调试工具</h2>
  
 [![Ascend](https://img.shields.io/badge/Community-MindStudio-blue.svg)](https://www.hiascend.com/developer/software/mindstudio) 
 [![License](https://badgen.net/badge/License/MulanPSL-2.0/blue)](./LICENSE)

</div>

## ✨ 最新消息

<span style="font-size:14px;">

🔹 **[2025.12.31]**：MindStudio Debugger 项目全面开源

</span>

## ️ ℹ️ 简介

MindStudio Debugger（算子调试工具，msDebug）是基于 LLVM 编译器基础设施构建、面向昇腾设备的算子调试工具，用于调试在 NPU 侧运行的算子程序，为开发者提供关键调试能力，包括读取昇腾设备内存与寄存器、暂停与恢复程序执行状态等。

<div align="center">
  <h4>▶️ 核心能力快速演示</h4>
  <img src="./docs/zh/figures/demo-msdebug.gif" alt="快速演示" width="600">
  <p><sup>图示：算子上板调试断点设置、变量打印、单步调试等操作演示</sup></p>
</div>

## ⚙️ 功能介绍

msDebug工具支持调试所有的昇腾算子，包含Ascend C算子（Vector、Cube以及Mix融合算子）程序，用户可根据实际情况进行选择，支持的功能如下：

|功能名称|功能描述|
|:------|:---------|
|**断点设置**|可在算子的运行程序上设置行断点，即在算子代码文件的特定行号上设置断点。|
|**打印变量和内存**|根据变量类型和用法，变量可以存储在寄存器中或存储在Local Memory、Global Memory内存中，用户可以打印变量的地址以找出它的存储位置并进一步打印关联的内存。|
|**单步调试**|需要了解代码执行具体情况时，可进行单步调试。|
|**中断运行**|当算子运行程序卡顿时，手动中断算子运行程序并回显中断位置信息。|
|**核切换**|可将当前聚焦的核切换至指定的核，切核后会自动展示指定核代码中断处的位置。|
|**检查程序状态**|当调起算子后，可读取当前断点所在设备的寄存器值，检查程序状态。|
|**调试信息展示**|查询算子运行的设备信息。|
|**解析Core dump文件**|通过对异常算子dump文件的解析，即使在没有主动压测的情况下也能收集到足够的数据用于问题分析。|

## 🚀 快速入门

详细操作步骤请参见《[msDebug 快速入门](docs/zh/quick_start/msdebug_quick_start.md)》。

## 📦 安装指南

介绍工具的环境依赖与安装方法，请参见《[msDebug 安装指南](docs/zh/install_guide/msdebug_install_guide.md)》。

## 📘 使用指南

工具的详细使用方法，请参见《[msDebug 使用指南](docs/zh/user_guide/msdebug_user_guide.md)》。

## 💡 典型案例

通过典型问题场景帮助用户理解并掌握工具使用，请参见《[msDebug 典型案例](docs/zh/best_practices/msdebug_basic_cases.md)》。

## ❓ FAQ

常见问题及解决方案，请参见《[msDebug FAQ](docs/zh/support/msdebug_faq.md)》。

## 🛠️ 贡献指南

欢迎参与项目贡献，请参见《[贡献指南](./docs/zh/contributing/contributing_guide.md)》。  

## ⚖️ 相关说明

🔹《[版本说明](./docs/zh/release_notes/release_notes.md)》  
🔹《[许可证声明](./docs/zh/legal/license_notice.md)》  
🔹《[安全声明](./docs/zh/legal/security_statement.md)》  
🔹《[免责声明](./docs/zh/legal/disclaimer.md)》   
 
## 🤝 建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交[Issues](https://gitcode.com/Ascend/msdebug/issues)，我们会尽快回复。感谢您的支持。

|                                      📱 关注 MindStudio 公众号                                       | 💬 更多交流与支持                                                                                                                                                                                                                                                                                                                                                                                                                     |
|:-----------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| <img src="https://gitcode.com/Ascend/msot/blob/master/docs/zh/figures/readme/officialAccount.png" width="120"><br><sub>*扫码关注获取最新动态*</sub> | 💡 **加入微信交流群**：<br>关注公众号，回复“交流群”即可获取入群二维码。<br><br>🛠️ **其他渠道**：<br>👉 昇腾助手：[![WeChat](https://img.shields.io/badge/WeChat-07C160?style=flat-square&logo=wechat&logoColor=white)](https://gitcode.com/Ascend/msot/blob/master/docs/zh/figures/readme/xiaozhushou.png)<br>👉 昇腾论坛：[![Website](https://img.shields.io/badge/Website-%231e37ff?style=flat-square&logo=RSS&logoColor=white)](https://www.hiascend.com/forum/) |

## 🙏 致谢

本工具由华为公司的下列部门联合贡献：    
🔹 昇腾计算MindStudio开发部  
🔹 昇腾计算生态使能部  
🔹 华为云昇腾云服务  
🔹 2012编译器实验室  
🔹 2012马尔科夫实验室  
感谢来自社区的每一个 PR，欢迎贡献！
