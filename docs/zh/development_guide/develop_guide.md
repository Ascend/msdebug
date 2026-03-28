# MindStudio Debugger 开发环境搭建及编译和UT方法

<br>

## 1. 预备知识

请参考[《msDebug架构设计文档》](./architecture.md)学习代码框架及核心流程介绍。

## 2. 开发环境准备

请按照以下文档进行环境配置：[《算子工具开发环境安装指导》](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

## 3. 编译打包

分为如下两种方式，优缺点如下：

| 方法 | 适用场景 | 优点 | 缺点 |
|------|---------|------|------|
| 一键式脚本 | 首次构建、CI/CD 流水线 | 零配置，一步到位 | 无法单独执行某一步骤 |
| 分步骤脚本 | 日常开发、增量编译 | 灵活，效率高 | 需要多步操作 |

### 3.1 方法一：一键式脚本

```shell
python build.py
```

### 3.2 方法二：分步骤脚本

#### 3.2.1 下载依赖

```shell
python download_dependencies.py
```

#### 3.2.2 编译打包

##### 3.2.2.1 启动编译

执行如下命令启动编译：

```shell
mkdir build
cd build
cmake -G Ninja .. && ninja
```

若 `output` 目录下的 `mindstudio-debugger_<version>_<arch>.run --run` 文件的生成时间已更新为当前编译时间，则表明编译与打包已成功完成。

##### 3.2.2.2 编译结果说明

编译结果生成到 output 目录下：

```text
output/
|-- mindstudio-debugger_<version>_<arch>.run --run       # 安装包
|-- bin                                                  # 可执行bin文件，可直接调用调试功能
|-- lib64                                                # 各种动态库和静态库
|-- scene.info                                           # 环境信息
|-- version.info                                         # 版本信息
```

#### 3.2.3 清理/重新编译

删除构建目录，重新执行[第 3.2.2.1 节](#3221-启动编译)：

```shell
rm -rf build
```

## 4. 执行UT测试

```shell
python build.py test
```

## 5. FAQ

### 5.1 编译时执行make为什么没有生成run包？

有可能执行 cmake 时用了 `cmake ..`，cmake 命令说明如下:

- `cmake ..`：只编译本工程，make install 会把东西装到 output/，但不会调用 makeself，所以不会生成 `mindstudio-debugger_<version>_<arch>.run`。
- `cmake ../cmake`：用的是 cmake/CMakeLists.txt 的「超级构建」，会先编译+安装，再执行 parser.py + makeself，在 output/ 里生成 .run 包。
