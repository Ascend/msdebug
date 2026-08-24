<h1 align="center">MindStudio Debugger</h1>

<div align="center">
<p><b><span style="font-size:24px;">MindStudio Debugger for Ascend AI Operators</span></b></p>

 [![Quick Start](https://badgen.net/badge/快速入门/QuickStart/blue)](./docs/en/quick_start/msdebug_quick_start.md)
 [![AI Q&A (DeepWiki)](https://badgen.net/badge/AI问答/DeepWiki/blue)](https://deepwiki.com/mindstudio-docs/master)
 [![AI Q&A (ZRead)](https://badgen.net/badge/AI问答/ZRead/blue)](https://zread.ai/mindstudio-docs/master)
 [![Precise Search](https://badgen.net/badge/精确搜索/ReadTheDocs/blue)](https://mindstudio-operator-tools-docs.readthedocs.io/zh-cn/latest/)
 [![Ascend Community](https://badgen.net/badge/昇腾社区/Community/blue)](https://www.hiascend.com/cn/developer/software/mindstudio)
 [![Report Issues](https://badgen.net/badge/报告问题/Issues/blue)](https://gitcode.com/Ascend/msdebug/issues)

</div>

English | [简体中文](README.md)

## ✨ Latest Updates

<span style="font-size:14px;">

🔹 **[Dec 31, 2025]**: MindStudio Debugger is fully open-sourced.

</span>

## ️ ℹ️ Overview

MindStudio Debugger (msDebug) is an operator debugging tool built on the LLVM compiler infrastructure for Ascend devices. It is used to debug operator programs running on the NPU and provides developers with key debugging capabilities, including reading the memory and registers of Ascend devices, and pausing and resuming the program execution.

<div align="center">
  <h4>▶️ Quick demo</h4>
  <img src="./docs/en/figures/demo-msdebug.gif" alt="Quick demo" width="600">
  <p><sup>Figure: Demonstrates operations such as setting breakpoints for board debugging, printing variables, and step-by-step debugging of operators.</sup></p>
</div>

## ⚙️ Functions

msDebug can debug all Ascend operators, including Ascend C operators (Vector, Cube, and Mix fused operators). You can select the operators as required. The following functions are supported:

|Function|Description|
|:------|:---------|
|**Breakpoint setting**|You can set a line breakpoint on the running program of an operator, that is, set a breakpoint on a specific line in the operator code file.|
|**Variable and memory printing**|Based on the variable type and usage, a variable can be stored in a register or in the local memory or global memory. You can print the address of a variable to find its storage location and further print the associated memory.|
|**Step-by-step debugging**|You can perform step-by-step debugging to learn about the code execution details.|
|**Interrupting execution**|When the operator execution program freezes, manually interrupt the operator execution program and display the interrupted location information.|
|**Core switching**|Switch the current core to the specified core. After the core is switched, the position of the code interruption of the specified core is automatically displayed.|
|**Program status checking**|After an operator is called, you can read the register values of the device where the current breakpoint is located to check the program status.|
|**Debugging information displaying**|Query information about the device where the operator runs.|
|**Core dump file parsing**|By parsing the dump files of abnormal operators, you can collect sufficient data for problem analysis even without a stress test.|

## 🌌 Intelligent Search

To improve documentation search efficiency, we provide multiple efficient search methods:  
🔹 [AI Q&A (DeepWiki)](https://deepwiki.com/mindstudio-docs/master): Natural language Q&A that helps you quickly grasp the project architecture and module relationships.  
🔹 [AI Q&A (ZRead)](https://zread.ai/mindstudio-docs/master): Provides a better Chinese Q&A experience and precisely locates feature usage and details.  
🔹 [Precise Search (ReadTheDocs)](https://mindstudio-operator-tools-docs.readthedocs.io/zh-cn/latest/): Keyword-based full-text search that takes you directly to interfaces, parameters, and error messages.

## 🚀 Quick Start

For details, see [msDebug Quick Start](docs/en/quick_start/msdebug_quick_start.md).

## 📦 Installation Guide

For details about the environment dependencies and installation methods of the tool, see [msDebug Installation Guide](docs/en/install_guide/msdebug_install_guide.md).

## 📘 User Guide

For details about how to use the tool, see [msDebug User Guide](docs/en/user_guide/msdebug_user_guide.md).

## 💡 Typical Cases

For details about how to use the tool in typical scenarios, see [msDebug Typical Cases](docs/en/best_practices/msdebug_basic_cases.md).

## ❓ FAQs

For details about frequently asked questions and solutions, see [msDebug FAQs](docs/en/support/msdebug_faq.md).

## 🛠️ Contribution Guide

You are welcome to contribute to the project. For details, see [Contribution Guide](./docs/en/contributing/contributing_guide.md). 

## ⚖️ Related Information

🔹 [Release Notes](https://gitcode.com/Ascend/msdebug/releases)  
🔹 [License Notice](./docs/en/legal/license_notice.md)  
🔹 [Security Statement](./docs/en/legal/security_statement.md)  
🔹 [Disclaimer](./docs/en/legal/disclaimer.md) 

## 🤝 Suggestions and Communication

You are welcome to contribute to the community. If you have any questions or suggestions, please submit an [issue](https://gitcode.com/Ascend/msdebug/issues). We will reply as soon as possible. Thank you for your support.

|Instant Interaction (WeChat Group)|Official Information (WeChat Official Account)|In-Depth Support (Assistant/Forum)|
|:---:|:---:|:---|
| <img src="https://raw.gitcode.com/Ascend/docs/files/master/common/Writing_Template/figures/qr_code_wechat_work.png" width="120"><br><sub>*Scan the QR code to join the technical communication group.*</sub> | <img src="https://raw.gitcode.com/Ascend/docs/files/master/common/Writing_Template/figures/qr_code_wechat_official_account.png" width="120"><br><sub>*Scan the QR code to follow the official WeChat account.*</sub> | Scan the QR code to join the group and follow the official account to access the fastest communication platform for MindStudio users and developers:<br> **Quick questions:** Discuss technical issues with community members in real time.<br>**Stay informed:** Receive version release and feature update notifications as soon as they are published.<br> **Share experience:** Exchange best practices and hands-on insights with a wide range of developers.  <br> <br> **More support channels:** 👉 Ascend Assistant: [![WeChat](https://img.shields.io/badge/WeChat-07C160?style=flat-square&logo=wechat&logoColor=white)](https://gitcode.com/Ascend/msit/blob/master/docs/zh/figures/readme/xiaozhushou.png) 👉 Ascend Forum: [![Website](https://img.shields.io/badge/Website-%231e37ff?style=flat-square&logo=RSS&logoColor=white)](https://www.hiascend.com/forum/) |

## 🙏 Acknowledgements

This tool is jointly developed by the following Huawei departments:  
🔹 Ascend Computing MindStudio Development Department  
🔹 Ascend Computing Ecosystem Enablement Department  
🔹 Huawei Cloud AI Compute Service  
🔹 Compiler Technologies Lab, 2012 Labs  
🔹 Markov Lab, 2012 Labs  
Thank you to everyone in the community for your PRs. We warmly welcome your contributions.
