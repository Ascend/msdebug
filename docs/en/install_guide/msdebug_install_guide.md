# **MindStudio Debugger Installation Guide**

<br>

## 1. Binary Installation

The MindStudio toolchain is integrated into the CANN package for release. msDebug is stored in the `{install_cann_path}/cann/tools/msdebug` directory. You can install the CANN package in either of the following ways:

### Method 1: Install the software according to the CANN official document

For details, see <a href="https://www.hiascend.com/document/detail/zh/canncommercial/850/softwareinst" target="_blank">CANN Installation Guide</a>.
Perform the installation and configuration step by step according to the document.

### Method 2: Use the official CANN container image

Visit <a href="https://www.hiascend.com/developer/ascendhub/detail/17da20d1c2b6493cb38765adeba85884" target="_blank">CANN official image repository</a>.
Pull the image and start the container according to the instructions in the repository.

<br>

## 2. Source Code Installation

To use the functions of the latest code or modify the source code to enhance functions, you can download the code from this repository, build and package the tool, and install it.

### 2.1 Environment Setup

Set up the environment by referring to the [Operator Tool Development Environment Setup Guide](https://gitcode.com/Ascend/msot/blob/master/docs/en/common/dev_env_setup.md).

The requirements for compilation tools are as follows:

- The GCC version must be later than 7.4.0.

- The CMake version must be 3.20.2 or later.

- Git LFS is installed.

### 2.2 Building and Packaging

Run the one-click script to automatically download and build the dependency repository:

```shell
python build.py
```

> [!NOTE]NOTE
>
> If you have modified the code in the local dependency sub-repositories and wish to skip the update process during the build, run `python build.py local`.

When the following information is displayed, the package is built and the .run package is generated.

```text
"mindstudio-debugger_<version>_<arch>.run" successfully created.
```

By default, the built .run package is saved in the `output` directory. In the file name, `<version>` indicates the version number and `<arch>` indicates the CPU architecture.

> [!NOTE]NOTE
>
> The generation of the .runfile depends on the pigz library, which is typically provided by the system. If no version is displayed in `pigz --version`, download it.

### 2.3 Installation and Uninstallation

#### 2.3.1 Preparing the .run Package

Before installation, grant the execute permission to the .run package. Go to the directory where the .run package is saved and run the following command to add the execute permission:

```shell
chmod +x mindstudio-debugger_<version>_<arch>.run
```

#### 2.3.2 Installation

Copy the .run package to the operating environment and run the following command to install it:

```shell
./mindstudio-debugger_<version>_<arch>.run --run
```

When the following information is displayed, the software package is successfully installed:

```text
mindstudio-debugger package install success!
```

> [!NOTE]NOTE
>
> - If the `ASCEND_HOME_PATH` environment variable is configured, the software package will be installed to `${ASCEND_HOME_PATH}`. Otherwise, it will be installed to `${HOME}/Ascend`.
>
> - To specify a custom installation path, use the `--install-path` option. For example: `./mindstudio-debugger_<version>_<arch>.run  --install-path=./test --run` installs the .run package to the `test` directory in the current directory.
>
> - If an earlier version of the tool has been installed in the system, a message will be displayed during the installation asking you whether to replace it. Enter "y" to perform an overwrite installation.

#### 2.3.3 Uninstallation

Run the following command to uninstall the software:

```shell
./mindstudio-debugger_<version>_<arch>.run --uninstall
```

When the following information is displayed, the software package is successfully uninstalled:

```text
mindstudio-debugger uninstall success!
```

> [!NOTE]NOTE
> By default, the uninstallation targets the directory in `${HOME}/Ascend`. If the package is installed using the `--install-path` option, specify the same path during uninstallation, for example, `./mindstudio-debugger_<version>_<arch>.run  --install-path=./test --uninstall`.

If the .run package has been deleted, run the following command to uninstall the software:

```shell
bash $HOME/Ascend/share/info/mindstudio-debugger/script/uninstall.sh   # For custom installation paths, run "bash ./xxx/share/info/mindstudio-debugger/script/uninstall.sh".
```

#### 2.3.4 Upgrade

To replace the installed mindstudio-debugger package with the .run package, run the following command:

```shell
./mindstudio-debugger_<version>_<arch>.run --run
```

During the process, you will be prompted `do you want to overwrite current installation? [y/n]` Enter `y` to proceed with the automatic upgrade.

> [!NOTE]NOTE
> By default, the upgrade targets the `mindstudio-debugger` directory in `${HOME}/Ascend`. If the previous version is installed to a custom path, use the `--install-path` option, for example, `./mindstudio-debugger_<version>_<arch>.run  --install-path=./test --run`, where `test` is the previous installation directory.
