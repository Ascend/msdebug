#!/bin/bash

# ==============================================================================
# 脚本名称: install-vscode-server.sh
# 描述: 自动下载并解压 VS Code Server 到指定目录
# 目的：用于当主机网速较慢时使用，当自动安装vscode-server花费较多时间甚至失败时考虑使用
# ==============================================================================

# 颜色定义
GREEN=$'\033[1;32m'
YELLOW=$'\033[1;33m'
RED=$'\033[1;31m'
RESET=$'\033[0m'

COMMIT_ID="a5b500951314efd502d07465bd138dfbd714a960"
# 目标安装路径（请根据实际用户名调整）
TARGET_DIR="/home/mindstudio/.vscode-server/bin/${COMMIT_ID}"
# 下载地址（使用国内镜像加速）
DOWNLOAD_URL="https://update.code.visualstudio.com/commit:${COMMIT_ID}/server-linux-arm64/stable"
# 临时下载文件名
TAR_FILE="vscode-server-linux-x64.tar.gz"
# ==========================================================

# 1. 创建目标目录
echo -e "${GREEN}[1/4]${RESET} 正在创建目标目录..."
mkdir -p "${TARGET_DIR}"

# 2. 下载 VS Code Server
echo -e "${GREEN}[2/4]${RESET} 正在从镜像站下载 VS Code Server..."
if wget --tries=3 --timeout=30 -O "${TAR_FILE}" "${DOWNLOAD_URL}"; then
    echo -e "${GREEN}      -> 下载完成！${RESET}"
else
    echo -e "${RED}[✗] 下载失败，请检查网络连接或 Commit ID 是否正确。${RESET}"
    exit 1
fi

# 3. 解压到目标目录
echo -e "${GREEN}[3/4]${RESET} 正在解压文件到目标目录..."
if tar -xzf "${TAR_FILE}" -C "${TARGET_DIR}" --strip-components 1; then
    echo -e "${GREEN}      -> 解压成功！${RESET}"
else
    echo -e "${RED}[✗] 解压失败，压缩包可能已损坏。${RESET}"
    rm -f "${TAR_FILE}"
    exit 1
fi

# 4. 清理临时文件
echo -e "${GREEN}[4/4]${RESET} 正在清理临时文件..."
rm -f "${TAR_FILE}"
