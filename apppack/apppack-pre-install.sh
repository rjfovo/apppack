#!/bin/bash
# AppPack 工具链预安装脚本
# 检查系统环境

echo "正在准备安装 AppPack 工具链..."

# 检查必要工具
MISSING=""
for tool in cmake g++ make zstd; do
    if ! command -v "$tool" &> /dev/null; then
        MISSING="$MISSING $tool"
    fi
done

if [ -n "$MISSING" ]; then
    echo "警告: 以下工具未安装，AppPack 打包功能可能需要它们:"
    for tool in $MISSING; do
        echo "  - $tool"
    done
    echo ""
    echo "建议安装:"
    echo "  Ubuntu/Debian: sudo apt-get install cmake g++ make zstd"
    echo "  Fedora:        sudo dnf install cmake gcc-c++ make zstd"
    echo "  Arch:          sudo pacman -S cmake gcc make zstd"
fi

exit 0
