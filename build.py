#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

import argparse
import logging
import os
import subprocess
import sys
import traceback
import shutil
from pathlib import Path

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


class BuildManager:
    """
    统一构建管理：依赖拉取 → CMake 配置 → Ninja 编译 → 安装 / 测试。

    用法:
        python build.py                  完整构建（拉取依赖 + Release 编译）
        python build.py local            本地构建（跳过依赖拉取, Release 编译）
        python build.py test             单元测试（拉取依赖 + 编译 + 执行测试）
        python build.py test local       单元测试（跳过依赖拉取, 编译 + 执行测试）
        python build.py -r <revision>    指定依赖的内部源码仓(例如msopcom)的 Git 分支/标签/commit

    参数说明:
        - 参数: command : 构建动作: 为空时为全构建, local 为跳过依赖下载, test 为运行单元测试。
        - 参数: -r, --revision : 指定 Git 修订版本或标签用于依赖检出。
    """

    def __init__(self):
        self.project_root = Path(__file__).resolve().parent
        argument_parser = argparse.ArgumentParser(description='Build the project and optionally run tests.')
        argument_parser.add_argument('command', nargs='*', default=[],
                                     choices=[[], 'local', 'test'],
                                     help='Build action: omit for full build, "local" to skip dependency download, "test" to run unit tests')
        argument_parser.add_argument('-r', '--revision',
                                     help='Specify Git revision for internal dependent repo (e.g., msopcom).')
        argument_parser.add_argument('--build-version', type=str, default=None,
                                     help='Build version for run/exe/dmg packages')
        argument_parser.add_argument('--whl-version', type=str, default=None,
                                     help='WHL version for Python wheel packages')
        self.parsed_arguments = argument_parser.parse_args()

    def _execute_command(self, command_sequence, timeout_seconds=36000, cwd=None, env=None):
        logging.info("Running: %s", " ".join(command_sequence))
        subprocess.run(command_sequence, timeout=timeout_seconds, check=True, cwd=cwd, env=env)

    def _get_cmake_generator(self):
        if shutil.which("ninja") is not None:
            return "Ninja"
        return "Unix Makefiles"

    def run(self):
        os.chdir(self.project_root)

        if self.parsed_arguments.build_version != None:
            logging.info("--build-version: %s", self.parsed_arguments.build_version)
            subprocess.run(['sed', '-i', f"s/^Version=.*/Version={self.parsed_arguments.build_version}/", "./package/conf/version.info"], check=True)
        # 在非 local 场景下按需更新依赖；在 local 场景下仅使用本地已有代码，不更新依赖。
        if 'local' not in self.parsed_arguments.command:
            from download_dependencies import DependencyManager
            DependencyManager(self.parsed_arguments).run()

        if 'test' in self.parsed_arguments.command:
            # -------------------- 单元测试 --------------------
            unit_test_build_dir = self.project_root / "build_ut"
            unit_test_build_dir.mkdir(exist_ok=True)
            os.chdir(unit_test_build_dir)

            self._execute_command([
                "cmake", "-G", self._get_cmake_generator(),
                "-DCMAKE_BUILD_TYPE=Release",
                "-DENABLE_LLDB_TESTS=ON",
                ".."
            ])
        else:
            # -------------------- 产品构建 --------------------
            product_build_dir = self.project_root / "build"
            product_build_dir.mkdir(exist_ok=True)
            os.chdir(product_build_dir)

            self._execute_command([
                "cmake", "-G", self._get_cmake_generator(),
                "-DCMAKE_BUILD_TYPE=Release",
                "-DENABLE_LLDB_TESTS=OFF",
                ".."
            ])
        # 自动选择ninja还是make构建
        self._execute_command(["cmake", "--build", "."])


if __name__ == "__main__":
    try:
        BuildManager().run()
    except Exception:
        logging.error(f"Unexpected error: {traceback.format_exc()}")
        sys.exit(1)
