#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

import argparse
import json
import logging
import os
import re
import subprocess
import sys
import tempfile
import traceback
import shutil
import tarfile
from pathlib import Path

from download_dependencies import cxx11_abi, default_cxx

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


class BuildManager:
    """
    统一构建管理：依赖拉取 → CMake 配置 → Ninja 编译 → 安装 / 测试。

    用法:
        python build.py [--no-prebuilt]  预编译库加速构建（默认）；--no-prebuilt 走源码全量编译
        python build.py local [--no-prebuilt]    本地构建（跳过依赖拉取）
        python build.py prebuild         从源码全量编译 LLVM/Clang 并打包 prebuilt 包
        python build.py prebuild local   同上（跳过依赖拉取）
        python build.py test [--no-prebuilt]     单元测试（默认预编译模式；--no-prebuilt 走源码）
        python build.py test local [--no-prebuilt]    单元测试（跳过依赖拉取）
        python build.py -r <revision>    指定依赖的内部源码仓(例如msopcom)的 Git 分支/标签/commit
        python build.py -v <version>     指定构建版本号，同时覆盖 --build-version 和 --whl-version
        python build.py -e KEY=VALUE     指定额外构建选项，可多次使用

    参数说明:
        - 参数: command : 构建动作: 为空时为全构建, local 为跳过依赖下载, test 为运行单元测试。
        - 参数: -r, --revision : 指定 Git 修订版本或标签用于依赖检出。
        - 参数: -v, --version : 指定构建版本号；若设置，则同时覆盖 --build-version 和 --whl-version 的值。
        - 参数: --build-version, --whl-version : 历史参数，保留用于兼容；设置了 --version 时以 --version 为准。
        - 参数: -e, --extra : 额外构建选项，格式为 KEY=VALUE，可多次指定。
        - 参数: --no-prebuilt : 关闭预编译，从源码全量编译 LLVM/Clang（默认开启预编译加速）。
        - ABI 兼容 : prebuilt 包由高于本机的 GCC/glibc 构建时会自动回退源码全量编译（libstdc++ 仅向后兼容）。

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
        argument_parser.add_argument('--no-prebuilt', action='store_true',
                                     help='Disable prebuilt LLVM/Clang libraries and build from source (default on: use prebuilt)')
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

    def _prebuilt(self):
        """构造 prebuilt 管理器，复用本类的命令执行与 CMake 生成器选择。"""
        return PrebuiltManager(self.project_root, self._execute_command, self._get_cmake_generator)

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
            self._prebuilt().build_and_package()
            return

        use_prebuilt = not self.parsed_arguments.no_prebuilt

        # 在非 local 场景下按需更新依赖；在 local 场景下仅使用本地已有代码，不更新依赖。
        if 'local' not in self.parsed_arguments.command:
            from download_dependencies import DependencyManager
            # 预编译模式（默认）拉取对应架构的 prebuilt 包；
            # --no-prebuilt 源码模式无需 prebuilt 包，只下 submodule（避免未发布架构报缺链接）。
            DependencyManager(self.parsed_arguments, need_prebuilt=use_prebuilt).run()

        # ABI 预检：prebuilt 包若由更高版本 GCC/glibc 构建，其静态库依赖本机
        # libstdc++/glibc 没有的新符号，链接会 undefined reference。此时自动回退源码。
        if use_prebuilt:
            prebuilt = self._prebuilt()
            abi_ok, abi_reason = prebuilt.check_abi(prebuilt.detect_arch())
            if not abi_ok:
                logging.warning("prebuilt 包与本机环境不兼容，自动回退源码全量编译：%s", abi_reason)
                logging.warning("如需使用 prebuilt，请与代码仓库container联系。")
                use_prebuilt = False

        prebuilt_flag = "-DUSE_PREBUILT_LLVM=ON" if use_prebuilt else "-DUSE_PREBUILT_LLVM=OFF"
        if use_prebuilt:
            logging.info("使用预编译 LLVM/Clang 库构建（默认，USE_PREBUILT_LLVM=ON）")
        elif self.parsed_arguments.no_prebuilt:
            logging.info("源码全量编译 LLVM/Clang（--no-prebuilt，USE_PREBUILT_LLVM=OFF）")
        else:
            logging.info("源码全量编译 LLVM/Clang（ABI 回退，USE_PREBUILT_LLVM=OFF）")

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
            # 产品构建（默认预编译；--no-prebuilt 走源码）
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


class PrebuiltManager:
    """prebuilt LLVM/Clang 预编译包：全量构建、ABI 兼容校验、打包与元数据。

    职责：
        - 检测宿主架构与工具链/glibc/ABI 元数据；
        - 校验已下载的 prebuilt 包与本机是否 ABI 兼容（不兼容则调用方回退源码编译）；
        - `prebuild` 流程：源码全量编译 LLVM/Clang → 打包成可分发 tar 包。

    命令执行与 CMake 生成器由 BuildManager 注入（run_command / cmake_generator），
    避免重复实现。
    """

    def __init__(self, project_root, run_command, cmake_generator):
        self.project_root = Path(project_root)
        self._run_command = run_command
        self._cmake_generator = cmake_generator

    @staticmethod
    def detect_arch():
        """检测当前机器架构（prebuilt 包固定宿主架构，不支持交叉打包）。"""
        import platform
        m = platform.machine().lower()
        if m in ("x86_64", "amd64"):
            return "x86_64"
        if m in ("aarch64", "arm64"):
            return "aarch64"
        raise SystemExit(f"不支持的非宿主架构: {m}")

    @staticmethod
    def _ver_tuple(version):
        """把版本字符串转为可比较的整数元组，如 "2.17" -> (2, 17)。"""
        return tuple(int(x) for x in re.findall(r"\d+", str(version)))

    @staticmethod
    def _tool_output(cmd, text_input=None):
        try:
            return subprocess.check_output(cmd, input=text_input, stderr=subprocess.DEVNULL,
                                           text=True).strip()
        except Exception:
            return None

    @classmethod
    def _compiler_version(cls, cxx):
        """获取编译器版本号，如 9.4.0。"""
        out = cls._tool_output([cxx, "--version"])
        if not out:
            return None
        m = re.search(r"(\d+)\.(\d+)\.(\d+)", out)
        return m.group(0) if m else None

    @classmethod
    def _host_glibc_version(cls):
        """获取本机 glibc 版本，如 2.17。"""
        out = cls._tool_output(["getconf", "GNU_LIBC_VERSION"])
        if not out:
            return None
        m = re.search(r"(\d+\.\d+)", out)
        return m.group(1) if m else None

    @staticmethod
    def _max_symbol_version(text, prefix):
        """从 readelf 输出中提取某符号前缀（GLIBCXX/CXXABI）的最高版本，返回可比较的整数元组。"""
        versions = re.findall(rf"{prefix}_([0-9]+(?:\.[0-9]+)*)", text or "")
        return max((PrebuiltManager._ver_tuple(v) for v in versions), default=None)

    @staticmethod
    def _fmt_ver_tuple(version):
        """把版本元组还原为字符串，如 (3, 4, 26) -> "3.4.26"。"""
        return ".".join(str(x) for x in version) if version else None

    @classmethod
    def _host_libstdcxx_ceiling(cls, cxx=None):
        """本机 libstdc++.so.6 提供的最高 GLIBCXX/CXXABI 符号版本，返回 (glibcxx, cxxabi)。

        这是判断 prebuilt 包能否在本机链接的**真正下限**：编译器版本号不参与判断。
        """
        cxx = cxx or default_cxx()
        candidates = []
        print_name = cls._tool_output([cxx, "-print-file-name=libstdc++.so.6"])
        if print_name:
            candidates.append(print_name)
        m = re.search(r"=>\s*(\S*libstdc\+\+\.so\.6\S*)", cls._tool_output(["ldconfig", "-p"]) or "")
        if m:
            candidates.append(m.group(1))
        for path in candidates:
            if not Path(path).exists():
                continue
            text = cls._tool_output(["readelf", "--version-info", path])
            if text:
                return (cls._fmt_ver_tuple(cls._max_symbol_version(text, "GLIBCXX")),
                        cls._fmt_ver_tuple(cls._max_symbol_version(text, "CXXABI")))
        return None, None

    def _probe_libstdcxx_requirement(self, lib_dir, cxx=None):
        """链接探针：探测 prebuilt 静态库**实际需要**的最高 libstdc++ 符号版本（GLIBCXX/CXXABI）。

        静态库自身不带 @GLIBCXX_ 版本标记，只有链接后才会在产物的版本需求表中出现。
        以 --whole-archive 全量拉入、--unresolved-symbols=ignore-all 保证产物一定生成，
        再 readelf 读产物取其上限，作为使用端 libstdc++ 的下限。返回 (glibcxx, cxxabi)。
        """
        # 只探测实际随包分发的库（libLLVM*/libclang*）；build 树里还有 lldb 插件库，
        # 其中 ElfoCore/ElfCoreDevice 等存在同名重复符号，全量链会多重定义。
        libs = sorted(str(p) for pattern in ("libLLVM*.a", "libclang*.a")
                      for p in Path(lib_dir).glob(pattern))
        if not libs:
            return None, None
        cxx = cxx or default_cxx()
        with tempfile.TemporaryDirectory() as td:
            td = Path(td)
            (td / "probe.cpp").write_text("int main() { return 0; }\n")
            (td / "libs.rsp").write_text("\n".join(libs) + "\n")
            out = td / "probe"
            cmd = [cxx, str(td / "probe.cpp"),
                   "-Wl,--whole-archive", f"-Wl,@{td / 'libs.rsp'}", "-Wl,--no-whole-archive",
                   "-Wl,--unresolved-symbols=ignore-all", "-o", str(out)]
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0 or not out.exists():
                tail = (result.stderr or "").strip().splitlines()
                logging.warning("libstdc++ 版本探针链接失败，本次不记录 GLIBCXX/CXXABI 下限：%s",
                                tail[-1] if tail else "")
                return None, None
            text = self._tool_output(["readelf", "--version-info", str(out)])
        if not text:
            logging.warning("readelf 不可用，本次不记录 GLIBCXX/CXXABI 下限")
            return None, None
        return (self._fmt_ver_tuple(self._max_symbol_version(text, "GLIBCXX")),
                self._fmt_ver_tuple(self._max_symbol_version(text, "CXXABI")))

    def _collect_meta(self, arch, build, version):
        """收集 prebuilt 包的构建元数据（工具链/glibc/ABI），供使用方做兼容性校验。"""
        cxx = None
        cache = build / "CMakeCache.txt"
        cache_text = cache.read_text() if cache.exists() else ""
        if cache_text:
            m = re.search(r"^CMAKE_CXX_COMPILER:[A-Z_]+=(.+)$", cache_text, re.M)
            if m:
                cxx = m.group(1).strip()
        cxx = cxx or default_cxx()
        compiler_version = self._compiler_version(cxx)
        compiler_major = int(compiler_version.split(".")[0]) if compiler_version else None
        glibc_version = self._host_glibc_version()
        # libstdc++ 符号版本下限：由链接探针实测包真正需要的 GLIBCXX/CXXABI 上限
        glibcxx_min, cxxabi_min = self._probe_libstdcxx_requirement(build / "lib", cxx)

        llvm_version = None
        llvm_cfg = build / "include" / "llvm" / "Config" / "llvm-config.h"
        if llvm_cfg.exists():
            m = re.search(r'#define LLVM_VERSION_STRING "([^"]+)"', llvm_cfg.read_text())
            if m:
                llvm_version = m.group(1)

        llvm_targets = None
        if cache_text:
            m = re.search(r"^LLVM_TARGETS_TO_BUILD:[A-Z_]+=(.+)$", cache_text, re.M)
            if m:
                llvm_targets = m.group(1).strip()

        return {
            "os": "linux",
            "arch": arch,
            "compiler": Path(cxx).name,
            "compiler_version": compiler_version,
            "compiler_major": compiler_major,
            "glibc_version": glibc_version,
            "cxx11_abi": cxx11_abi(cxx),
            # 兼容性下限（构建机即保守下限：glibc/libstdc++ 仅向后兼容）
            "min_gcc": compiler_major,
            "min_glibc": glibc_version,
            "glibcxx_min": glibcxx_min,
            "cxxabi_min": cxxabi_min,
            # 能力标记，避免误用
            "llvm_targets": llvm_targets,
            "has_ut_libs": (build / "lib" / "libLLVMObjectYAML.a").exists(),
            "llvm_version": llvm_version,
            "prebuilt_version": version,
        }

    def check_abi(self, arch):
        """校验 prebuilt 包与本机工具链是否 ABI 兼容。

        规则：_GLIBCXX_USE_CXX11_ABI 必须与本机一致；glibc 与 libstdc++ 符号版本
        （GLIBCXX/CXXABI）均需不低于包构建基线。不比较 GCC 主版本——低/高 GCC 未必
        决定符号兼容，真正的链接判据是 libstdc++.so.6 提供的符号版本。
        prebuilt 包必须携带 prebuilt_meta.json。返回 (是否兼容, 原因说明)。
        """
        meta_path = self.project_root / "prebuilt" / arch / "prebuilt_meta.json"
        if not meta_path.exists():
            return False, ("prebuilt 包缺少 prebuilt_meta.json（包不完整），无法校验 ABI/glibc 兼容性；"
                           "请更新 prebuilt 包，或使用 --no-prebuilt 从源码编译")
        try:
            meta = json.loads(meta_path.read_text())
        except Exception as e:
            return False, f"prebuilt 元数据解析失败({e})，请更新 prebuilt 包或使用 --no-prebuilt"

        # ABI 档（0/1）必须与本机一致，否则 std 符号名不匹配
        meta_abi = meta.get("cxx11_abi")
        host_abi = cxx11_abi()
        if meta_abi and host_abi and meta_abi != host_abi:
            return False, (f"prebuilt 包 _GLIBCXX_USE_CXX11_ABI={meta_abi} 与本机 ABI={host_abi} "
                           f"不一致（C++11 ABI 符号不匹配，链接会 undefined reference）")

        need_glibc = meta.get("min_glibc") or meta.get("glibc_version")
        host_glibc = self._host_glibc_version()
        if need_glibc and host_glibc and self._ver_tuple(host_glibc) < self._ver_tuple(need_glibc):
            return False, f"本机 glibc {host_glibc} 低于 prebuilt 最低要求 glibc {need_glibc}"

        # libstdc++ 符号版本下限：本机 libstdc++.so.6 必须提供包所需的 GLIBCXX/CXXABI
        need_glibcxx = meta.get("glibcxx_min")
        need_cxxabi = meta.get("cxxabi_min")
        if need_glibcxx or need_cxxabi:
            host_glibcxx, host_cxxabi = self._host_libstdcxx_ceiling()
            for label, need, have in (("GLIBCXX", need_glibcxx, host_glibcxx),
                                      ("CXXABI", need_cxxabi, host_cxxabi)):
                if need and have and self._ver_tuple(have) < self._ver_tuple(need):
                    return False, (f"本机 libstdc++ 最高提供 {label}_{have}，低于 prebuilt 包要求的 "
                                   f"{label}_{need}（本机编译器/工具链版本过低，链接会 undefined reference）")
            logging.info("libstdc++ 符号版本校验通过：需 GLIBCXX_%s/CXXABI_%s，本机提供 GLIBCXX_%s/CXXABI_%s",
                         need_glibcxx, need_cxxabi, host_glibcxx, host_cxxabi)

        return True, "ABI 兼容"

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

    @staticmethod
    def package_name(version, arch, meta):
        """prebuilt tar 包名（不含扩展名）：msdebug-prebuilt-llvm-<ver>-<arch>-gcc<N>-abi<A>。

        带上 GCC 主版本与 ABI 档，使不同工具链/ABI 的同类包在命名上可区分。
        """
        name = "msdebug-prebuilt-llvm"
        if version:
            name += f"-{version}"
        name += f"-{arch}"
        if meta.get("compiler_major") is not None:
            name += f"-gcc{meta['compiler_major']}"
        if meta.get("cxx11_abi") is not None:
            name += f"-abi{meta['cxx11_abi']}"
        return name

    def prepare(self):
        """制作 prebuilt 预编译包：从 build/llvm-build 产物拷贝 .a + 生成头 + cmake 配置，并打 tar 可分发包。"""
        build = self.project_root / "build" / "llvm-build"
        output = self.project_root / "prebuilt"
        version = self._read_version()
        if not version:
            logging.warning("未获取到版本号，tarball 命名将不含版本")

        if not (build / "lib" / "cmake" / "llvm" / "LLVMConfig.cmake").exists():
            logging.error("%s 不是有效的 LLVM 构建目录（缺少 lib/cmake/llvm/LLVMConfig.cmake）", build)
            sys.exit(1)

        arch = self.detect_arch()
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

        # 5. 版本与构建元数据
        if version:
            (out / "PREBUILT_VERSION").write_text(version + "\n")
        meta = self._collect_meta(arch, build, version)
        (out / "prebuilt_meta.json").write_text(json.dumps(meta, indent=2, ensure_ascii=False) + "\n")
        logging.info("prebuilt 元数据: compiler=%s %s, glibc=%s, cxx11_abi=%s",
                     meta["compiler"], meta["compiler_version"], meta["glibc_version"], meta["cxx11_abi"])

        logging.info("prebuilt 包就绪: %s", out)
        logging.info("  库: %d LLVM + %d clang",
                     len(list(lib_dir.glob("libLLVM*.a"))), len(list(lib_dir.glob("libclang*.a"))))

        # 6. 打 tar 包（包名带 GCC 版本与 ABI 档，避免同名覆盖混淆）
        name = self.package_name(version, arch, meta)
        tar_path = output / f"{name}.tar.gz"
        with tarfile.open(tar_path, "w:gz") as tar:
            tar.add(out, arcname=arch)
        logging.info("tar 包已生成: %s (%.1f MB)", tar_path, tar_path.stat().st_size / 1024 / 1024)

    def build_and_package(self):
        """从源码全量编译 LLVM/Clang 并打包 prebuilt 预编译包（生成可分发产物）。

        流程：源码模式配置 → 全量编译 llvm_project（产出 build/llvm-build）→ 打包。
        """
        logging.info("=== prebuild: 全量编译 LLVM/Clang 并打包 prebuilt 包 ===")
        build_dir = self.project_root / "build"
        build_dir.mkdir(exist_ok=True)
        os.chdir(build_dir)

        # 1. 源码模式配置（不启用预编译，全量编译 LLVM/Clang）
        self._run_command([
            "cmake", "-G", self._cmake_generator(),
            "-DCMAKE_BUILD_TYPE=Release",
            "-DENABLE_LLDB_TESTS=OFF",
            "-DUSE_PREBUILT_LLVM=OFF",
            ".."
        ])

        # 2. 全量编译 llvm_project（产出 build/llvm-build 的 .a / 生成头 / llvm-tblgen）
        self._run_command(["cmake", "--build", ".", "--target", "llvm_project"])

        # 2.5 补编全量单测所需的 LLVM 库：libLLVMObjectYAML.a 不在 LLDB 产品链接闭包
        # （llvm_project 只编 lldb/lldb-server/runtime_stub），但 lldb/unittests 的
        # Callback 单测链接它。预编译模式跑 UT 需此库随包分发。
        self._run_command(["cmake", "--build", "llvm-build", "--target", "LLVMObjectYAML"])

        # 3. 打包 prebuilt 包
        self.prepare()


if __name__ == "__main__":
    try:
        BuildManager().run()
    except Exception:
        logging.error(f"Unexpected error: {traceback.format_exc()}")
        sys.exit(1)
