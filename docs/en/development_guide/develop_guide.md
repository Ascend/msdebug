# MindStudio Debugger Development Environment Setup, Build, and UT Methods

<br>

## 1. Background Knowledge Required

For details about the code framework and core process, see the [msDebug Architecture Design Specifications](./architecture.md).

## 2. Development Environment Setup

Set up the environment by referring to the [Operator Tool Development Environment Setup Guide](https://gitcode.com/Ascend/msot/blob/master/docs/en/common/dev_env_setup.md).

## 3. Building and Packaging

There are two methods, each with its own advantages and disadvantages:

| Method| Application Scenario| Advantages| Disadvantages|
|------|---------|------|------|
| One-click script| Initial build and CI/CD pipeline| Zero configuration, one-step setup| Steps cannot be executed independently.|
| Step-by-step script| Routine development and incremental build| Flexibility and efficiency| Multiple steps are required.|

### 3.1 Method 1: One-click Script

```shell
python build.py
```

### 3.2 Method 2: Step-by-Step Script

#### 3.2.1 Downloading Dependencies

```shell
python download_dependencies.py
```

#### 3.2.2 Building and Packaging

##### 3.2.2.1 Starting Build

Run the following commands to start the build:

```shell
mkdir build
cd build
cmake -G Ninja .. && ninja
```

If the generation time of the `mindstudio-debugger_<version>_<arch>.run --run` file in the `output` directory is updated to the current build time, the building and packaging are successful.

##### 3.2.2.2 Build Result Description

The build result is generated in the `output` directory:

```text
output/
|-- mindstudio-debugger_<version>_<arch>.run --run       # Installation package
|-- bin                                                  # Executable bin file, which can directly call the debugging function
|-- lib64                                                # Various dynamic and static libraries
|-- scene.info                                           # Environment information
|-- version.info                                         # Version information
```

#### 3.2.3 Cleanup/Rebuild

Delete the build directory and perform [3.2.2.1](#3221-starting-build).

```shell
rm -rf build
```

## 4. Running UT

```shell
python build.py test
```

## 5. FAQ

### 5.1 Why Is No .run Package Generated When I Run the make Command During Building?

It is possible that `cmake ..` is used when running the `cmake` command. The `cmake` command is described as follows:

- `cmake ..`: Only the current project is built. The `make install` command installs the project to the `output/` directory, but does not call `makeself`. Therefore, no `Ascend-mindstudio-sanitizer-xxx .run` file is generated.
- `cmake ../cmake`: The "super build" of `cmake/CMakeLists.txt` is used. The project is built and installed first, and then `parser.py` and `makeself` are executed to generate the .run file in the `output/` directory.
