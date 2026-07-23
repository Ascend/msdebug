# **MindStudio Debugger安装指南**

<br>

## 1. 安装说明

本工具已集成于CANN中，若已安装CANN且无需更新此工具，可直接使用，无需按本文档安装。

若您的环境尚未安装CANN，请参见《[CANN 快速安装](https://www.hiascend.com/cann/download)》安装昇腾NPU驱动和CANN软件（包含Toolkit和ops包），并配置环境变量。

如需单独升级本工具或使用最新版本，您可通过以下三种方式进行安装：[在线安装](#21-在线安装)、[离线安装](#22-离线安装)、[源码安装](#23-源码安装)。

## 2. 安装方式

### 2.1 在线安装

若您的设备具备互联网访问能力，可通过一条命令自动完成工具的下载与安装。请参见昇腾社区MindStudio[下载](https://www.hiascend.com/developer/software/mindstudio/download)页面，选择对应的CANN版本，并在安装方式中选择“在线安装”，系统将引导您完成后续操作。

### 2.2 离线安装

对处于企业内网等无外网环境的设备，请先在可联网的机器上下载完整的离线安装包，再将其传输至目标设备进行安装。请参见昇腾社区MindStudio[下载](https://www.hiascend.com/developer/software/mindstudio/download)页面，选择对应的CANN版本，并在安装方式中选择“离线安装”，获取对应的安装包及操作指引。

### 2.3 源码安装

如需使用最新代码的功能，或对源码进行修改以增强功能，可下载本仓库代码，自行编译、打包工具并完成安装。

#### 2.3.1 环境准备

请按照以下文档进行环境配置：[《算子工具开发环境安装指导》](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

编译工具要求如下：

- gcc版本应大于7.4.0。

- CMake版本应大于等于3.20.2。

- 安装git-lfs

#### 2.3.2 执行编译打包

- 克隆本仓库

    ```sh
    git clone https://gitcode.com/Ascend/msdebug.git
    ```

- 构建打包

    通过一键式脚本自动完成依赖仓库的下载与构建流程：

    ```shell
    cd msdebug
    python build.py
    ```

> [!NOTE]
>
> 如果本地更改了依赖子仓库中的代码，不想构建过程中执行更新动作，可以执行`python build.py local`。

当回显包含以下信息时，表示软件包构建成功，生成run包：

```text
"mindstudio-debugger_<version>_<arch>.run" successfully created.
```

构建成功的run包默认保存在output目录下。其中`<version>`表示版本号，`<arch>`表示CPU架构。

> [!NOTE]  说明
>
> 生成run包依赖pigz库，一般系统自带，`pigz --version`如果没有显示版本，可自行下载。

#### 2.3.3 安装

##### 2.3.3.1 准备 run 包

安装软件包前需给run包添加可执行权限。进入run包保存路径，执行如下命令，增加可执行权限。

```shell
chmod +x mindstudio-debugger_<version>_<arch>.run
```

##### 2.3.3.2 安装

将run包拷贝到运行环境中，执行以下命令安装。

```shell
./mindstudio-debugger_<version>_<arch>.run --run
```

当回显包含以下信息时，表示软件包安装成功。

```text
mindstudio-debugger package install success!
```

> [!NOTE]
>
> - 如果环境中配置过`ASCEND_HOME_PATH`环境变量，则会安装到`${ASCEND_HOME_PATH}`目录下；否则会默认安装到`${HOME}/Ascend`目录下。
>
> - 如果要指定路径安装，则需添加`--install-path`，例如`./mindstudio-debugger_<version>_<arch>.run  --install-path=./test --run`，则将此run包安装到当前目录下的test目录下。
>
> - 若系统中已安装该工具的旧版本，安装过程中会提示是否替换；输入 "y" 可执行覆盖安装。

## 3. 验证安装

安装完成后，执行以下命令验证工具是否安装成功：

```shell
msdebug --help
```

若输出不报错，且能显示帮助信息，则表明安装成功。

## 4. 卸载

可通过如下步骤卸载：

1. 下载脚本。

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.1.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - 需要联网环境才能下载，若环境不允许联网或处于离线状态，请先在可联网的环境下载该脚本后拷贝到目标设备。
   > - 若执行命令无响应或出现连接失败、SSL证书错误等问题，请参见[FAQ](https://www.hiascend.com/developer/blog/details/02176213671719317003)。

2. 执行卸载。

   ```bash
   python ms_install.py uninstall {tools_name}
   ```

   其中{tools_name}配置为需卸载的工具名称，可通过`python ms_install.py help`命令查询，在打印信息中的Available Tools字段下显示工具名称。

   卸载成功打印如下信息：

   ```text
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 5. 升级

升级即“先卸后装”。直接执行安装命令，工具将自动卸载旧版本，并引导您完成覆盖安装。
