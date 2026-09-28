#!/usr/bin/env bash
# =============================================================================
# clang-tidy wrapper — checks compile_commands.json before running clang-tidy
# =============================================================================
set -euo pipefail

BUILD_DIRS=("build")

found_dir=""
for dir in "${BUILD_DIRS[@]}"; do
    if [[ -f "${dir}/compile_commands.json" ]]; then
        found_dir="${dir}"
        break
    fi
done

# 编译库缺失时跳过 clang-tidy（例如 CI 的 pre-commit job 不执行构建），
# 避免因缺少 compile_commands.json 直接判失败。
if [[ -z "${found_dir}" ]]; then
    cat >&2 <<'EOF'
=======================================================================
  WARNING: compile_commands.json NOT FOUND, skip clang-tidy
-----------------------------------------------------------------------
  clang-tidy requires a compilation database to work. Generate it via:

    python3 build.py

  (This will create compile_commands.json under the build/ directory.)
  clang-tidy is skipped for this run.
=======================================================================
EOF
    exit 0
fi

exec clang-tidy "$@"
