#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

import argparse
import logging
import os
import re
import subprocess
import sys
import traceback
import shutil
import tarfile
from pathlib import Path

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


class BuildManager:
    """
    统一构建管理：依赖拉取 → CMake 配置 → Ninja 编译 → 安装 / 测试。

    用法:
        python build.py [--use-prebuilt]  源码全量编译构建（默认）；--use-prebuilt 走预编译库加速
        python build.py local [--use-prebuilt]    本地构建（跳过依赖拉取）
        python build.py prebuild         从源码全量编译 LLVM/Clang 并打包 prebuilt 包
        python build.py prebuild local   同上（跳过依赖拉取）
        python build.py test [--use-prebuilt]     单元测试（默认源码模式；--use-prebuilt 走预编译）
        python build.py test local [--use-prebuilt]    单元测试（跳过依赖拉取）
        python build.py -r <revision>    指定依赖的内部源码仓(例如msopcom)的 Git 分支/标签/commit
        python build.py -v <version>     指定构建版本号，同时覆盖 --build-version 和 --whl-version
        python build.py -e KEY=VALUE     指定额外构建选项，可多次使用

    参数说明:
        - 参数: command : 构建动作: 为空时为全构建, local 为跳过依赖下载, test 为运行单元测试。
        - 参数: -r, --revision : 指定 Git 修订版本或标签用于依赖检出。
        - 参数: -v, --version : 指定构建版本号；若设置，则同时覆盖 --build-version 和 --whl-version 的值。
        - 参数: --build-version, --whl-version : 历史参数，保留用于兼容；设置了 --version 时以 --version 为准。
        - 参数: -e, --extra : 额外构建选项，格式为 KEY=VALUE，可多次指定。
        - 参数: --use-prebuilt : 使用预编译 LLVM/Clang 库构建（默认关闭，即源码全量编译）。

    产物归档:
        产品构建完成后，归档到 artifacts/ 目录中。
    """

    def __init__(self):
        self.project_root = Path(__file__).resolve().parent
        argument_parser = argparse.ArgumentParser(description='Build the project and optionally run tests.')
        argument_parser.add_argument('command', nargs='*', default=[],
                                     choices=[[], 'local', 'test', 'prebuild'],
                                     help='Build action: omit for prebuilt build, "local" to skip dependency download, "test" to run unit tests, "prebuild" to build from source and package prebuilt libraries')
        argument_parser.add_argument('-r', '--revision',
                                     help='Specify Git revision for internal dependent repo (e.g., msopcom).')
        argument_parser.add_argument('--build-version', type=str, default=None, help='Build version for run/exe/dmg packages')
        argument_parser.add_argument('--whl-version', type=str, default=None, help='WHL version for Python wheel packages')
        argument_parser.add_argument('-v', '--version', type=str, default=None,
                                     help='Build version, overrides --build-version and --whl-version if set')
        argument_parser.add_argument('-e', '--extra', metavar='KEY=VALUE', action='append', default=[],
                                     help='Extra build options in KEY=VALUE format, can be specified multiple times')
        argument_parser.add_argument('--use-prebuilt', action='store_true',
                                     help='Use prebuilt LLVM/Clang libraries (default off: build from source)')
        self.parsed_arguments = argument_parser.parse_args()

        if self.parsed_arguments.version is not None:
            self.parsed_arguments.build_version = self.parsed_arguments.version
            self.parsed_arguments.whl_version = self.parsed_arguments.version

    def _execute_command(self, command_sequence, timeout_seconds=36000, cwd=None, env=None):
        logging.info("Running: %s", " ".join(command_sequence))
        subprocess.run(command_sequence, timeout=timeout_seconds, check=True, cwd=cwd, env=env)

    def _get_cmake_generator(self):
        if shutil.which("ninja") is not None:
            return "Ninja"
        return "Unix Makefiles"

    def _archive_artifacts(self):
        """将产品构建产物（output 目录下的 .run）归档到工程根目录的 artifacts 目录。"""
        artifact_patterns = ("*.run",)
        output_dir = self.project_root / "output"
        artifacts_dir = self.project_root / "artifacts"
        artifacts_dir.mkdir(exist_ok=True)

        if not output_dir.exists():
            logging.warning("Output directory not found, skip archiving: %s", output_dir)
            return

        for pattern in artifact_patterns:
            for artifact in output_dir.rglob(pattern):
                destination = artifacts_dir / artifact.name
                logging.info("Archiving artifact: %s -> %s", artifact, destination)
                shutil.copy2(artifact, destination)

    def _run_unit_tests(self, unit_test_build_dir):
        """运行预编译模式下现编译的全部 LLDB gtest 单测。

        构建产物位于 build_ut/lldb-standalone-build/unittests/ 下，
        由 LLVM.cmake 以 LLDBUnitTests 聚合 target 构建（EXCLUDE_FROM_ALL，
        显式加入 BUILD_COMMAND）。此处遍历所有 *Tests 可执行并逐个执行。
        """
        standalone_dir = unit_test_build_dir / "lldb-standalone-build"
        unittests_dir = standalone_dir / "unittests"
        if not unittests_dir.is_dir():
            logging.warning("未找到 standalone unittests 目录，跳过单测运行: %s", unittests_dir)
            return

        # 测试二进制运行时需链接现编译的 liblldb/libedit/ncurses
        env = os.environ.copy()
        runtime_dirs = [
            standalone_dir / "lib",
            unit_test_build_dir / "libedit" / "lib",
            unit_test_build_dir / "ncurses" / "install" / "lib",
        ]
        ld_paths = [str(p) for p in runtime_dirs if p.is_dir()]
        env["LD_LIBRARY_PATH"] = os.pathsep.join(ld_paths + [env.get("LD_LIBRARY_PATH", "")]).strip(os.pathsep)

        test_exes = []
        for test_exe in sorted(unittests_dir.rglob("*Tests")):
            if test_exe.is_file() and os.access(str(test_exe), os.X_OK):
                test_exes.append(test_exe)

        if not test_exes:
            logging.warning("未找到可运行的单测可执行，请确认 LLDBUnitTests 已构建: %s", unittests_dir)
            return

        failed = []
        total_tests = 0
        for test_exe in test_exes:
            logging.info("=== 运行单测: %s ===", test_exe)
            # gtest 测试自身会打印 [ INFO ]/GMOCK 等输出（brief 无法抑制），
            # 与 lit 一致默认静默：成功只记汇总，仅失败/超时回放完整输出便于定位。
            try:
                proc = subprocess.run([str(test_exe), "--gtest_brief=1"], check=True, env=env,
                                      stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                      text=True, timeout=3600)
                output = proc.stdout
                if proc.returncode != 0:
                    sys.stdout.write(output)
                    sys.stdout.flush()
            except subprocess.CalledProcessError as e:
                failed.append((str(test_exe), e.returncode))
                output = e.output or ""
                sys.stdout.write(output)
                sys.stdout.flush()
                logging.error("单测失败: %s (exit=%s)", test_exe, e.returncode)
            except subprocess.TimeoutExpired as e:
                failed.append((str(test_exe), "timeout"))
                output = e.output or ""
                sys.stdout.write(output)
                sys.stdout.flush()
                logging.error("单测超时: %s", test_exe)

            # 统计该可执行 gtest 汇总行：`[==========] N tests from M test suites ran.`
            m = re.search(r"\[==========\]\s+(\d+)\s+tests?\s+from", output or "")
            if m:
                total_tests += int(m.group(1))

        logging.info("单测汇总: 共 %d 个可执行，用例总数 %d，失败 %d 个",
                     len(test_exes), total_tests, len(failed))
        if failed:
            raise RuntimeError("以下单测失败: " + ", ".join(f"{p}({c})" for p, c in failed))

    @staticmethod
    def _detect_arch():
        """检测当前机器架构（prebuilt 包固定宿主架构，不支持交叉打包）。"""
        import platform
        m = platform.machine().lower()
        if m in ("x86_64", "amd64"):
            return "x86_64"
        if m in ("aarch64", "arm64"):
            return "aarch64"
        raise SystemExit(f"不支持的非宿主架构: {m}")

    def _read_version(self):
        """从 package/conf/version.info 读取版本号。"""
        version_file = self.project_root / "package" / "conf" / "version.info"
        try:
            content = version_file.read_text()
        except OSError:
            return None
        m = re.search(r"Version=([^\n]+)", content)
        return m.group(1).strip() if m else None

    @staticmethod
    def _relocate_cmake_file(path, build_root, project_root):
        """把 cmake 配置文件中的绝对路径重定位为可移植路径。

        - build 树路径（生成的 config.h/.inc 所在）→ 相对 CMAKE_CURRENT_LIST_DIR，指向 prebuilt 包
        - 源码树路径（llvm/include、clang/include）→ ${MSDEBUG_SOURCE_DIR}/llvm 等，
          由 LLVM.cmake 以 -DMSDEBUG_SOURCE_DIR 传入（源码头文件来自仓库源码树，不打包）
        """
        content = path.read_text()
        build_root_str = str(build_root)
        content = content.replace(build_root_str, "${CMAKE_CURRENT_LIST_DIR}/../../..")
        src_llvm = str(project_root / "llvm")
        src_clang = str(project_root / "clang")
        content = content.replace(src_llvm, "${MSDEBUG_SOURCE_DIR}/llvm")
        content = content.replace(src_clang, "${MSDEBUG_SOURCE_DIR}/clang")
        path.write_text(content)

    def _prepare_prebuilt(self):
        """制作 prebuilt 预编译包：从 build/llvm-build 产物拷贝 .a + 生成头 + cmake 配置，并打 tar 可分发包。"""
        build = self.project_root / "build" / "llvm-build"
        output = self.project_root / "prebuilt"
        version = self._read_version()
        if not version:
            logging.warning("未获取到版本号，tarball 命名将不含版本")

        if not (build / "lib" / "cmake" / "llvm" / "LLVMConfig.cmake").exists():
            logging.error("%s 不是有效的 LLVM 构建目录（缺少 lib/cmake/llvm/LLVMConfig.cmake）", build)
            sys.exit(1)

        arch = self._detect_arch()
        out = output / arch
        lib_dir = out / "lib"
        inc_dir = out / "include"
        cmake_llvm = lib_dir / "cmake" / "llvm"
        cmake_clang = lib_dir / "cmake" / "clang"

        shutil.rmtree(out, ignore_errors=True)
        lib_dir.mkdir(parents=True)
        inc_dir.mkdir(parents=True)

        # 1. 拷贝预编译库
        for pat in ("libLLVM*.a", "libclang*.a"):
            n = 0
            for f in build.glob(f"lib/{pat}"):
                shutil.copy2(f, lib_dir)
                n += 1
            logging.info("拷贝 %s: %d 个", pat, n)
        if not any(lib_dir.glob("libLLVM*.a")):
            logging.error("未找到 libLLVM*.a")
            sys.exit(1)
        if not any(lib_dir.glob("libclang*.a")):
            logging.error("未找到 libclang*.a")
            sys.exit(1)

        # 2. 拷贝 cmake 配置并重定位路径
        shutil.copytree(build / "lib" / "cmake" / "llvm", cmake_llvm)
        shutil.copytree(build / "lib" / "cmake" / "clang", cmake_clang)
        for cmake_dir in (cmake_llvm, cmake_clang):
            for f in cmake_dir.glob("*.cmake"):
                self._relocate_cmake_file(f, build, self.project_root)
        llvm_cmake_modules = out / "cmake" / "modules"
        shutil.copytree(self.project_root / "llvm" / "cmake" / "modules", llvm_cmake_modules, dirs_exist_ok=True)
        logging.info("cmake 配置拷贝并重定位完成")

        # 3. 拷贝生成头（config.h / .inc。源码头文件来自仓库源码树，不打包）
        def copy_headers(src, dst):
            if src.exists():
                shutil.copytree(src, dst, dirs_exist_ok=True)

        copy_headers(build / "include" / "llvm", inc_dir / "llvm")
        clang_gen_inc = out / "tools" / "clang" / "include" / "clang"
        copy_headers(build / "tools" / "clang" / "include" / "clang", clang_gen_inc)
        logging.info("生成头拷贝完成")

        # 3.5 拷贝 llvm-tblgen 工具（LLDB standalone 编译必需，LLVM 不构建时缺失）
        tblgen_src = build / "bin" / "llvm-tblgen"
        if tblgen_src.exists():
            bin_dir = out / "bin"
            bin_dir.mkdir(exist_ok=True)
            shutil.copy2(tblgen_src, bin_dir / "llvm-tblgen")
            logging.info("llvm-tblgen 工具已打包到 bin/")
        else:
            logging.error("未找到 llvm-tblgen: %s", tblgen_src)
            sys.exit(1)

        # 4. 校验关键接口文件
        checks = [
            (cmake_llvm / "LLVMConfig.cmake", "LLVMConfig.cmake"),
            (cmake_clang / "ClangConfig.cmake", "ClangConfig.cmake"),
            (inc_dir / "llvm" / "Config" / "config.h", "llvm/Config/config.h"),
            (inc_dir / "llvm" / "CodeGen" / "GenVT.inc", "llvm/CodeGen/GenVT.inc"),
            (out / "tools" / "clang" / "include" / "clang" / "AST" / "DeclNodes.inc", "clang/AST/DeclNodes.inc"),
        ]
        for path, name in checks:
            if not path.exists():
                logging.error("关键接口文件缺失: %s", name)
                sys.exit(1)

        # 5. 版本元数据
        if version:
            (out / "PREBUILT_VERSION").write_text(version + "\n")

        logging.info("prebuilt 包就绪: %s", out)
        logging.info("  库: %d LLVM + %d clang",
                     len(list(lib_dir.glob("libLLVM*.a"))), len(list(lib_dir.glob("libclang*.a"))))

        # 6. 打 tar 包
        name = "msdebug-prebuilt-llvm"
        if version:
            name += f"-{version}"
        name += f"-{arch}"
        tar_path = output / f"{name}.tar.gz"
        with tarfile.open(tar_path, "w:gz") as tar:
            tar.add(out, arcname=arch)
        logging.info("tar 包已生成: %s (%.1f MB)", tar_path, tar_path.stat().st_size / 1024 / 1024)

    def _build_and_package_prebuilt(self):
        """从源码全量编译 LLVM/Clang 并打包 prebuilt 预编译包（生成可分发产物）。

        流程：源码模式配置 → 全量编译 llvm_project（产出 build/llvm-build）→ 打包。
        """
        logging.info("=== prebuild: 全量编译 LLVM/Clang 并打包 prebuilt 包 ===")
        build_dir = self.project_root / "build"
        build_dir.mkdir(exist_ok=True)
        os.chdir(build_dir)

        # 1. 源码模式配置（不启用预编译，全量编译 LLVM/Clang）
        self._execute_command([
            "cmake", "-G", self._get_cmake_generator(),
            "-DCMAKE_BUILD_TYPE=Release",
            "-DENABLE_LLDB_TESTS=OFF",
            "-DUSE_PREBUILT_LLVM=OFF",
            ".."
        ])

        # 2. 全量编译 llvm_project（产出 build/llvm-build 的 .a / 生成头 / llvm-tblgen）
        self._execute_command(["cmake", "--build", ".", "--target", "llvm_project"])

        # 2.5 补编全量单测所需的 LLVM 库：libLLVMObjectYAML.a 不在 LLDB 产品链接闭包
        # （llvm_project 只编 lldb/lldb-server/runtime_stub），但 lldb/unittests 的
        # Callback 单测链接它。预编译模式跑 UT 需此库随包分发。
        self._execute_command(["cmake", "--build", "llvm-build", "--target", "LLVMObjectYAML"])

        # 3. 打包 prebuilt 包
        self._prepare_prebuilt()

    def run(self):
        os.chdir(self.project_root)

        if self.parsed_arguments.build_version != None:
            logging.info("--build-version: %s", self.parsed_arguments.build_version)
            subprocess.run(['sed', '-i', f"s/^Version=.*/Version={self.parsed_arguments.build_version}/", "./package/conf/version.info"], check=True)
        extra_options = {}
        for option in self.parsed_arguments.extra:
            key, _, value = option.partition('=')
            extra_options[key] = value
            logging.info("--extra: %s = %s", key, value)

        # prebuild：从源码全量编译 + 打包 prebuilt 包
        if 'prebuild' in self.parsed_arguments.command:
            self._build_and_package_prebuilt()
            return

        use_prebuilt = self.parsed_arguments.use_prebuilt
        prebuilt_flag = "-DUSE_PREBUILT_LLVM=ON" if use_prebuilt else "-DUSE_PREBUILT_LLVM=OFF"
        if use_prebuilt:
            logging.info("使用预编译 LLVM/Clang 库构建（USE_PREBUILT_LLVM=ON）")
        else:
            logging.info("源码全量编译 LLVM/Clang（默认，USE_PREBUILT_LLVM=OFF）")

        # 在非 local 场景下按需更新依赖；在 local 场景下仅使用本地已有代码，不更新依赖。
        if 'local' not in self.parsed_arguments.command:
            from download_dependencies import DependencyManager
            # 源码模式无需 prebuilt 包（不下载，避免 x86_64 等未发布架构报缺链接）；
            # 仅 --use-prebuilt 才拉取对应架构的 prebuilt 包。
            DependencyManager(self.parsed_arguments, need_prebuilt=use_prebuilt).run()

        if extra_options.get('only_down_deps') == 'true':
            logging.info("only_down_deps=true, exiting after dependency download.")
            return

        if 'test' in self.parsed_arguments.command:
            if use_prebuilt:
                logging.info("预编译模式 UT：LLVM/Clang 用 prebuilt .a，现编译 LLDB + gtest 单测")
            else:
                logging.info("源码模式 UT：全量编译 LLVM/Clang，check-lldb-unit（lit）驱动单测")
            unit_test_build_dir = self.project_root / "build_ut"
            unit_test_build_dir.mkdir(exist_ok=True)
            os.chdir(unit_test_build_dir)

            self._execute_command([
                "cmake", "-G", self._get_cmake_generator(),
                "-DCMAKE_BUILD_TYPE=Release",
                "-DENABLE_LLDB_TESTS=ON",
                "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                prebuilt_flag,
                ".."
            ])
        else:
            # 产品构建（源码模式默认；--use-prebuilt 走预编译）
            product_build_dir = self.project_root / "build"
            product_build_dir.mkdir(exist_ok=True)
            os.chdir(product_build_dir)

            self._execute_command([
                "cmake", "-G", self._get_cmake_generator(),
                "-DCMAKE_BUILD_TYPE=Release",
                "-DENABLE_LLDB_TESTS=OFF",
                "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                prebuilt_flag,
                ".."
            ])
        # 自动选择ninja还是make构建
        self._execute_command(["cmake", "--build", "."])

        if 'test' in self.parsed_arguments.command:
            if use_prebuilt:
                self._run_unit_tests(unit_test_build_dir)
            else:
                logging.info("源码模式 UT 已由 check-lldb-unit（lit）执行")
        else:
            self._archive_artifacts()


if __name__ == "__main__":
    try:
        BuildManager().run()
    except Exception:
        logging.error(f"Unexpected error: {traceback.format_exc()}")
        sys.exit(1)
