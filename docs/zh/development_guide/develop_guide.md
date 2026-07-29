# msDebug 开发指南

<br>

本文档介绍 msDebug 的开发环境搭建、编译构建及 UT 单元测试方法。

## 1. 前置条件

### 1.1 知识准备

开发前请先阅读《[MindStudio Debugger 架构设计文档](./architecture.md)》，了解代码框架及核心流程。

### 1.2 环境准备

按照《[MindStudio工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)》完成开发环境的搭建。

> **说明：** 环境镜像的构建方法及配套软件版本由 MindStudio 统一镜像制作指南维护，本仓库不重复定义。

## 2. 编译构建

提供两种构建方式，按需选择：

| 方式 | 适用场景 | 特点 |
|------|---------|------|
| 一键式脚本 | 首次构建、CI/CD 流水线 | 零配置，一步完成全流程 |
| 分步骤构建 | 日常开发、增量编译 | 可按需执行单步，效率更高 |

### 2.1 一键式脚本

在项目根目录执行：

```bash
python3 build.py
```

脚本将自动完成下载依赖、编译、打包全流程。

### 2.2 分步骤构建

#### 下载依赖

```bash
python3 download_dependencies.py
```

#### 执行编译

```bash
python3 build.py local
```

#### 构建产物

构建成功后，输出将生成在 `artifacts` 目录下。

#### 清理构建

如需全量重新编译，删除构建目录后重新执行上述步骤：

```bash
rm -rf build
```

## 3. 单元测试

### 3.1 执行测试

```bash
python3 build.py test
```

命令返回码为 0，且测试用例均无失败，表示单元测试通过。若存在 `FAILED` 用例，请根据日志定位失败原因。

### 3.2 清理测试环境

删除 UT 构建目录后重新执行测试：

```bash
rm -rf build_ut
```
