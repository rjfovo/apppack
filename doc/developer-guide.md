# AppPack 开发者指南 - 如何打包你的应用

## 目录

1. [概述](#1-概述)
2. [准备工作](#2-准备工作)
3. [快速开始：使用项目模板（推荐）](#3-快速开始使用项目模板推荐)
4. [手动创建 AppDir 目录结构](#4-手动创建-appdir-目录结构)
5. [多可执行文件支持](#5-多可执行文件支持)
6. [使用 app-builder 打包](#6-使用-app-builder-打包)
7. [高级打包选项](#7-高级打包选项)
8. [处理动态库依赖](#8-处理动态库依赖)
9. [桌面集成配置](#9-桌面集成配置)
10. [安装生命周期脚本](#10-安装生命周期脚本)
11. [用户配置文件管理](#11-用户配置文件管理)
12. [版本管理与更新](#12-版本管理与更新)
13. [完整示例](#13-完整示例)
14. [常见问题](#14-常见问题)

---

## 1. 概述

AppPack 是一种自解压安装包格式，类似于 AppImage，但更接近 Windows 的安装体验：

- **自包含** - 应用及其依赖打包在一个文件中
- **跨发行版** - 不依赖特定包管理器（deb/rpm）
- **安装到指定目录** - 像 Windows 一样安装到 `/opt/` 或自定义目录
- **桌面集成** - 自动创建桌面图标和菜单项
- **干净卸载** - 一键删除所有安装文件和桌面集成

### 工作流程

```
你的应用目录 (AppDir)  ──→  app-builder  ──→  MyApp.apppack
                          (打包工具)              (自解压安装包)
```

---

## 2. 准备工作

### 2.1 安装构建工具

```bash
# Ubuntu/Debian
sudo apt-get install cmake g++ make zstd

# Fedora
sudo dnf install cmake gcc-c++ make zstd

# Arch Linux
sudo pacman -S cmake gcc make zstd
```

### 2.2 获取 AppPack 工具链

```bash
# 克隆或下载 AppPack 项目
git clone <repository-url> apppack
cd apppack

# 构建工具链
./build.sh
```

构建完成后，你会得到：

| 文件 | 说明 |
|------|------|
| `build/app-builder` | 打包工具（用于将应用目录打包成 .apppack） |
| `build/HelloWorld.apppack` | 示例安装包 |

---

## 3. 快速开始：使用项目模板（推荐）

AppPack 提供了类似 deb 包 `debian/` 目录的项目模板功能，可以快速初始化打包配置，无需手动创建 AppDir 结构。

### 3.1 初始化项目模板

在你的项目目录中运行：

```bash
# 进入你的项目目录
cd /path/to/your/project

# 初始化 AppPack 模板
app-builder --init
```

工具会交互式地询问应用信息（也可以直接回车使用默认值）：

```
请输入应用信息（直接回车使用默认值）:
  应用名称 [MyApp]: HelloWorld
  版本号 [1.0.0]: 2.0.0
  应用描述 [我的应用程序]: 一个示例应用
  安装目录 [/opt/HelloWorld]:
  可执行文件名称 [hello]:
  应用类别 [Utility;]:
  是否在终端中运行？ [y/N]: n
  是否自动打包依赖库？ [y/N]: y
```

### 3.2 生成的模板结构

初始化后会在项目目录下创建 `apppack/` 目录：

```
your-project/
├── ... (你的项目文件)
└── apppack/                    # AppPack 打包模板目录
    ├── apppack.json             # 打包配置文件（类似 debian/control）
    ├── AppRun                   # 应用启动脚本
    ├── HelloWorld.desktop       # 桌面文件
    ├── HelloWorld.svg           # 应用图标
    ├── apppack-pre-install.sh   # 安装前脚本
    ├── apppack-post-install.sh  # 安装后脚本
    ├── apppack-post-uninstall.sh# 卸载后脚本
    └── usr/
        ├── bin/                 # 可执行文件目录
        └── lib/                 # 依赖库目录
```

### 3.3 配置文件说明

`apppack.json` 是打包的核心配置文件，类似 deb 包的 `control` 文件：

```json
{
    "app_name": "HelloWorld",       // 应用名称
    "version": "2.0.0",             // 版本号
    "install_dir": "/opt/HelloWorld",// 安装目录
    "description": "一个示例应用",   // 应用描述
    "executable": "hello",          // 可执行文件名称
    "maintainer": "Your Name <your@email.com>",  // 维护者信息
    "categories": "Utility;",       // 应用类别
    "terminal": false,              // 是否在终端中运行
    "bundle_deps": true,            // 是否自动打包依赖
    "scripts": {
        "pre_install": "apppack-pre-install.sh",
        "post_install": "apppack-post-install.sh",
        "post_uninstall": "apppack-post-uninstall.sh"
    }
}
```

### 3.4 构建安装包

将可执行文件放入 `apppack/usr/bin/` 后，运行：

```bash
# 从 apppack/ 目录读取配置并构建
app-builder --build

# 或自动打包依赖库
app-builder --build --bundle-deps
```

构建工具会自动：
1. 读取 `apppack.json` 获取应用名称、版本号、安装目录等配置
2. 收集 `apppack/` 目录下的所有文件
3. 生成 `.apppack` 自解压安装包

### 3.5 完整工作流程

```bash
# 1. 初始化模板
cd ~/projects/myapp
app-builder --init

# 2. 编辑 apppack.json 配置（可选）
vim apppack/apppack.json

# 3. 编译你的应用
g++ -o myapp main.cpp

# 4. 将可执行文件放入模板
cp myapp apppack/usr/bin/

# 5. 构建安装包
app-builder --build --bundle-deps

# 6. 测试安装
sudo ./myapp-1.0.0.apppack --install
```

> **提示**: 查看 `examples/hello/apppack/` 目录获取一个完整的模板示例。

---

## 4. 手动创建 AppDir 目录结构

AppDir 是打包前的应用目录，包含应用的所有文件。推荐结构：

```
MyApp.AppDir/
├── AppRun                  # [必需] 应用启动脚本
├── myapp.desktop           # [必需] 桌面入口文件
├── myapp.svg               # [可选] 应用图标
└── usr/
    ├── bin/
    │   └── myapp           # [必需] 可执行文件
    ├── lib/
    │   └── libfoo.so       # [可选] 依赖库
    └── share/
        ├── icons/          # [可选] 图标
        └── applications/   # [可选] 桌面文件
```

### 4.1 AppRun 启动脚本

`AppRun` 是应用的入口脚本，必须放在 AppDir 根目录：

```bash
#!/bin/bash
# AppRun - 应用启动脚本

# 获取 AppDir 路径
APPDIR="$(dirname "$(readlink -f "$0")")"

# 设置库路径（如果有 bundled 库）
if [ -d "${APPDIR}/usr/lib" ]; then
    export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH}"
fi

# 设置安装目录环境变量（可选）
export APPPACK_INSTALL_DIR="${APPDIR}"

# 执行应用
exec "${APPDIR}/usr/bin/myapp" "$@"
```

> **注意**: `AppRun` 必须具有可执行权限（`chmod +x AppRun`）

### 4.2 Desktop 文件

桌面入口文件用于创建桌面图标和菜单项：

```ini
[Desktop Entry]
Name=MyApp
Comment=我的应用
Exec=AppRun
Icon=myapp
Terminal=false
Type=Application
Categories=Utility;
StartupNotify=true
```

> **注意**: `Exec` 应设置为 `AppRun`（相对于安装目录），`Icon` 设置为图标文件名（不含扩展名）

---

## 5. 多可执行文件支持

一个 AppPack 包可以包含多个可执行文件，无需修改打包工具或安装器。以下是三种推荐方案：

### 5.1 方案一：通过 AppRun 入口脚本调度（推荐）

AppRun 作为统一的入口点，根据调用方式分发到不同的可执行文件：

**apppack/AppRun 示例：**
```bash
#!/bin/bash
# AppRun - 应用入口调度脚本
APPDIR="$(dirname "$(readlink -f "$0")")"

# 获取用户调用的命令名
CMD=$(basename "$0")

case "$CMD" in
    AppRun|myapp-main)
        exec "$APPDIR/usr/bin/myapp-main" "$@"
        ;;
    myapp-tool)
        exec "$APPDIR/usr/bin/myapp-tool" "$@"
        ;;
    myapp-helper)
        exec "$APPDIR/usr/bin/myapp-helper" "$@"
        ;;
    *)
        # 默认启动主程序
        exec "$APPDIR/usr/bin/myapp-main" "$@"
        ;;
esac
```

**apppack/usr/bin/ 目录结构：**
```
apppack/
├── AppRun              # 入口调度脚本
├── usr/
│   └── bin/
│       ├── myapp-main     # 主程序
│       ├── myapp-tool     # 工具程序
│       └── myapp-helper   # 辅助程序
├── myapp.desktop
└── myapp.svg
```

**使用方式：**
- 双击桌面图标 → 启动主程序
- 命令行调用 `/opt/MyApp/AppRun --tool` → 启动工具程序
- 创建符号链接：`ln -s /opt/MyApp/AppRun /usr/local/bin/myapp-tool` → 直接通过命令名调用

### 5.2 方案二：直接在 usr/bin/ 放多个可执行文件

AppPack 打包时会自动收集 `usr/bin/` 下的所有文件，安装后全部放到目标目录：

```
apppack/
├── AppRun
├── usr/
│   └── bin/
│       ├── myapp          # 主程序
│       ├── myapp-tool1    # 工具1
│       └── myapp-tool2    # 工具2
├── myapp.desktop
└── myapp.svg
```

AppRun 直接启动主程序，其他工具通过完整路径调用：
```bash
/opt/MyApp/usr/bin/myapp-tool1 --help
```

### 5.3 方案三：多个 .desktop 文件

如果需要为每个工具创建独立的桌面图标，只需在 apppack/ 目录下放多个 .desktop 文件：

```
apppack/
├── AppRun
├── usr/bin/
│   ├── myapp
│   ├── myapp-editor
│   └── myapp-converter
├── myapp.desktop           # 主程序桌面图标
├── myapp-editor.desktop    # 编辑器桌面图标
├── myapp-converter.desktop # 转换器桌面图标
└── myapp.svg
```

安装器会自动处理所有 `.desktop` 文件，为每个创建独立的菜单项。

> **推荐使用方案一**，它保持入口统一，可以通过符号链接实现命令级调用，且与现有的桌面集成机制完全兼容。

---

## 6. 使用 app-builder 打包

### 6.1 基本打包命令

```bash
./build/app-builder \
    --appdir=/path/to/MyApp.AppDir \
    --output=MyApp.apppack
```

### 6.2 指定安装目录

```bash
./build/app-builder \
    --appdir=/path/to/MyApp.AppDir \
    --output=MyApp.apppack \
    --install-dir="/opt/MyApp"
```

### 6.3 查看打包工具帮助

```bash
./build/app-builder --help
```

输出：

```
AppPack Builder - 创建自解压安装包

用法: app-builder --appdir=<目录> --output=<输出文件> [选项]

必要参数:
  --appdir=<目录>    要打包的应用目录路径
  --output=<文件>    输出的安装包路径

可选参数:
  --install-dir=<路径>  默认安装目录 (默认: /opt/<应用名>)
  --version=<版本号>    应用版本号 (默认: 1.0.0)
  --bundle-deps         自动收集并打包动态库依赖
  --help                显示此帮助信息
```

---

## 7. 高级打包选项

### 7.1 指定版本号

```bash
./build/app-builder \
    --appdir=/path/to/MyApp.AppDir \
    --output=MyApp-v2.0.apppack \
    --version=2.0.0
```

### 7.2 打包并包含依赖

```bash
./build/app-builder \
    --appdir=/path/to/MyApp.AppDir \
    --output=MyApp.apppack \
    --bundle-deps
```

### 7.3 完整打包示例

```bash
./build/app-builder \
    --appdir=/path/to/MyApp.AppDir \
    --output=MyApp-v2.0.apppack \
    --install-dir="/opt/MyApp" \
    --version=2.0.0 \
    --bundle-deps
```

---

## 8. 处理动态库依赖

### 8.1 自动收集依赖

使用 `--bundle-deps` 参数，app-builder 会自动：

1. 扫描 AppDir 中所有 ELF 文件
2. 使用 `ldd` 分析动态库依赖
3. 递归收集所有依赖库（最多 10 层，防止循环依赖）
4. 将依赖库打包到 `usr/lib/` 目录

```bash
./build/app-builder \
    --appdir=/path/to/MyApp.AppDir \
    --output=MyApp.apppack \
    --bundle-deps
```

### 8.2 手动管理依赖

如果自动收集不满足需求，可以手动将依赖库放入 AppDir：

```
MyApp.AppDir/
└── usr/
    └── lib/
        ├── libssl.so.3      # 手动放入的依赖
        ├── libcrypto.so.3
        └── libcustom.so.1
```

AppRun 启动脚本会自动设置 `LD_LIBRARY_PATH` 包含 `usr/lib`。

### 8.3 依赖分析输出示例

```
正在分析动态库依赖...
  分析: usr/bin/myapp
  收集依赖: libstdc++.so.6
  收集依赖: libssl.so.3
  收集依赖: libcrypto.so.3
  共收集 3 个依赖库
```

---

## 9. 桌面集成配置

### 9.1 桌面文件规范

桌面文件（`.desktop`）必须符合 [FreeDesktop 规范](https://specifications.freedesktop.org/desktop-entry-spec/latest/)：

```ini
[Desktop Entry]
Type=Application
Name=MyApp
Comment=我的应用
Exec=AppRun
Icon=myapp
Terminal=false
Categories=Development;
MimeType=text/plain;
StartupNotify=true
```

### 9.2 图标支持

支持的图标格式：

| 格式 | 文件名示例 | 说明 |
|------|-----------|------|
| SVG | `myapp.svg` | 矢量图标，推荐 |
| PNG | `myapp.png` | 位图图标 |
| XPM | `myapp.xpm` | 旧格式 |

图标文件放在 AppDir 根目录即可。

### 9.3 安装后的桌面集成

安装器会自动：

1. 复制 `.desktop` 文件到 `~/.local/share/applications/`
2. 复制图标到 `~/.local/share/icons/`
3. 更新桌面数据库（如果可用）

---

## 10. 安装生命周期脚本

AppPack 支持在安装和卸载的不同阶段自动执行自定义脚本，让开发者可以处理安装前环境准备、安装后系统配置、卸载后清理等操作。

### 10.1 支持的脚本

在 AppDir 根目录放置以下脚本文件，安装器会在对应阶段自动执行：

| 脚本文件 | 执行时机 | 典型用途 |
|----------|---------|---------|
| `apppack-pre-install.sh` | 解压文件**之前** | 检查系统环境、下载资源、检查磁盘空间、备份旧配置 |
| `apppack-post-install.sh` | 解压文件和桌面集成**之后** | 设置环境变量、注册 systemd 服务、创建符号链接、初始化配置 |
| `apppack-post-uninstall.sh` | 删除安装目录**之前** | 删除环境变量文件、删除符号链接、停止服务、清理用户配置 |
| `apppack-pre-upgrade.sh` | 升级/降级安装时，解压新文件**之前** | 清理旧的系统环境配置，以便新的 post-install 重新设置 |

### 10.2 执行流程

```
首次安装流程:
  创建安装目录
    ↓
  执行 apppack-pre-install.sh  ← 安装前准备
    ↓ (失败则终止安装)
  解压应用文件
    ↓
  创建桌面集成
    ↓
  执行 apppack-post-install.sh ← 安装后配置
    ↓ (失败仅警告，不终止)
  安装完成

升级/降级安装流程:
  执行 apppack-pre-upgrade.sh  ← 清理旧配置（新增！）
    ↓ (失败仅警告，继续安装)
  创建安装目录
    ↓
  执行 apppack-pre-install.sh  ← 安装前准备
    ↓ (失败则终止安装)
  解压新版本文件
    ↓
  创建桌面集成
    ↓
  执行 apppack-post-install.sh ← 设置新配置
    ↓ (失败仅警告，不终止)
  安装完成

卸载流程:
  执行 apppack-post-uninstall.sh ← 卸载后清理
    ↓
  删除安装目录
    ↓
  清理桌面集成
    ↓
  卸载完成
```

### 10.3 脚本可用环境变量

执行脚本时，安装器会传入以下环境变量：

| 环境变量 | 说明 | 示例值 |
|----------|------|--------|
| `APPPACK_INSTALL_DIR` | 安装目录路径 | `/opt/MyApp` |
| `APPPACK_APP_NAME` | 应用名称 | `MyApp` |
| `APPPACK_VERSION` | 应用版本号 | `1.0.0` |

### 10.4 安装前脚本示例 (apppack-pre-install.sh)

```bash
#!/bin/bash
# 安装前环境检查

echo "检查系统环境..."

# 检查磁盘空间（至少需要 100MB）
AVAILABLE=$(df --output=avail "$APPPACK_INSTALL_DIR" | tail -1)
if [ "$AVAILABLE" -lt 102400 ]; then
    echo "错误: 磁盘空间不足"
    exit 1  # 非零退出码会终止安装
fi

# 检查必要系统包
for pkg in libssl sox ffmpeg; do
    if ! dpkg -l "$pkg" &>/dev/null; then
        echo "警告: 建议安装 $pkg"
    fi
done

# 下载额外资源
if [ ! -f "$APPPACK_INSTALL_DIR/resources/data.bin" ]; then
    echo "下载资源文件..."
    wget -O "$APPPACK_INSTALL_DIR/resources/data.bin" \
        "https://example.com/resources/data.bin"
fi

echo "环境检查通过"
exit 0  # 退出码 0 继续安装
```

### 10.5 安装后脚本示例 (apppack-post-install.sh)

```bash
#!/bin/bash
# 安装后系统配置

echo "配置系统环境..."

# 1. 创建环境变量文件
cat > /etc/profile.d/${APPPACK_APP_NAME}.sh << EOF
export ${APPPACK_APP_NAME}_HOME="$APPPACK_INSTALL_DIR"
export PATH="\$PATH:$APPPACK_INSTALL_DIR/usr/bin"
EOF

# 2. 创建符号链接到 /usr/local/bin
ln -sf "$APPPACK_INSTALL_DIR/AppRun" "/usr/local/bin/${APPPACK_APP_NAME}"

# 3. 注册 systemd 服务（如果应用是服务）
cat > /etc/systemd/system/${APPPACK_APP_NAME}.service << EOF
[Unit]
Description=${APPPACK_APP_NAME} Service
After=network.target

[Service]
ExecStart=$APPPACK_INSTALL_DIR/AppRun
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
systemctl enable ${APPPACK_APP_NAME}.service

# 4. 初始化应用配置
mkdir -p /etc/${APPPACK_APP_NAME}
cp "$APPPACK_INSTALL_DIR/usr/share/default.conf" "/etc/${APPPACK_APP_NAME}/"

echo "系统配置完成"
exit 0
```

### 10.6 卸载后脚本示例 (apppack-post-uninstall.sh)

```bash
#!/bin/bash
# 卸载后系统清理

echo "清理系统配置..."

# 1. 删除环境变量文件
rm -f "/etc/profile.d/${APPPACK_APP_NAME}.sh"

# 2. 删除符号链接
rm -f "/usr/local/bin/${APPPACK_APP_NAME}"

# 3. 停止并删除 systemd 服务
if systemctl is-active --quiet ${APPPACK_APP_NAME}.service 2>/dev/null; then
    systemctl stop ${APPPACK_APP_NAME}.service
fi
systemctl disable ${APPPACK_APP_NAME}.service 2>/dev/null
rm -f "/etc/systemd/system/${APPPACK_APP_NAME}.service"
systemctl daemon-reload

# 4. 清理应用配置（可选）
# rm -rf "/etc/${APPPACK_APP_NAME}"

# 5. 清理用户配置（可选）
# rm -rf "$HOME/.config/${APPPACK_APP_NAME}"

echo "系统清理完成"
exit 0
```

### 10.7 脚本执行规则

| 规则 | 说明 |
|------|------|
| **脚本不存在** | 静默跳过，不影响安装/卸载流程 |
| **安装前脚本失败** | 终止安装过程（非零退出码） |
| **安装后脚本失败** | 仅显示警告，安装已完成 |
| **卸载后脚本失败** | 仅显示警告，继续执行卸载 |
| **执行权限** | 安装器自动设置脚本的可执行权限 |
| **输出捕获** | 脚本的标准输出和错误输出会被实时显示 |

### 10.8 升级前脚本示例 (apppack-pre-upgrade.sh)

```bash
#!/bin/bash
# apppack-pre-upgrade.sh - 升级/降级前执行脚本
# 在安装新版本之前执行，用于清理旧的系统环境配置
# 这样新的 post-install 脚本可以重新设置新的配置
# 注意：此脚本仅在升级或降级安装时执行，首次安装不执行

echo "  升级前清理旧配置..."
echo "    应用: ${APPPACK_APP_NAME}"
echo "    新版本: ${APPPACK_VERSION}"
echo "    安装目录: ${APPPACK_INSTALL_DIR}"

# 清理旧的系统环境配置
# 删除之前 post-install 创建的环境变量文件
rm -f /etc/profile.d/${APPPACK_APP_NAME}.sh

# 删除之前 post-install 创建的符号链接
rm -f /usr/local/bin/${APPPACK_APP_NAME}

# 停止并删除旧的 systemd 服务（如果配置有变化）
# systemctl stop ${APPPACK_APP_NAME}.service 2>/dev/null || true
# systemctl disable ${APPPACK_APP_NAME}.service 2>/dev/null || true
# rm -f /etc/systemd/system/${APPPACK_APP_NAME}.service
# systemctl daemon-reload

exit 0
```

### 10.9 完整示例

查看 `examples/hello/` 目录中的示例脚本：

```
examples/hello/
├── apppack-pre-install.sh      # 安装前：检查磁盘空间和系统命令
├── apppack-post-install.sh     # 安装后：创建环境变量和符号链接
├── apppack-post-uninstall.sh   # 卸载后：清理环境变量和符号链接
└── apppack-pre-upgrade.sh      # 升级前：清理旧配置，准备新配置
```

---

## 11. 用户配置文件管理

应用程序通常需要保存用户的个性化设置、偏好、缓存数据等。AppPack 遵循 [XDG 基础目录规范](https://specifications.freedesktop.org/basedir-spec/latest/)，为应用提供标准的用户配置文件管理方案。

### 11.1 XDG 基础目录规范

Linux 下用户配置文件的存放遵循以下约定：

| 环境变量 | 默认路径 | 用途 | 示例 |
|----------|---------|------|------|
| `$XDG_CONFIG_HOME` | `~/.config` | 用户配置文件 | `~/.config/MyApp/settings.conf` |
| `$XDG_DATA_HOME` | `~/.local/share` | 用户数据文件 | `~/.local/share/MyApp/scores.dat` |
| `$XDG_CACHE_HOME` | `~/.cache` | 缓存文件 | `~/.cache/MyApp/cache.db` |
| `$XDG_STATE_HOME` | `~/.local/state` | 状态文件 | `~/.local/state/MyApp/window-state.conf` |

> **说明**: `$XDG_STATE_HOME` 是较新的规范，用于存放应用运行时状态（如窗口位置、会话恢复数据等），这些数据不属于配置也不属于缓存。

### 11.2 在 AppRun 中设置用户配置环境

推荐在 `AppRun` 启动脚本中设置用户配置目录的环境变量，方便应用读取：

```bash
#!/bin/bash
# AppRun - 应用启动脚本（含用户配置支持）
APPDIR="$(dirname "$(readlink -f "$0")")"

# 设置库路径
if [ -d "${APPDIR}/usr/lib" ]; then
    export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH}"
fi

# 设置安装目录环境变量
export APPPACK_INSTALL_DIR="${APPDIR}"

# ========== 用户配置目录设置 ==========
APP_NAME="MyApp"

# 用户配置目录（~/.config/MyApp/）
export APP_CONFIG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/${APP_NAME}"

# 用户数据目录（~/.local/share/MyApp/）
export APP_DATA_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/${APP_NAME}"

# 缓存目录（~/.cache/MyApp/）
export APP_CACHE_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/${APP_NAME}"

# 状态目录（~/.local/state/MyApp/）
export APP_STATE_DIR="${XDG_STATE_HOME:-$HOME/.local/state}/${APP_NAME}"

# 自动创建用户配置目录（首次运行时）
if [ ! -d "${APP_CONFIG_DIR}" ]; then
    mkdir -p "${APP_CONFIG_DIR}"
fi
if [ ! -d "${APP_DATA_DIR}" ]; then
    mkdir -p "${APP_DATA_DIR}"
fi
if [ ! -d "${APP_CACHE_DIR}" ]; then
    mkdir -p "${APP_CACHE_DIR}"
fi
if [ ! -d "${APP_STATE_DIR}" ]; then
    mkdir -p "${APP_STATE_DIR}"
fi

# 执行应用
exec "${APPDIR}/usr/bin/myapp" "$@"
```

### 11.3 在安装后脚本中初始化默认配置

可以在 `apppack-post-install.sh` 中为用户创建默认配置文件：

```bash
#!/bin/bash
# apppack-post-install.sh - 安装后配置（含用户配置初始化）
echo "  安装后配置..."

# 为所有当前用户和未来用户创建默认配置模板
DEFAULT_CONFIG_DIR="/etc/skel/.config/${APPPACK_APP_NAME}"
mkdir -p "${DEFAULT_CONFIG_DIR}"

# 写入默认配置文件
cat > "${DEFAULT_CONFIG_DIR}/settings.conf" << 'CONF'
# MyApp 默认配置
[General]
theme=default
language=system
auto_save=true

[Window]
width=1024
height=768
CONF

# 为当前已登录的用户创建配置（如果有的话）
for user_home in /home/*; do
    if [ -d "${user_home}" ]; then
        USER_CONFIG_DIR="${user_home}/.config/${APPPACK_APP_NAME}"
        if [ ! -d "${USER_CONFIG_DIR}" ]; then
            mkdir -p "${USER_CONFIG_DIR}"
            cp "${DEFAULT_CONFIG_DIR}/settings.conf" "${USER_CONFIG_DIR}/"
            chown -R $(stat -c "%u:%g" "${user_home}") "${USER_CONFIG_DIR}"
        fi
    fi
done

# 如果 root 也有配置目录
ROOT_CONFIG="/root/.config/${APPPACK_APP_NAME}"
if [ ! -d "${ROOT_CONFIG}" ]; then
    mkdir -p "${ROOT_CONFIG}"
    cp "${DEFAULT_CONFIG_DIR}/settings.conf" "${ROOT_CONFIG}/"
fi

echo "  ✓ 用户默认配置已创建"
exit 0
```

### 11.4 在卸载后脚本中处理用户配置

卸载时是否删除用户配置是一个需要谨慎处理的问题。推荐的做法是：

```bash
#!/bin/bash
# apppack-post-uninstall.sh - 卸载后清理（含用户配置处理）
echo "  卸载后清理..."

# 询问用户是否保留配置（通过交互式提示）
echo "  是否保留用户配置文件？"
echo "    配置文件位于: ~/.config/${APPPACK_APP_NAME}/"
echo "    数据文件位于: ~/.local/share/${APPPACK_APP_NAME}/"
echo "    缓存文件位于: ~/.cache/${APPPACK_APP_NAME}/"
echo ""
echo "  选择 [y/N]: 保留配置（默认）"
echo "  选择 [Y/n]: 删除所有用户配置和数据"

# 注意：实际卸载脚本中，建议通过参数控制而非交互式提示
# 这里展示的是推荐的处理策略

exit 0
```

### 11.5 推荐的用户配置管理策略

| 策略 | 说明 | 适用场景 |
|------|------|---------|
| **保留配置** | 卸载时保留用户配置，重装后恢复 | 大多数应用（推荐） |
| **清理配置** | 卸载时删除所有用户配置 | 临时工具、测试应用 |
| **提供清理选项** | 卸载脚本提供 `--purge` 参数清理配置 | 需要灵活控制的应用 |

**推荐实现方式** - 在 `apppack-post-uninstall.sh` 中通过参数控制：

```bash
#!/bin/bash
# apppack-post-uninstall.sh - 支持 --purge 参数清理用户配置

echo "  卸载后清理..."

# 检查是否传入了 --purge 参数
# 注意：卸载脚本通过 source 方式调用，参数通过 APPPACK_UNINSTALL_ARGS 环境变量传入
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
    echo "  用户配置已保留（使用 --purge 参数可清理）"
fi

exit 0
```

### 11.6 在卸载脚本中集成 --purge 支持

为了让用户能通过 `--purge` 参数清理配置，需要修改安装器生成的卸载脚本逻辑。安装器会在卸载时设置 `APPPACK_PURGE_CONFIG` 环境变量：

```bash
# 卸载时保留配置（默认）
sudo /opt/MyApp/uninstall.sh

# 卸载时清理所有用户配置
sudo APPPACK_PURGE_CONFIG=true /opt/MyApp/uninstall.sh
```

### 11.7 应用内读取配置的最佳实践

应用在运行时，应优先读取用户配置，再回退到系统默认配置：

```cpp
// C++ 示例：读取用户配置
#include <cstdlib>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>

std::string getConfigPath(const std::string& filename) {
    // 1. 优先读取用户配置
    const char* config_home = std::getenv("XDG_CONFIG_HOME");
    std::string user_config;
    if (config_home) {
        user_config = std::string(config_home) + "/MyApp/" + filename;
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            user_config = std::string(home) + "/.config/MyApp/" + filename;
        }
    }
    
    if (!user_config.empty() && std::filesystem::exists(user_config)) {
        return user_config;
    }
    
    // 2. 回退到安装目录的默认配置
    const char* install_dir = std::getenv("APPPACK_INSTALL_DIR");
    if (install_dir) {
        std::string default_config = std::string(install_dir) + "/usr/share/MyApp/" + filename;
        if (std::filesystem::exists(default_config)) {
            return default_config;
        }
    }
    
    return "";
}

// 使用示例
void loadSettings() {
    std::string config_file = getConfigPath("settings.conf");
    if (!config_file.empty()) {
        std::ifstream file(config_file);
        // 读取配置...
        std::cout << "加载配置: " << config_file << std::endl;
    } else {
        std::cout << "使用默认配置" << std::endl;
    }
}
```

### 11.8 用户配置目录结构示例

```
~/.config/MyApp/              # 用户配置（XDG_CONFIG_HOME）
├── settings.conf             # 用户自定义设置
├── bookmarks.xml             # 用户书签
└── profiles/                 # 多用户配置
    └── default.json

~/.local/share/MyApp/         # 用户数据（XDG_DATA_HOME）
├── database.db               # 用户数据库
├── scores.dat                # 游戏得分
└── downloads/                # 用户下载

~/.cache/MyApp/               # 缓存文件（XDG_CACHE_HOME）
├── thumbnails/               # 缩略图缓存
├── icon-cache.db             # 图标缓存
└── logs/                     # 日志文件

~/.local/state/MyApp/         # 应用状态（XDG_STATE_HOME）
├── window-state.conf         # 窗口位置和大小
├── recent-files.txt          # 最近打开的文件列表
└── session.json              # 会话恢复数据
```

---

## 12. 版本管理与更新

### 12.1 版本号规范

建议使用语义化版本号：`主版本.次版本.修订号`

- `1.0.0` - 初始版本
- `1.1.0` - 功能更新
- `1.1.1` - 补丁更新
- `2.0.0` - 大版本更新

### 12.2 发布更新

当有新版本时，重新打包：

```bash
# 打包 v1.0.0
./build/app-builder \
    --appdir=/path/to/MyApp.AppDir \
    --output=MyApp-v1.0.0.apppack \
    --version=1.0.0

# 打包 v1.1.0（更新后）
./build/app-builder \
    --appdir=/path/to/MyApp.AppDir \
    --output=MyApp-v1.1.0.apppack \
    --version=1.1.0
```

### 12.3 用户更新流程

用户只需运行新版本的安装包：

```bash
# 用户下载 v1.1.0 后直接安装
./MyApp-v1.1.0.apppack --install

# 安装器会自动检测已安装的 v1.0.0 并升级
# 输出: ✓ MyApp 升级成功! 1.0.0 → 1.1.0
```

### 12.4 版本检测行为

| 场景 | 行为 |
|------|------|
| 首次安装 | 正常安装 |
| 相同版本 | 覆盖安装 |
| 新版本 | 升级安装（显示版本变化） |
| 旧版本 | 降级安装（显示警告） |

---

## 13. 完整示例

### 13.1 打包一个 Qt 应用

```bash
# 1. 创建 AppDir 结构
mkdir -p MyQtApp.AppDir/usr/bin
mkdir -p MyQtApp.AppDir/usr/lib

# 2. 复制应用文件
cp /path/to/build/myqtapp MyQtApp.AppDir/usr/bin/
cp /path/to/resources/* MyQtApp.AppDir/usr/share/

# 3. 创建 AppRun
cat > MyQtApp.AppDir/AppRun << 'EOF'
#!/bin/bash
APPDIR="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${APPDIR}/usr/lib/qt5/plugins"
exec "${APPDIR}/usr/bin/myqtapp" "$@"
EOF
chmod +x MyQtApp.AppDir/AppRun

# 4. 创建 desktop 文件
cat > MyQtApp.AppDir/myqtapp.desktop << 'EOF'
[Desktop Entry]
Name=MyQtApp
Exec=AppRun
Icon=myqtapp
Terminal=false
Type=Application
Categories=Development;
EOF

# 5. 打包（含依赖）
./build/app-builder \
    --appdir=MyQtApp.AppDir \
    --output=MyQtApp-v1.0.apppack \
    --version=1.0.0 \
    --bundle-deps
```

### 13.2 打包一个 Python 应用

```bash
# 1. 创建 AppDir
mkdir -p MyPyApp.AppDir/usr/bin

# 2. 创建启动脚本（使用系统 Python）
cat > MyPyApp.AppDir/AppRun << 'EOF'
#!/bin/bash
APPDIR="$(dirname "$(readlink -f "$0")")"
exec python3 "${APPDIR}/usr/bin/main.py" "$@"
EOF
chmod +x MyPyApp.AppDir/AppRun

# 3. 复制 Python 脚本
cp main.py MyPyApp.AppDir/usr/bin/

# 4. 打包
./build/app-builder \
    --appdir=MyPyApp.AppDir \
    --output=MyPyApp.apppack
```

---

## 14. 常见问题

### Q: 打包后的文件太大怎么办？

A: 使用 `--bundle-deps` 会包含依赖库，可能增大包体积。可以：
- 不打包依赖，要求用户系统已安装所需库
- 使用 UPX 压缩可执行文件后再打包

### Q: 如何测试打包结果？

A: 安装到临时目录测试：

```bash
./MyApp.apppack --install --dest=/tmp/test-myapp
/tmp/test-myapp/AppRun
```

### Q: 应用需要 root 权限怎么办？

A: 在 AppRun 中使用 `pkexec` 或 `sudo`：

```bash
#!/bin/bash
APPDIR="$(dirname "$(readlink -f "$0")")"
exec pkexec "${APPDIR}/usr/bin/myapp" "$@"
```

### Q: 如何调试依赖问题？

A: 使用 `ldd` 手动检查：

```bash
ldd /path/to/MyApp.AppDir/usr/bin/myapp
```

### Q: 支持哪些架构？

A: 目前支持 x86_64 (amd64)。ARM 架构支持正在开发中。

### Q: 如何创建卸载脚本？

A: 安装器会自动生成卸载脚本，无需手动创建。用户运行 `uninstall.sh` 即可卸载。

---

> **提示**: 查看 `examples/hello/` 目录获取一个完整的最小示例。
