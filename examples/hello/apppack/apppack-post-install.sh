#!/bin/bash
# apppack-post-install.sh - HelloWorld 安装后配置脚本
# 在安装文件之后执行，可用于创建符号链接、注册服务、初始化用户配置等

echo "  安装后配置..."
echo "    应用: ${APPPACK_APP_NAME}"
echo "    版本: ${APPPACK_VERSION}"
echo "    安装目录: ${APPPACK_INSTALL_DIR}"

# ========== 初始化默认用户配置 ==========
DEFAULT_CONFIG_DIR="/etc/skel/.config/${APPPACK_APP_NAME}"
mkdir -p "${DEFAULT_CONFIG_DIR}"

# 写入默认配置文件
cat > "${DEFAULT_CONFIG_DIR}/settings.conf" << 'CONF'
# HelloWorld 默认配置
[General]
# 应用主题 (default / dark / light)
theme=default
# 界面语言 (system / zh_CN / en_US)
language=system
# 是否自动保存
auto_save=true

[Window]
# 窗口大小
width=800
height=600
# 启动时是否最大化
maximized=false

[Recent]
# 最近打开的文件列表（最多 10 个）
max_recent_files=10
CONF

echo "  ✓ 已创建默认配置模板: ${DEFAULT_CONFIG_DIR}/settings.conf"

# 为当前已登录的用户创建配置（如果有的话）
for user_home in /home/*; do
    if [ -d "${user_home}" ]; then
        USER_CONFIG_DIR="${user_home}/.config/${APPPACK_APP_NAME}"
        if [ ! -d "${USER_CONFIG_DIR}" ]; then
            mkdir -p "${USER_CONFIG_DIR}"
            cp "${DEFAULT_CONFIG_DIR}/settings.conf" "${USER_CONFIG_DIR}/"
            chown -R $(stat -c "%u:%g" "${user_home}") "${USER_CONFIG_DIR}"
            echo "  ✓ 已为用户初始化配置: ${USER_CONFIG_DIR}"
        fi
    fi
done

# 如果 root 也有配置目录
ROOT_CONFIG="/root/.config/${APPPACK_APP_NAME}"
if [ ! -d "${ROOT_CONFIG}" ]; then
    mkdir -p "${ROOT_CONFIG}"
    cp "${DEFAULT_CONFIG_DIR}/settings.conf" "${ROOT_CONFIG}/"
    echo "  ✓ 已为 root 初始化配置: ${ROOT_CONFIG}"
fi

echo "  ✓ 安装后配置完成"
exit 0
