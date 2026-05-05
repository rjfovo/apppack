#!/bin/bash
# AppPack 工具链卸载后清理脚本
# 移除符号链接，处理用户配置
#
# 用户配置处理策略：
#   默认保留用户配置（~/.config/AppPack/ 等）
#   设置 APPPACK_PURGE_CONFIG=true 可清理所有用户配置

INSTALL_DIR="${APPPACK_INSTALL_DIR:-/opt/AppPack}"

echo "正在清理 AppPack 工具链..."

# 移除符号链接
if [ -d "${INSTALL_DIR}/usr/bin" ]; then
    for exe in "${INSTALL_DIR}/usr/bin/"*; do
        if [ -f "$exe" ] && [ -x "$exe" ]; then
            name=$(basename "$exe")
            if [ -L "/usr/local/bin/${name}" ]; then
                rm -f "/usr/local/bin/${name}"
                echo "  已移除符号链接: /usr/local/bin/${name}"
            fi
        fi
    done
fi

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
            USER_CONFIG_DIR="${user_home}/.config/AppPack"
            USER_DATA_DIR="${user_home}/.local/share/AppPack"
            USER_CACHE_DIR="${user_home}/.cache/AppPack"
            USER_STATE_DIR="${user_home}/.local/state/AppPack"
            
            [ -d "${USER_CONFIG_DIR}" ] && rm -rf "${USER_CONFIG_DIR}" && echo "    已删除: ${USER_CONFIG_DIR}"
            [ -d "${USER_DATA_DIR}" ] && rm -rf "${USER_DATA_DIR}" && echo "    已删除: ${USER_DATA_DIR}"
            [ -d "${USER_CACHE_DIR}" ] && rm -rf "${USER_CACHE_DIR}" && echo "    已删除: ${USER_CACHE_DIR}"
            [ -d "${USER_STATE_DIR}" ] && rm -rf "${USER_STATE_DIR}" && echo "    已删除: ${USER_STATE_DIR}"
        fi
    done
    
    # 清理 root 配置
    [ -d "/root/.config/AppPack" ] && rm -rf "/root/.config/AppPack"
    [ -d "/root/.local/share/AppPack" ] && rm -rf "/root/.local/share/AppPack"
    [ -d "/root/.cache/AppPack" ] && rm -rf "/root/.cache/AppPack"
    
    # 清理 /etc/skel 模板
    [ -d "/etc/skel/.config/AppPack" ] && rm -rf "/etc/skel/.config/AppPack"
    
    echo "  ✓ 用户配置已清理"
else
    echo "  用户配置已保留（使用 APPPACK_PURGE_CONFIG=true 可清理）"
    echo "    配置文件: ~/.config/AppPack/"
    echo "    数据文件: ~/.local/share/AppPack/"
    echo "    缓存文件: ~/.cache/AppPack/"
fi

echo "AppPack 工具链已卸载完成。"

exit 0
