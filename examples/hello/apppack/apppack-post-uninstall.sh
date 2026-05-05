#!/bin/bash
# apppack-post-uninstall.sh - HelloWorld 卸载后清理脚本
# 在卸载文件之后执行，可用于清理环境变量、删除配置等
#
# 用户配置处理策略：
#   默认保留用户配置（~/.config/HelloWorld/ 等）
#   设置 APPPACK_PURGE_CONFIG=true 可清理所有用户配置

echo "  卸载后清理..."
echo "    应用: ${APPPACK_APP_NAME}"

# ========== 清理系统环境配置 ==========
# 删除环境变量文件
rm -f /etc/profile.d/hello.sh

# 删除符号链接
rm -f /usr/local/bin/hello

# ========== 处理用户配置 ==========
PURGE=false
if [ "${APPPACK_PURGE_CONFIG}" = "true" ]; then
    PURGE=true
fi

if [ "$PURGE" = true ]; then
    echo "  正在清理用户配置..."
    
    # 清理所有用户的配置
    for user_home in /home/*; do
        if [ -d "${user_home}" ]; then
            USER_CONFIG_DIR="${user_home}/.config/${APPPACK_APP_NAME}"
            USER_DATA_DIR="${user_home}/.local/share/${APPPACK_APP_NAME}"
            USER_CACHE_DIR="${user_home}/.cache/${APPPACK_APP_NAME}"
            USER_STATE_DIR="${user_home}/.local/state/${APPPACK_APP_NAME}"
            
            [ -d "${USER_CONFIG_DIR}" ] && rm -rf "${USER_CONFIG_DIR}" && echo "    已删除: ${USER_CONFIG_DIR}"
            [ -d "${USER_DATA_DIR}" ] && rm -rf "${USER_DATA_DIR}" && echo "    已删除: ${USER_DATA_DIR}"
            [ -d "${USER_CACHE_DIR}" ] && rm -rf "${USER_CACHE_DIR}" && echo "    已删除: ${USER_CACHE_DIR}"
            [ -d "${USER_STATE_DIR}" ] && rm -rf "${USER_STATE_DIR}" && echo "    已删除: ${USER_STATE_DIR}"
        fi
    done
    
    # 清理 root 配置
    [ -d "/root/.config/${APPPACK_APP_NAME}" ] && rm -rf "/root/.config/${APPPACK_APP_NAME}"
    [ -d "/root/.local/share/${APPPACK_APP_NAME}" ] && rm -rf "/root/.local/share/${APPPACK_APP_NAME}"
    [ -d "/root/.cache/${APPPACK_APP_NAME}" ] && rm -rf "/root/.cache/${APPPACK_APP_NAME}"
    
    # 清理 /etc/skel 模板
    [ -d "/etc/skel/.config/${APPPACK_APP_NAME}" ] && rm -rf "/etc/skel/.config/${APPPACK_APP_NAME}"
    
    echo "  ✓ 用户配置已清理"
else
    echo "  用户配置已保留（使用 APPPACK_PURGE_CONFIG=true 可清理）"
    echo "    配置文件: ~/.config/${APPPACK_APP_NAME}/"
    echo "    数据文件: ~/.local/share/${APPPACK_APP_NAME}/"
    echo "    缓存文件: ~/.cache/${APPPACK_APP_NAME}/"
fi

echo "  ✓ 卸载后清理完成"
exit 0
