#!/bin/bash
# AppPack 工具链后安装脚本
# 创建符号链接到系统 PATH，初始化用户配置

INSTALL_DIR="${APPPACK_INSTALL_DIR:-/opt/AppPack}"

echo "正在配置 AppPack 工具链..."

# 创建符号链接到 /usr/local/bin
if [ -d "${INSTALL_DIR}/usr/bin" ]; then
    for exe in "${INSTALL_DIR}/usr/bin/"*; do
        if [ -f "$exe" ] && [ -x "$exe" ]; then
            name=$(basename "$exe")
            ln -sf "$exe" "/usr/local/bin/${name}"
            echo "  已创建符号链接: /usr/local/bin/${name}"
        fi
    done
fi

# ========== 初始化默认用户配置 ==========
DEFAULT_CONFIG_DIR="/etc/skel/.config/AppPack"
mkdir -p "${DEFAULT_CONFIG_DIR}"

# 写入默认配置文件
cat > "${DEFAULT_CONFIG_DIR}/apppack.conf" << 'CONF'
# AppPack 工具链默认配置
[General]
# 默认打包输出目录
output_dir=.
# 是否默认打包依赖库
bundle_deps=false
# 压缩级别 (1-19, 默认 15)
compression_level=15

[Builder]
# 默认安装目录前缀
default_install_prefix=/opt
# 是否生成调试信息
verbose=false

[Template]
# 默认作者信息
author=
# 默认应用类别
default_categories=Utility;
CONF

echo "  ✓ 已创建默认配置模板: ${DEFAULT_CONFIG_DIR}/apppack.conf"

# 为当前已登录的用户创建配置（如果有的话）
for user_home in /home/*; do
    if [ -d "${user_home}" ]; then
        USER_CONFIG_DIR="${user_home}/.config/AppPack"
        if [ ! -d "${USER_CONFIG_DIR}" ]; then
            mkdir -p "${USER_CONFIG_DIR}"
            cp "${DEFAULT_CONFIG_DIR}/apppack.conf" "${USER_CONFIG_DIR}/"
            chown -R $(stat -c "%u:%g" "${user_home}") "${USER_CONFIG_DIR}"
            echo "  ✓ 已为用户初始化配置: ${USER_CONFIG_DIR}"
        fi
    fi
done

# 如果 root 也有配置目录
ROOT_CONFIG="/root/.config/AppPack"
if [ ! -d "${ROOT_CONFIG}" ]; then
    mkdir -p "${ROOT_CONFIG}"
    cp "${DEFAULT_CONFIG_DIR}/apppack.conf" "${ROOT_CONFIG}/"
    echo "  ✓ 已为 root 初始化配置: ${ROOT_CONFIG}"
fi

echo ""
echo "AppPack 工具链安装完成！"
echo "现在可以在终端中直接使用以下命令:"
echo "  app-builder --help         - 查看打包工具帮助"
echo "  app-builder --init         - 初始化项目模板"
echo "  app-builder --build        - 构建安装包"
echo "  app-installer --help       - 查看安装器帮助"
echo ""
echo "用户配置文件:"
echo "  ~/.config/AppPack/apppack.conf  - 工具链配置"
echo ""
echo "示例:"
echo "  cd /path/to/your/project"
echo "  app-builder --init"
echo "  # 编辑 apppack/ 目录下的配置文件"
echo "  app-builder --build"
echo ""
echo "也可以通过 AppRun 调度脚本调用:"
echo "  /opt/AppPack/AppRun --build     # 调用打包工具"
echo "  /opt/AppPack/AppRun --install   # 调用安装器"
echo ""

exit 0
