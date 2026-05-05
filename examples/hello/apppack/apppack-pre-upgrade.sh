#!/bin/bash
# apppack-pre-upgrade.sh - 升级/降级前执行脚本
# 在安装新版本之前执行，用于清理旧的系统环境配置
# 这样新的 post-install 脚本可以重新设置新的配置
# 注意：此脚本仅在升级或降级安装时执行，首次安装不执行
#
# 用户配置处理：
#   升级时保留用户配置（~/.config/HelloWorld/ 等）
#   只清理系统级的环境配置

echo "  升级前清理旧配置..."
echo "    应用: ${APPPACK_APP_NAME}"
echo "    新版本: ${APPPACK_VERSION}"
echo "    安装目录: ${APPPACK_INSTALL_DIR}"

# 清理旧的系统环境配置
# 删除之前 post-install 创建的环境变量文件
rm -f /etc/profile.d/hello.sh

# 删除之前 post-install 创建的符号链接
rm -f /usr/local/bin/hello

# 注意：用户配置（~/.config/HelloWorld/）在升级时保留
# 这样用户的自定义设置不会丢失

exit 0
