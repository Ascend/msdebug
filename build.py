#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
import os
import sys
import logging
import subprocess
import argparse


def exec_cmd(cmd, env=None, shell=False):
    try:
        result = subprocess.run(
            cmd, 
            capture_output=False, 
            text=True, 
            timeout=360000,
            env=env if env else os.environ, 
            shell=shell
        )
        if result.returncode != 0:
            logging.error("Execute command failed: %s", " ".join(cmd) if not shell else cmd)
            sys.exit(result.returncode)
    except subprocess.TimeoutExpired:
        logging.error("Command timed out: %s", " ".join(cmd) if not shell else cmd)
        sys.exit(1)
    except Exception as e:
        logging.error("Failed to execute command %s: %s", " ".join(cmd) if not shell else cmd, str(e))
        sys.exit(1)       


def update_submodle(args):
        logging.info("============ start download thirdparty code using git submodule ============")
        if 'test' not in args.command and 'fuzz' not in args.command:
            # 只构建打包时，只需下载构建所需子仓即可
            exec_cmd(["git", "submodule", "update", "--init", "--depth=1", "--jobs=4",
                   "third-party/libedit", "third-party/ncurses", "third-party/makeself"])
        else:
            exec_cmd(["git", "submodule", "update", "--init", "--recursive", "--depth=1", "--jobs=4"])      
        logging.info("============ download thirdparty code success ============")


def execute_build(build_path):
    try:
        if not os.path.exists(build_path):
            os.makedirs(build_path, mode=0o755)
        os.chdir(build_path)
    except Exception as e:
        logging.error("Failed to execute build: %s", str(e))
        sys.exit(1)


def execute_compile(args):
    try:
        current_dir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
        llvm_build_dir = os.path.join(current_dir, 'build')
        os.makedirs(llvm_build_dir, exist_ok=True)
        os.chdir(llvm_build_dir)
        cmake_args = [
            '-G', 'Ninja',
            '-DCMAKE_BUILD_TYPE=Release',
            f'-DENABLE_LLDB_TESTS={"ON" if "test" in args.command else "OFF"}',
            '..' 
        ]
        ninja_targets = []
        if "test" in args.command:
            ninja_targets.extend(['check-lldb-unit'])
        logging.info("Configuring project with CMake...")
        exec_cmd(["cmake"] + cmake_args)
        logging.info("Building targets: %s", ninja_targets if ninja_targets else "[default]")
        exec_cmd(["ninja"] + ninja_targets)
    except Exception as e:
        logging.error("Build failed: %s", str(e), exc_info=True)
        sys.exit(1)


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    parser = argparse.ArgumentParser(description='Build script with optional testing')
    parser.add_argument('command', nargs='*', default='',
                      choices=['', 'local', 'test'],
                      help='Command to execute (python build.py [local] [test]):\n')
    args = parser.parse_args()

    current_dir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
    os.chdir(current_dir)

    build_path = os.path.join(current_dir, "build")

    # 解析入参是否为local，非local场景时按需更新代码；local场景不更新代码只使用本地代码
    if 'local' not in args.command:
        update_submodle(args)
    # 执行构建并打run包
    execute_compile(args)
    execute_build(build_path)
