# msDebug Installation Guide

<br>

## 1. Installation Description

This tool has been integrated into CANN. If CANN has been installed and this tool does not need to be updated, you can directly use it without following the instructions in this document.

If CANN has not been installed in your environment, install the Ascend NPU driver and CANN software (including the Toolkit and ops) by referring to [CANN Quick Installation](https://www.hiascend.com/en/cann/download), and configure environment variables.

If you need to upgrade this tool separately or use the latest version, you can install it in any of the following ways: [Online Installation](#21-online-installation), [Offline Installation](#22-offline-installation), and [Source Installation](#23-source-installation).

## 2. Installation Methods

### 2.1 Online Installation

If your device has Internet access, you can run a single command to automatically download and install the tool. Visit the [Ascend community](https://www.hiascend.com/en/developer/software/mindstudio/download), select the target CANN version, and choose "Online" installation method. The system will guide you through the subsequent operations.

### 2.2 Offline Installation

For devices that are not connected to the Internet, such as those on an enterprise intranet, download the complete offline installation package on a device that has Internet access and then transfer the package to the target device for installation. Visit the [Ascend community](https://www.hiascend.com/en/developer/software/mindstudio/download), select the target CANN version, and choose "Offline" installation method. The system will guide you through the subsequent operations.

### 2.3 Source Installation

To use the functions of the latest code or modify the source code to enhance functions, you can download the code from this repository, build and package the tool, and install it.

#### 2.3.1 Preparing the Environment

Source compilation consistently uses the MindStudio standard build environment.

- For daily development or use of the released image, refer to [MindStudio Tool Development Environment Installation Guide](https://atomgit.com/Ascend/msot/blob/26.2.0/docs/en/common/dev_env_setup.md).
- If you need to reproduce the environment from a base operating system or run source build verification or unit test verification, you must refer to [MindStudio Unified Build Image Guide](https://atomgit.com/Ascend/msot/blob/26.2.0/docs/en/common/docker_image_build_guide.md) and build an environment image on site from the openEuler base image.

Run the source compilation and unit test commands in the remainder of this guide in the container of the aforementioned specified image or in the container of the environment image built on site. For the CANN package, GCC, and Python versions, refer to the unified image building guide. This repository does not maintain them separately.

After the image is built, you must use the `ctr_in.py` command provided in Chapter 7 of the unified image building guide to start and enter the container in an interactive terminal. Do not use a regular `docker run` command to create the container, and do not use `docker exec <container name> bash -c '<command>'` to replace the interactive environment. Otherwise, the Python, GCC, and CANN environment initialization may be skipped.

After you enter the interactive container shell opened by `ctr_in.py`, run the following command to clone this repository:

```bash
cd ~
git clone https://atomgit.com/Ascend/msdebug.git -b 26.2.0
```

#### 2.3.2 Building

Stay in the same interactive container shell opened by `ctr_in.py` and run the following command in the repository root directory. The command downloads the dependencies and completes the build automatically:

```bash
cd ~/msdebug
python3 build.py
```

After the build succeeds, the installation package is generated in the `artifacts/` directory.

#### 2.3.3 Running Unit Tests (Optional)

This step is not required for installation. To verify the basic functions of the code, run the unit tests:

```bash
cd ~/msdebug
python3 build.py test
```

If the command returns exit code 0 and no test case fails, the unit tests pass.

#### 2.3.4 Installation

##### 2.3.4.1 Preparing the `.run` Package

Before installation, grant the execute permission to the `.run` package. Go to the directory where the `.run` package is saved and run the following command to add the execute permission:

```bash
cd ~/msdebug/artifacts
chmod +x mindstudio-debugger_<version>_<arch>.run
```

##### 2.3.4.2 Installation

Copy the `.run` package to the operating environment and run the following command to install it:

```bash
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
> - To specify a custom installation path, use the `--install-path` option. For example: `./mindstudio-debugger_<version>_<arch>.run --install-path=./test --run` installs the `.run` package to the `test` directory in the current directory.
>
> - If an earlier version of the tool has been installed in the system, a message will be displayed during the installation asking you whether to replace it. Enter "y" to perform an overwrite installation.

## 3. Installation Verification

After the installation is complete, run the following command to check whether the tool is successfully installed:

```bash
msdebug --help
```

If no error is reported and the help information is displayed, the installation is successful.

## 4. Uninstallation

To uninstall the tool, perform the following steps:

1. Download the script.

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.2.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - Internet access is required to download the script. If your target environment is offline or does not allow Internet access, download the script on an Internet-connected device first, then copy it to the target device.
   > - If the command does not respond, or you encounter connection failures, SSL certificate errors, or other issues, refer to [FAQs](https://www.hiascend.com/developer/blog/details/02176213671719317003).

2. Uninstall the tool.

   ```bash
   python3 ms_install.py uninstall {tools_name}
   ```

   Replace `{tools_name}` with the name of the tool to be uninstalled. You can run the `python3 ms_install.py help` command to query the tool name, which is displayed under the `Available Tools` field in the command output.

   If the uninstallation is successful, the following information is displayed:

   ```text
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 5. Upgrade

Upgrades follow the "uninstall first, then install" process. Simply run the installation command. The tool will automatically remove the previous version and guide you through the upgrade process.

You can run the `msdebug --version` command to view the version information of the current environment, and then select the version to upgrade. When upgrading, pay attention to the version compatibility. For details, see the [Release Notes](https://atomgit.com/Ascend/release-management/blob/master/MindStudio/26.2.0/release_notes.md).
