# **MindStudio Debugger Installation Guide**

<br>

## 1. Installation Description

This tool has been integrated into CANN. If CANN has been installed and this tool does not need to be updated, you can directly use it without following the instructions in this document.

If CANN has not been installed in your environment, install the Ascend NPU driver and CANN software (including the Toolkit and ops) by referring to [CANN Quick Installation](https://www.hiascend.com/cann/download), and configure environment variables.

If you need to upgrade this tool separately or use the latest version, you can install it in any of the following ways: [Online Installation](#21-online-installation), [Offline Installation](#22-offline-installation), and [Source Installation](#23-source-installation).

## 2. Installation Methods

### 2.1 Online Installation

If your device has Internet access, you can run a single command to automatically download and install the tool. Visit the [Ascend community](https://www.hiascend.com/en/developer/software/mindstudio/download), select the target CANN version, and choose "Online" installation method. The system will guide you through the subsequent operations.

### 2.2 Offline Installation

For devices that are not connected to the Internet, such as those on an enterprise intranet, download the complete offline installation package on a device that has Internet access and then transfer the package to the target device for installation. Visit the [Ascend community](https://www.hiascend.com/en/developer/software/mindstudio/download), select the target CANN version, and choose "Offline" installation method. The system will guide you through the subsequent operations.

### 2.3 Source Installation

To use the functions of the latest code or modify the source code to enhance functions, you can download the code from this repository, build and package the tool, and install it.

#### 2.3.1 Preparing the Environment

Set up the environment by referring to the [Operator Tool Development Environment Setup Guide](https://gitcode.com/Ascend/msot/blob/26.1.0/docs/en/common/dev_env_setup.md).

The requirements for compilation tools are as follows:

- The GCC version must be later than 7.4.0.

- The CMake version must be 3.20.2 or later.

- Git LFS is installed.

#### 2.3.2 Building and Packaging

- Clone this repository.

    ```sh
    git clone https://gitcode.com/Ascend/msdebug.git -b 26.1.0
    ```

- Build and packaging

    Run the one-click script to automatically download and build the dependency repository:

    ```shell
    cd msdebug
    python build.py
    ```

> [!NOTE]
>
> If you have modified the code in the local dependency sub-repositories and wish to skip the update process during the build, run `python build.py local`.

When the following information is displayed, the package is built and the .run package is generated.

```text
"mindstudio-debugger_<version>_<arch>.run" successfully created.
```

By default, the built .run package is saved in the `output` directory. In the file name, `<version>` indicates the version number and `<arch>` indicates the CPU architecture.

> [!NOTE] Description
>
> The generation of `.run` depends on the pigz library, which is typically provided by the system. If no version is displayed in `pigz --version`, download it.

#### 2.3.3 Installation

##### 2.3.3.1 Preparing the `.run` Package

Before installation, grant the execute permission to the `.run` package. Go to the directory where the .run package is saved and run the following command to add the execute permission:

```shell
chmod +x mindstudio-debugger_<version>_<arch>.run
```

##### 2.3.3.2 Installation

Copy the .run package to the operating environment and run the following command to install it:

```shell
./mindstudio-debugger_<version>_<arch>.run --run
```

When the following information is displayed, the software package is successfully installed:

```text
mindstudio-debugger package install success!
```

> [!NOTE]
>
> - If the `ASCEND_HOME_PATH` environment variable is configured, the software package will be installed to `${ASCEND_HOME_PATH}`. Otherwise, it will be installed to `${HOME}/Ascend`.
>
> - To specify a custom installation path, use the `--install-path` option. For example: `./mindstudio-debugger_<version>_<arch>.run  --install-path=./test --run` installs the .run package to the `test` directory in the current directory.
>
> - If an earlier version of the tool has been installed in the system, a message will be displayed during the installation asking you whether to replace it. Enter "y" to perform an overwrite installation.

## 3. Installation Verification

After the installation is complete, run the following command to check whether the tool is successfully installed:

```shell
msdebug --help
```

If no error is reported and the help information is displayed, the installation is successful.

## 4. Uninstallation

To uninstall the tool, perform the following steps:

1. Download the script.

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.1.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - Internet access is required to download the script. If your target environment is offline or does not allow Internet access, download the script on an Internet-connected device first, then copy it to the target device.
   > - If the command does not respond, or you encounter connection failures, SSL certificate errors, or other issues, refer to [FAQs](https://www.hiascend.com/developer/blog/details/02176213671719317003).

2. Uninstall the tool.

   ```bash
   python ms_install.py uninstall {tools_name}
   ```

   Replace `{tools_name}` with the name of the tool to be uninstalled. You can run the `python ms_install.py help` command to query the tool name, which is displayed under the `Available Tools` field in the command output.

   If the uninstallation is successful, the following information is displayed:

   ```text
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 5. Upgrade

Upgrades follow the "uninstall first, then install" process. Simply run the installation command. The tool will automatically remove the previous version and guide you through the upgrade process.

You can run the `msdebug --version` command to view the version information of the current environment, and then select the version to upgrade. When upgrading, pay attention to the version compatibility. For details, see the [Release Notes](https://gitcode.com/Ascend/release-management/blob/master/MindStudio/26.1.0/release_notes.md).
