# -*- Python -*-

# Configuration file for the 'lit' test runner.

import os
import sys

import lit.formats
from lit.llvm import llvm_config

# name: The name of this test suite.
config.name = "lldb-unit"

# suffixes: A list of file extensions to treat as test files.
config.suffixes = []

# test_source_root: The root path where unit test binaries are located.
# test_exec_root: The root path where tests should be run.
config.test_source_root = os.path.join(config.lldb_obj_root, "unittests")
config.test_exec_root = config.test_source_root

# One of our unit tests dynamically links against python.dll, and on Windows
# it needs to be able to find it at runtime.  This is fine if Python is on your
# system PATH, but if it's not, then this unit test executable will fail to run.
# We can solve this by forcing the Python directory onto the system path here.
llvm_config.with_system_environment(
    [
        "HOME",
        "PATH",
        "TEMP",
        "TMP",
        "XDG_CACHE_HOME",
    ]
)
llvm_config.with_environment("PATH", os.path.dirname(sys.executable), append_path=True)
llvm_config.with_environment(
    "LD_LIBRARY_PATH", os.path.join(config.llvm_obj_root, "lib"), append_path=True
)

# ---------------------------------------------------------------------------
# 根因：liblldb.so 链接时依赖了 GCC 11 工具链自带的 libtinfow.so.6（ncurses
# wide-char, ABI v6），而非工程自建 ncurses 的 libtinfo.so.5（无 wide-char,
# ABI v5）。该库位于 /opt/gcc11-glibc2.17-deps/lib64，链接器通过编译环境变量
# LIBRARY_PATH 找到它（链接成功），但 CMake 将其视为编译器隐式系统路径，不会
# 为它生成 RPATH。运行时动态链接器搜索不到该路径，导致测试二进制启动失败。
#
# 修复思路：从 LIBRARY_PATH 中精确定位包含 libtinfow.so.6 的目录，注入测试
# 进程的 LD_LIBRARY_PATH。不能简单追加整个 LIBRARY_PATH —— 其中 devtoolset-7
# 等路径的库版本与编译产物不兼容，会导致 SIGSEGV。
# ---------------------------------------------------------------------------
for path in os.environ.get("LIBRARY_PATH", "").split(":"):
    if path and os.path.isdir(path) and os.path.isfile(os.path.join(path, "libtinfow.so.6")):
        llvm_config.with_environment("LD_LIBRARY_PATH", path, append_path=True)
        break

# Enable sanitizer runtime flags.
config.environment["ASAN_OPTIONS"] = "detect_stack_use_after_return=1"
config.environment["TSAN_OPTIONS"] = "halt_on_error=1"

# testFormat: The test format to use to interpret tests.
config.test_format = lit.formats.GoogleTest(config.llvm_build_mode, "Tests")
