
# **MindStudio Debugger安装指南**

## 安装说明

MindStudio Debugger（算子调试工具，msDebug）工具用于调试NPU侧运行的算子程序，为算子开发人员提供调试手段。本文主要介绍msDebug工具的安装方法。

## 安装前准备

**配置用户名和密钥**

为了避免依赖下载过程中反复输入密码，可通过如下命令配置git保存用户密码：

```shell
git config --global credential.helper store
```

**准备工具**

编译工具要求如下：

- gcc版本应大于7.4.0。
- CMake版本应大于3.20.2，小于3.31.10。

## 安装步骤

### 软件包构建

可以通过命令行和脚本两种方式进行软件包构建，具体操作如下。

- 命令行方式 
  
  1. 执行如下命令下载项目构建依赖的子仓库，并更新依赖到最新代码：

      ```shell
      python download_dependencies.py
      ```

  2. 执行如下命令，构建软件包。

      ```shell
      mkdir build
      cd build
      cmake -G Ninja .. && ninja
      ```
 
- 一键式脚本方式

  执行如下命令，通过一键式脚本构建软件包。

    ```shell
    python build.py
    ```

    > [!NOTE]  说明 
    > 
    > 如果本地更改了依赖子仓库中的代码，不想构建过程中执行更新动作，可以执行```python build.py local```。

当回显包含以下信息时，表示软件包构建成功，生成run包。构建成功的run包默认保存在output目录下。

其中`<version>`表示版本号，`<arch>`表示CPU架构。

```text
"Ascend-mindstudio-debugger-<version>_linux-<arch>.run" successfully created.
```

### 安装软件包 

1. 安装软件包前需给run包添加可执行权限。进入run包保存路径，执行如下命令，增加可执行权限。

    ```shell
    chmod +x Ascend-mindstudio-debugger-<version>_linux-<arch>.run
    ```

2. 将run包拷贝到运行环境中，执行以下命令安装。

    ```shell
    ./Ascend-mindstudio-debugger-<version>_linux-<arch>.run --run   
    ```

    当回显包含以下信息时，表示软件包安装成功。

    ```text
    mindstudio-debugger package install success!
    ```

> [!NOTE]  说明 
> 
> - 如果环境中配置过ASCEND_HOME_PATH环境变量，则会安装到${ASCEND_HOME_PATH}目录下；否则会默认安装到${HOME}/Ascend目录下。
> 
> - 如果要指定路径安装，则需添加```--install-path```，例如```./Ascend-mindstudio-debugger-<version>_linux-<arch>.run  --install-path=./test --run```，则将此run包安装到当前目录下的test目录下。

## 安装后配置

软件包安装成功后，需设置环境变量，确保算子工具可以正常运行。

```shell
export ASCEND_HOME_PATH=$HOME/Ascend    # 或export ASCEND_HOME_PATH=$PWD/xxx（指定路径安装场景）
export PATH=$ASCEND_HOME_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/lib64:$LD_LIBRARY_PATH
```

## 升级

如需使用run包替换运行环境中已安装的mindstudio-debugger包，执行如下安装操作：

```shell
./Ascend-mindstudio-debugger-<version>_linux-<arch>.run --run 
```  

安装过程中，会提示是否替换原有安装包```do you want to overwrite current installation? [y/n]```，输入y后，安装包会自动完成升级操作。

> [!NOTE]  说明  
> 默认会升级到${HOME}/Ascend目录下的mindstudio-debugger，如果老版本是通过指定路径安装的，则需添加```--install-path```，例如```./Ascend-mindstudio-debugger-<version>_linux-<arch>.run  --install-path=./test --run```，其中test是老版本的安装路径。

## 卸载

执行以下命令，卸载软件。

```shell
./Ascend-mindstudio-debugger-<version>_linux-<arch>.run --uninstall 
```

当回显包含以下信息时，表示软件包卸载成功。

```text
mindstudio-debugger uninstall success!
```

> [!NOTE]  说明  
> 默认会在${HOME}/Ascend目录下卸载，如果安装时通过```--install-path```指定了安装路径，则卸载时也需添加```--install-path```，例如```./Ascend-mindstudio-debugger-<version>_linux-<arch>.run  --install-path=./test --uninstall```。

如果run包已经删除，则可执行如下命令，卸载软件。

```shell
bash $HOME/Ascend/share/info/mindstudio-debugger/script/uninstall.sh   # 或bash ./xxx/share/info/mindstudio-debugger/script/uninstall.sh（指定路径安装场景）
```
