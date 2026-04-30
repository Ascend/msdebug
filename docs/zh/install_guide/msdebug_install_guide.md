# **MindStudio Debugger安装指南**

<br>

## 1. 安装说明

msDebug工具的安装方式包括：

- 使用CANN包安装：msDebug工具完整功能已集成在CANN包中，请参考《[CANN 快速安装](https://www.hiascend.com/cann/download)》安装昇腾NPU驱动和CANN软件（包含Toolkit和ops包），并配置环境变量。
- 源码编译安装：如需使用最新代码的功能，或对源码进行修改以增强功能，可下载本仓库代码，自行编译、打包工具并完成安装，具体请参见[源码编译安装](#2-源码编译安装)。

## 2. 源码编译安装

如需使用最新代码的功能，或对源码进行修改以增强功能，可下载本仓库代码，自行编译、打包工具并完成安装。

### 2.1 环境准备

请按照以下文档进行环境配置：[《算子工具开发环境安装指导》](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。
 
编译工具要求如下：

- gcc版本应大于7.4.0。

- CMake版本应大于3.20.2，小于3.31.10。

- 安装git-lfs

### 2.2 执行编译打包

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
> 生成run包依赖pigz库，一般系统自带，```pigz --version```如果没有显示版本，可自行下载。

### 2.3 安装

#### 2.3.1 准备 run 包

安装软件包前需给run包添加可执行权限。进入run包保存路径，执行如下命令，增加可执行权限。

```shell
chmod +x mindstudio-debugger_<version>_<arch>.run
```

#### 2.3.2 安装

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
> - 如果要指定路径安装，则需添加```--install-path```，例如```./mindstudio-debugger_<version>_<arch>.run  --install-path=./test --run```，则将此run包安装到当前目录下的test目录下。
>
> - 若系统中已安装该工具的旧版本，安装过程中会提示是否替换；输入 "y" 可执行覆盖安装。

### 2.4 卸载

执行以下命令，卸载软件。

```shell
./mindstudio-debugger_<version>_<arch>.run --uninstall 
```

当回显包含以下信息时，表示软件包卸载成功。

```text
mindstudio-debugger uninstall success!
```

> [!NOTE]  
> 默认会在${HOME}/Ascend目录下卸载，如果安装时通过```--install-path```指定了安装路径，则卸载时也需添加```--install-path```，例如```./mindstudio-debugger_<version>_<arch>.run  --install-path=./test --uninstall```。

如果run包已经删除，则可执行如下命令，卸载软件。

```shell
bash $HOME/Ascend/cann/share/info/mindstudio-debugger/script/uninstall.sh   # 或bash ./xxx/share/info/mindstudio-debugger/script/uninstall.sh（指定路径安装场景）
```

### 2.5 升级

如需使用run包替换运行环境中已安装的mindstudio-debugger包，执行如下安装操作：

```shell
./mindstudio-debugger_<version>_<arch>.run --run
```

安装过程中，会提示是否替换原有安装包```do you want to overwrite current installation? [y/n]```，输入y后，安装包会自动完成升级操作。

> [!NOTE]  
> 默认会升级到${HOME}/Ascend目录下的mindstudio-debugger，如果老版本是通过指定路径安装的，则需添加```--install-path```，例如```./mindstudio-debugger_<version>_<arch>.run  --install-path=./test --run```，其中test是老版本的安装路径。
