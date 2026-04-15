# MindStudio Debugger Development environment construction, compilation and UT guide

<br>

## 1. Necessary knowledge

Please refer to [《msDebug Architecture design document》](./architecture.md) Introduction to learning code framework and core process。

## 2. Development environment preparation

Please follow documents to configure the environment：[《Operator tool development environment installation guide》](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

## 3. Compile and package

It is divided into the following two methods, the advantages and disadvantages are as follows：

| Method | Applicable scenarios | Advantages | Disadvantages |
|------|---------|------|------|
| One-click script | first build, CI/CD pipeline | zero configuration, one step in place | Cannot perform a certain step separately |
| Step-by-step script | daily development, incremental compilation | flexible and efficient | Requires multi-step operation |

### 3.1 Method 1: One-click script

```shell
python build.py
```

### 3.2 Method two: Step-by-step script

#### 3.2.1 Download dependency

```shell
python download_dependencies.py
```

#### 3.2.2 Compile and package

##### 3.2.2.1 Start compilation

Execute the following command to start the compilation：

```shell
mkdir build
cd build
cmake -G Ninja .. && ninja
```

if `output` Under the directory `mindstudio-debugger_<version>_<arch>.run --run` The generation time of the file has been updated to the current compilation time, which indicates that the compilation and packaging have been successfully completed.

##### 3.2.2.2 Description of compilation results

The compilation result is generated to output Under the table of contents：

```text
output/
|-- mindstudio-debugger_<version>_<arch>.run --run       # Installation package
|-- bin                                                  # Executable bin file, you can directly call the debugging function
|-- lib64                                                # Various dynamic and static libraries
|-- scene.info                                           # Environmental information
|-- version.info                                         # Version information
```

#### 3.2.3 Clean up/recompile

Delete the build directory and re-execute[3.2.2.1](#3221 - Start compilation)：

```shell
rm -rf build
```

## 4. Perform UT test

```shell
python build.py test
```

## 5. FAQ

### 5.1 Why didn't the run package be generated when make was executed at compile time?

It is possible to execute cmake Time to use `cmake ..`，cmake The command description is as follows:

- `cmake ..`：Only compile this project，make install Will put things in output/，But will not call makeself，So it will not be generated `mindstudio-debugger_<version>_<arch>.run`。
- `cmake ../cmake`：Used is cmake/CMakeLists.txt The "super build" will be compiled + installed first, and then executed parser.py + makeself，in output/ Generated in .run package。
