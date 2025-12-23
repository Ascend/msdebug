
# **MindStudio Debugger安装指南**

## 安装说明

MindStudio Debugger（算子调试工具，msDebug）工具用于采集和分析运行在昇腾AI处理器上算子的关键性能指标，本文主要介绍msDebug工具的安装方法。  

## 安装前准备

**配置用户名和密钥**

为了确保代码能够下载成功，需提前在环境中配置git仓库的用户名和密码。  

执行以下命令，配置git存储用户密码，并通过git submodule下载.gitmodules中的子仓库，在下载过程中可能会提示需要输入用户名和密码，输入后git会记住授权信息，后续就不会再需要输入用户名和密码了。  

```
git config --global credential.helper store
git submodule update --init --recursive --depth=1
```

**准备工具**

编译工具要求如下：  
- gcc版本应大于7.4.0。
- CMake版本应大于3.20.2，小于3.31.10。

## 安装步骤

### 软件包构建

- 命令行方式  
  执行如下命令，构建软件包。
    ```
    mkdir build
    cd build
    cmake -G Ninja .. && ninja
    ```
 
- 一键式脚本方式  
  通过一键式脚本构建软件包。  
    ```
    python build.py
    ```
    [!NOTE]  说明  
    如果本地更改了依赖子仓库中的代码，不想构建过程中执行更新动作，可以执行```python build.py local```。

### 安装软件包  

1. 软件包构建成功后，生成的是run包。安装前需给run包添加可执行权限。

    ```
    chmod +x Ascend-mindstudio-debugger-*.run
    ```

2. run包默认保存在output目录下，需将run包拷贝到运行环境中，执行以下命令安装。

    ```
    ./Ascend-mindstudio-debugger-*.run --run   
    ```

[!NOTE]  说明  
如果环境中配置过ASCEND_HOME_PATH环境变量，则会安装到\${ASCEND_HOME_PATH}目录下；否则会默认安装到${HOME}/Ascend目录下。  
如果要指定路径安装，则需添加```--install-path```，例如```./Ascend-mindstudio-debugger-*.run  --install-path=./test --run```，则将此run包安装到当前目录下的test目录下。

## 安装后配置

软件包安装成功后，需设置环境变量，确保算子工具可以正常运行。

```
export ASCEND_HOME_PATH=$HOME/Ascend  # 或export ASCEND_HOME_PATH=$PWD/xxx（指定路径安装场景）
export PATH=$ASCEND_HOME_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/lib64:$LD_LIBRARY_PATH
```

## 升级

如需使用run包替换运行环境中已安装的mindstudio-debugger包，执行如下安装操作：

```
./Ascend-mindstudio-debugger-*.run --run 
```  

[!NOTE]  说明  
    - 默认会升级到${HOME}/Ascend目录下的mindstudio-debugger，如果老版本是通过指定路径安装的，则需添加```--install-path```，例如```./Ascend-mindstudio-debugger-*.run  --install-path=./test --run```，其中test是老版本的安装路径。  
    - 安装过程中，会提示是否替换原有安装包```do you want to overwrite current installation? [y/n]```，输入y后，安装包会自动完成升级操作。

## 卸载

执行以下命令，卸载软件。

```
./Ascend-mindstudio-debugger-*.run --uninstall 
```

[!NOTE]  说明  
    默认会在${HOME}/Ascend目录下卸载，如果安装时通过```--install-path```指定了安装路径，则卸载时也需添加```--install-path```，例如```./Ascend-mindstudio-debugger-*.run  --install-path=./test --uninstall```。

如果run包已经删除，则可执行如下命令，卸载软件。

```
bash $HOME/Ascend/share/info/mindstudio-debugger/script/uninstall.sh   # 或bash ./xxx/share/info/mindstudio-debugger/script/uninstall.sh（指定路径安装场景）
```