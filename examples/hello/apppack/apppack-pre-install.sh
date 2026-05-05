#!/bin/bash
# apppack-pre-install.sh - HelloWorld 安装前检查脚本
# 在安装文件之前执行，可用于检查环境

echo "  安装前检查..."
echo "    应用: ${APPPACK_APP_NAME}"
echo "    版本: ${APPPACK_VERSION}"
echo "    安装目录: ${APPPACK_INSTALL_DIR}"

# 检查磁盘空间（示例）
# REQUIRED_SPACE=10  # MB
# AVAILABLE_SPACE=$(df "${APPPACK_INSTALL_DIR}" 2>/dev/null | awk 'NR==2 {print $4}')
# if [ -n "$AVAILABLE_SPACE" ] && [ "$AVAILABLE_SPACE" -lt "$((REQUIRED_SPACE * 1024))" ]; then
#     echo "错误: 磁盘空间不足，需要 ${REQUIRED_SPACE}MB"
#     exit 1
# fi

echo "  ✓ 环境检查通过"
exit 0
