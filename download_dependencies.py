#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

import argparse
import hashlib
import json
import logging
import shutil
import subprocess
import sys
import tempfile
import traceback
from pathlib import Path

class DependencyManager:
    """
    依赖下载管理：根据 dependencies.json 拉取源码仓(Git submodule)与二进制包(artifacts)。

    用法:
        python3 download_dependencies.py                  下载生产依赖：源码仓(Git submodule) + 二进制包(artifacts)
        python3 download_dependencies.py test             下载测试依赖：源码仓(Git submodule) + 二进制包(artifacts)
        python3 download_dependencies.py local            跳过所有下载：直接返回不处理
        python3 download_dependencies.py -r <revision>    指定内部源码仓的 Git 分支/标签/commit

    参数说明:
        - 参数: command : 执行模式: 为空时下载生产依赖, test 为下载测试依赖, local 为跳过下载。
        - 参数: -r, --revision : 指定 Git 修订版本或标签用于依赖检出。
    """

    def __init__(self, args, need_prebuilt=False):
        self.args, self.root = args, Path(__file__).resolve().parent
        self.config = json.loads((self.root / "dependencies.json").read_text())
        self.mode = "test" if "test" in args.command else "prod"
        self.is_local = "local" in args.command
        # 源码模式（USE_PREBUILT_LLVM=OFF）不需要 prebuilt 包，仅拉 submodule；
        # --use-prebuilt 或独立运行 download_dependencies.py 时才下载 artifact。
        self.need_prebuilt = need_prebuilt

    def _exec_shell_cmd(self, cmd, cwd=None, msg=None):
        if msg: logging.info(msg)
        try:
            logging.info(f"Run CMD: {' '.join(cmd)}")
            subprocess.run(cmd, cwd=cwd, check=True)
        except Exception as e:
            logging.error(f"Error executing command: {' '.join(cmd)}")
            raise e

    def _download_submodule_recursively(self, path):
        mod_dir = self.root / path
        script = mod_dir / "download_dependencies.py"
        if not script.exists(): return

        logging.info(f"Download submodule: {path}")
        if self.args.revision:
            self._exec_shell_cmd(["git", "config", "remote.origin.fetch", "+refs/heads/*:refs/remotes/origin/*"], cwd=mod_dir)
            self._exec_shell_cmd(["git", "fetch", "--tags", "--all"], cwd=mod_dir)
            self._exec_shell_cmd(["git", "checkout", self.args.revision], cwd=mod_dir)

        cmd = [sys.executable, script.name] + (
            ["-r", self.args.revision] if self.args.revision else []) + self.args.command
        self._exec_shell_cmd(cmd, cwd=mod_dir)

    def proc_submodule(self, submodules, force_latest_submodules):
        logging.info("=== Download git submodules start ===")
        third = [m for m in submodules if m.startswith("third") and "party" in m]
        builtin = [m for m in submodules if m not in third]
        base = ["git", "submodule", "update", "--init", "--progress", "--depth=1", "--jobs=4"]

        traditional_third = [x for x in third if x not in force_latest_submodules]
        if traditional_third:
            self._exec_shell_cmd(base + ["--recursive"] + traditional_third, msg="Fetching third-party submodules...")
        if force_latest_submodules:
            self._exec_shell_cmd(base + ["--recursive", "--remote"] + force_latest_submodules, msg="Fetching third-party submodules...")

        if builtin:
            self._exec_shell_cmd(base + ["--remote"] + builtin, msg="Fetching built-in submodules...")
            for m in builtin:
                self._download_submodule_recursively(m)
        logging.info("=== Download git submodules end ===")

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

    def _resolve_artifacts(self, artifacts):
        """解析 artifact 名中的 {arch} 占位：按当前机器架构展开。

        例：msdebug-prebuilt-llvm-lldb-{arch} → msdebug-prebuilt-llvm-lldb-aarch64（aarch64 机器）
        """
        arch = self._detect_arch()
        resolved = []
        for name in artifacts:
            if "{arch}" in name:
                resolved.append(name.replace("{arch}", arch))
            else:
                resolved.append(name)
        return resolved, arch

    def proc_artifact(self, artifacts, spec):
        logging.info("=== Download artifacts start ===")
        artifacts, arch = self._resolve_artifacts(artifacts)
        for name in artifacts:
            if name not in spec:
                logging.error(f"artifact spec 中不存在 {name}")
                sys.exit(1)
            if not spec[name].get("url"):
                logging.error(
                    f"架构 {arch} 的 prebuilt 包未发布：{name}。请在对应架构机器上执行 "
                    f"`python build.py prebuild` 后将产物上传 release 并填写 dependencies.json 的 url/sha256")
                sys.exit(1)
            target = self.root / spec[name]["path"]
            if target.exists() and any(target.iterdir()):
                logging.info(f"Skip existing: {name}")
                continue

            url, sha = spec[name]["url"], spec[name].get("sha256")
            with tempfile.TemporaryDirectory() as td:
                archive_path = Path(td) / Path(url).name
                self._exec_shell_cmd(["curl", "-Lfk", "--retry", "5", "--retry-delay", "2",
                                      "-o", str(archive_path), url], msg=f"Download {name} ...")
                if sha and hashlib.sha256(archive_path.read_bytes()).hexdigest() != sha:
                    sys.exit(f"SHA256 mismatch for {name}")

                extract_path = Path(td) / "extract"
                try:
                    extract_path.mkdir(parents=True, exist_ok=True)
                    self._exec_shell_cmd(["tar", "-xf", str(archive_path), "-C", str(extract_path)],
                                         msg=f"Unzip {name}, please wait...")
                except Exception as e:
                    logging.warning(f"tar exec fail, falling back to shutil.unpack_archive, err:{e}")
                    shutil.unpack_archive(archive_path, extract_path)  # tar解压更快，如果失败，再用此接口保护解压

                items = list(extract_path.iterdir())
                source = items[0] if len(items) == 1 and items[0].is_dir() else extract_path  # 处理常见“单一顶层目录”情况

                if target.exists():
                    shutil.rmtree(target)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.move(str(source), str(target))
                logging.info(f"Download {name} successfully!")
        logging.info("=== Download artifacts end ===")

    def run(self):
        if self.is_local:
            logging.info("Local mode: Skipping downloads.")
            return

        submodules = self.config["dependency_sets"][self.mode].get("submodules", [])
        # 特殊功能：通常情况下，第三方仓库仅会检出父仓库所记录的固定 commit；若在特殊场景下需拉取其最新代码，则可将三方仓库配置到此数组。
        force_latest_submodules = self.config["dependency_sets"][self.mode].get("force_latest_submodules", [])
        artifacts = self.config["dependency_sets"][self.mode].get("artifacts", [])
        spec = self.config.get("artifact_spec", {})

        if self.need_prebuilt and artifacts:
            self.proc_artifact(artifacts, spec)

        if submodules:
            self.proc_submodule(submodules, force_latest_submodules)

def main():
    logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')
    parser = argparse.ArgumentParser(description='Download project dependencies (git submodules and artifacts) based on dependencies.json')
    parser.add_argument('command', nargs='*', default=[], choices=[[], 'local', 'test'],
                        help='Execution mode: omit to download prod dependencies, "local" to skip downloads, "test" to download test dependencies')
    parser.add_argument('-r', '--revision', help="Specify Git revision for internal dependent repo.")
    try:
        # 独立运行本脚本时视为需要 prebuilt 包（全量下载）；由 build.py 调用时
        # 会显式传 need_prebuilt=是否 --use-prebuilt。
        DependencyManager(parser.parse_args(), need_prebuilt=True).run()
        logging.info("")
        logging.info("=" * 50)
        logging.info("  ALL DEPENDENCIES DOWNLOADED SUCCESSFULLY!   ")
        logging.info("=" * 50)
        logging.info("")
    except Exception as _:
        logging.error(f"Unexpected error: {traceback.format_exc()}")
        sys.exit(1)

if __name__ == "__main__":
    main()
