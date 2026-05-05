# AppPack 用户指南 - 如何安装和使用 .apppack 应用

## 目录

1. [什么是 AppPack？](#1-什么是-apppack)
2. [快速开始](#2-快速开始)
3. [安装应用](#3-安装应用)
4. [运行应用](#4-运行应用)
5. [用户配置文件管理](#5-用户配置文件管理)
6. [更新应用](#6-更新应用)
7. [卸载应用](#7-卸载应用)
8. [查看包信息](#8-查看包信息)
9. [命令行参考](#9-命令行参考)
10. [常见问题](#10-常见问题)

---

## 1. 什么是 AppPack？

AppPack 是一种自解压安装包格式，类似于 Windows 的 `.exe` 安装程序。它有以下特点：

- **一个文件搞定** - 下载一个 `.apppack` 文件即可安装
- **无需处理依赖** - 应用自带所有需要的库，不依赖系统包管理器
- **跨发行版** - 在 Ubuntu、Fedora、Arch 等任何 Linux 发行版上都能运行
- **安装到指定目录** - 像 Windows 一样安装到 `/opt/` 或自定义位置
- **桌面图标** - 安装后自动出现在应用菜单中
- **干净卸载** - 一键删除所有文件

### 与其它格式对比

| 特性 | AppPack | AppImage | Snap | Flatpak | deb/rpm |
|------|---------|----------|------|---------|---------|
| 单文件分发 | ✓ | ✓ | ✗ | ✗ | ✗ |
| 安装到目录 | ✓ | ✗ | ✓ | ✓ | ✓ |
| 桌面集成 | ✓ | ✓ | ✓ | ✓ | ✓ |
| 离线安装 | ✓ | ✓ | ✗ | ✗ | ✓ |
| 跨发行版 | ✓ | ✓ | ✓ | ✓ | ✗ |
| 无需运行时 | ✓ | ✓ | ✗ | ✗ | ✓ |

---

## 2. 快速开始

### 2.1 下载应用

从开发者处获取 `.apppack` 文件，例如：

```bash
# 下载示例应用
wget https://example.com/download/MyApp-v1.0.0.apppack
```

### 2.2 安装应用

```bash
# 安装到默认目录（需要 sudo 权限）
sudo ./MyApp-v1.0.0.apppack --install
```

### 2.3 运行应用

安装后，可以通过应用菜单找到并启动应用，或直接运行：

```bash
# 运行已安装的应用
/opt/MyApp/AppRun
```

### 2.4 卸载应用

```bash
# 运行卸载脚本
sudo /opt/MyApp/uninstall.sh
```

---

## 3. 安装应用

### 3.1 安装到默认目录

大多数应用默认安装到 `/opt/<应用名>`：

```bash
sudo ./MyApp.apppack --install
```

> **为什么需要 sudo？** 安装到 `/opt/` 等系统目录需要 root 权限。如果安装到用户目录则不需要。

### 3.2 安装到自定义目录

使用 `--dest` 参数指定安装位置：

```bash
# 安装到用户目录（不需要 sudo）
./MyApp.apppack --install --dest=/home/用户名/Applications/MyApp

# 安装到 /tmp 测试（不需要 sudo）
./MyApp.apppack --install --dest=/tmp/test-myapp
```

### 3.3 安装过程示例

```
$ sudo ./HelloWorld.apppack --install
正在安装 HelloWorld v1.0.0...
  安装到: /opt/HelloWorld
  正在读取包数据...
  正在解压数据...
  正在提取文件...
  正在创建桌面图标...

✓ HelloWorld 安装成功!
  安装路径: /opt/HelloWorld
  卸载命令: /opt/HelloWorld/uninstall.sh
```

### 3.4 安装后发生了什么？

安装器会：

1. 创建安装目录（如 `/opt/HelloWorld/`）
2. 解压应用文件到安装目录
3. 创建桌面图标（在应用菜单中显示）
4. 创建卸载脚本（`uninstall.sh`）
5. 写入版本信息（`.apppack-version`）

---

## 4. 运行应用

### 4.1 从应用菜单启动

安装后，在系统应用菜单中可以找到应用：

- **GNOME**: 按 Super 键，搜索应用名称
- **KDE**: 打开应用启动器，搜索应用名称
- **XFCE**: 打开 Whisker 菜单，搜索应用名称

### 4.2 从命令行启动

```bash
# 直接运行 AppRun
/opt/MyApp/AppRun

# 或运行可执行文件
/opt/MyApp/usr/bin/myapp
```

### 4.3 添加到 PATH（可选）

为了方便从终端启动，可以创建符号链接：

```bash
sudo ln -s /opt/MyApp/AppRun /usr/local/bin/myapp

# 之后可以直接运行
myapp
```

---

## 5. 用户配置文件管理

AppPack 应用遵循 [XDG 基础目录规范](https://specifications.freedesktop.org/basedir-spec/latest/)，将用户配置文件、数据、缓存和状态分开存放。

### 5.1 用户配置目录结构

安装并运行 AppPack 应用后，会在用户目录下创建以下目录：

| 目录 | 默认路径 | 用途 |
|------|---------|------|
| 配置目录 | `~/.config/<应用名>/` | 用户个性化设置、偏好 |
| 数据目录 | `~/.local/share/<应用名>/` | 用户数据文件（如数据库、存档） |
| 缓存目录 | `~/.cache/<应用名>/` | 临时缓存文件（可安全删除） |
| 状态目录 | `~/.local/state/<应用名>/` | 应用运行时状态（如窗口位置） |

### 5.2 查看用户配置

以 HelloWorld 应用为例：

```bash
# 查看用户配置文件
ls -la ~/.config/HelloWorld/
cat ~/.config/HelloWorld/settings.conf

# 查看用户数据
ls -la ~/.local/share/HelloWorld/

# 查看缓存
ls -la ~/.cache/HelloWorld/
```

### 5.3 备份和恢复用户配置

**备份配置：**
```bash
# 备份所有用户配置和数据
tar -czf myapp-config-backup.tar.gz \
    ~/.config/MyApp/ \
    ~/.local/share/MyApp/ \
    ~/.local/state/MyApp/
```

**恢复配置：**
```bash
# 恢复之前备份的用户配置
tar -xzf myapp-config-backup.tar.gz -C ~/
```

### 5.4 卸载时处理用户配置

卸载应用时，默认**保留**用户配置，以便重新安装后恢复使用。

如果需要彻底清理所有用户配置，可以使用以下方式：

```bash
# 方式一：卸载时清理配置
sudo APPPACK_PURGE_CONFIG=true /opt/MyApp/uninstall.sh

# 方式二：手动删除用户配置
rm -rf ~/.config/MyApp/
rm -rf ~/.local/share/MyApp/
rm -rf ~/.cache/MyApp/
rm -rf ~/.local/state/MyApp/
```

> **注意**：清理用户配置是不可恢复的操作，请确保已备份重要数据。

### 5.5 用户配置常见问题

**Q: 为什么应用设置没有保存？**
A: 检查用户配置目录是否有写入权限：
```bash
ls -ld ~/.config/MyApp/
# 确保目录属于当前用户
```

**Q: 如何重置应用到默认设置？**
A: 删除用户配置文件，应用会在下次启动时重新创建默认配置：
```bash
rm -f ~/.config/MyApp/settings.conf
```

**Q: 缓存文件占用太多空间怎么办？**
A: 缓存目录可以安全清理：
```bash
rm -rf ~/.cache/MyApp/*
```

---

## 6. 更新应用

### 6.1 升级到新版本

当开发者发布新版本时，下载新版本的 `.apppack` 文件，然后运行安装命令：

```bash
# 下载新版本
wget https://example.com/download/MyApp-v1.1.0.apppack

# 直接安装（会自动检测旧版本并升级）
sudo ./MyApp-v1.1.0.apppack --install
```

### 6.2 升级过程示例

```
$ sudo ./MyApp-v1.1.0.apppack --install
正在安装 MyApp v1.1.0...
  安装到: /opt/MyApp
  检测到已安装版本: v1.0.0
  ✓ 检测到新版本，执行升级安装...
  正在读取包数据...
  正在解压数据...
  正在提取文件...
  正在创建桌面图标...

✓ MyApp 升级成功!
  1.0.0 → 1.1.0
  安装路径: /opt/MyApp
  卸载命令: /opt/MyApp/uninstall.sh
```

### 6.3 版本检测行为

| 场景 | 输出 | 说明 |
|------|------|------|
| 首次安装 | `正在安装 MyApp v1.0.0...` | 正常安装 |
| 相同版本 | `相同版本，执行覆盖安装...` | 覆盖安装，文件被替换 |
| 升级安装 | `✓ 检测到新版本，执行升级安装...` | 自动升级 |
| 降级安装 | `警告: 安装的版本低于已安装版本` | 降级安装（谨慎操作） |

---

## 7. 卸载应用

### 7.1 使用卸载脚本

```bash
# 运行安装目录中的卸载脚本
sudo /opt/MyApp/uninstall.sh
```

### 7.2 使用安装包卸载

如果你还保留着 `.apppack` 文件：

```bash
sudo ./MyApp.apppack --uninstall
```

### 7.3 卸载过程示例

```
$ sudo /opt/HelloWorld/uninstall.sh
正在卸载 HelloWorld...
HelloWorld 已成功卸载
```

### 7.4 卸载后发生了什么？

卸载脚本会：

1. 删除整个安装目录（如 `/opt/HelloWorld/`）
2. 删除桌面图标文件
3. 删除应用菜单项
4. 更新桌面数据库

---

## 8. 查看包信息

### 8.1 查看安装包信息

```bash
./MyApp.apppack --info
```

示例输出：

```
==========================================
  AppPack 安装包信息
==========================================
  应用名称: HelloWorld
  版本号:   v1.0.0
  安装目录: /opt/HelloWorld
  数据偏移: 2787392
  数据大小: 4135 字节
  原始大小: 17652 字节
  校验和: 0x7dad7
==========================================
```

### 8.2 查看已安装应用的版本

```bash
cat /opt/MyApp/.apppack-version
```

---

## 9. 命令行参考

### 9.1 安装包命令

```bash
./<package>.apppack [命令] [选项]
```

| 命令 | 说明 |
|------|------|
| `--install` | 安装或升级应用 |
| `--uninstall` | 卸载应用 |
| `--info` | 查看包信息 |
| `--help` | 显示帮助信息 |

| 选项 | 说明 |
|------|------|
| `--dest=<目录>` | 指定安装目录（覆盖默认） |

### 9.2 完整使用示例

```bash
# 查看帮助
./MyApp.apppack --help

# 查看包信息
./MyApp.apppack --info

# 安装到默认目录
sudo ./MyApp.apppack --install

# 安装到自定义目录
./MyApp.apppack --install --dest=/home/user/apps/MyApp

# 卸载
sudo ./MyApp.apppack --uninstall
```

---

## 10. 常见问题

### Q: 安装时提示"没有写入权限"怎么办？

A: 安装到 `/opt/` 或 `/usr/` 需要 root 权限，请使用 `sudo`：

```bash
sudo ./MyApp.apppack --install
```

或者安装到用户目录：

```bash
./MyApp.apppack --install --dest=/home/用户名/Applications/MyApp
```

### Q: 如何安装到非系统目录？

A: 使用 `--dest` 参数：

```bash
./MyApp.apppack --install --dest=/home/用户名/MyApp
```

### Q: 卸载后如何重新安装？

A: 直接再次运行安装命令即可：

```bash
sudo ./MyApp.apppack --install
```

### Q: 安装包文件可以删除吗？

A: 可以。安装后 `.apppack` 文件不再需要。但建议保留以便将来卸载或重新安装。

### Q: 如何找到已安装的应用？

A: 应用安装在 `--install-dir` 指定的目录（默认 `/opt/<应用名>`）。桌面图标会自动添加到应用菜单。

### Q: 应用无法启动怎么办？

A: 尝试以下步骤：

1. 检查是否有执行权限：
   ```bash
   ls -l /opt/MyApp/AppRun
   ```

2. 从命令行运行查看错误信息：
   ```bash
   /opt/MyApp/AppRun
   ```

3. 检查依赖库：
   ```bash
   ldd /opt/MyApp/usr/bin/myapp
   ```

### Q: 如何手动删除桌面图标？

A: 如果卸载脚本没有清理干净，可以手动删除：

```bash
rm -f ~/.local/share/applications/MyApp.desktop
rm -f ~/.local/share/icons/MyApp.*
update-desktop-database ~/.local/share/applications/
```

### Q: 支持哪些 Linux 发行版？

A: AppPack 设计为跨发行版兼容，已在以下发行版测试：

- Ubuntu 20.04+
- Debian 11+
- Fedora 35+
- Arch Linux
- openSUSE Tumbleweed

### Q: 安装包安全吗？

A: AppPack 安装包是自解压可执行文件。与任何可执行文件一样，请仅从可信来源下载。你可以使用 `--info` 查看包内容信息。

---

> **提示**: 如果遇到问题，请联系应用开发者获取支持。
