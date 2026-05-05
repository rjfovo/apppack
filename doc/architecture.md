# AppPack 架构文档

> 本文档详细描述 AppPack 项目的架构设计、功能列表和技术实现细节。

---

## 目录

1. [项目架构图](#1-项目架构图)
2. [功能列表](#2-功能列表)
3. [技术实现详解](#3-技术实现详解)
4. [数据流与执行流程](#4-数据流与执行流程)
5. [项目文件结构](#5-项目文件结构)

---

## 1. 项目架构图

### 1.1 整体架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           AppPack 项目架构                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        打包工具 (app-builder)                        │   │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────────┐   │   │
│  │  │ 模板管理器  │ │ 依赖分析器  │ │ 打包器      │ │ 文件工具       │   │   │
│  │  │ Template   │ │ Dependency │ │ Packager   │ │ FileUtils     │   │   │
│  │  │ Manager    │ │ Analyzer   │ │            │ │               │   │   │
│  │  └────────────┘ └────────────┘ └────────────┘ └────────────────┘   │   │
│  │         │              │              │               │            │   │
│  │         ▼              ▼              ▼               ▼            │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │                    压缩器 (Compressor)                       │   │   │
│  │  │             使用 Zstandard (zstd) 压缩数据                   │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    自解压安装包 (.apppack)                          │   │
│  │  ┌──────────────┐ ┌──────────────────┐ ┌──────────────────────┐   │   │
│  │  │  包头部信息   │ │  压缩数据区       │ │  安装器运行时        │   │   │
│  │  │  PackageHeader│ │  (zstd 压缩)     │ │  (app-installer)    │   │   │
│  │  └──────────────┘ └──────────────────┘ └──────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                     安装器运行时 (app-installer)                     │   │
│  │  ┌──────────┐ ┌──────────┐ ┌────────────┐ ┌──────────┐ ┌──────┐   │   │
│  │  │ 头部读取器│ │ 解压缩器  │ │ 安装器     │ │ 卸载器   │ │脚本  │   │   │
│  │  │ Header   │ │ Decompress│ │ Installer  │ │Uninstall │ │运行器│   │   │
│  │  │ Reader   │ │          │ │            │ │          │ │      │   │   │
│  │  └──────────┘ └──────────┘ └────────────┘ └──────────┘ └──────┘   │   │
│  │  ┌────────────────┐ ┌──────────────────────┐                      │   │
│  │  │ 桌面集成        │ │ 文件工具              │                      │   │
│  │  │ Desktop        │ │ FileUtils            │                      │   │
│  │  │ Integration    │ │                      │                      │   │
│  │  └────────────────┘ └──────────────────────┘                      │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 打包流程架构

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│  应用目录     │     │  依赖分析     │     │  数据打包     │     │  生成安装包   │
│  (AppDir)    │────▶│  (ldd 扫描)  │────▶│  (zstd 压缩) │────▶│  (.apppack)  │
└──────────────┘     └──────────────┘     └──────────────┘     └──────────────┘
       │                    │                    │                    │
       ▼                    ▼                    ▼                    ▼
  ┌──────────┐       ┌──────────┐       ┌──────────┐       ┌──────────────────┐
  │ 应用文件  │       │ 收集依赖  │       │ 序列化    │       │ 安装器 + 头部    │
  │ 生命周期  │       │ 库到     │       │ 压缩      │       │ + 压缩数据       │
  │ 脚本     │       │ usr/lib/ │       │          │       │ 拼接为自解压文件  │
  └──────────┘       └──────────┘       └──────────┘       └──────────────────┘
```

### 1.3 安装流程架构

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│  执行安装包   │     │  读取头部     │     │  解压数据     │     │  写入文件     │
│  --install   │────▶│  解析元数据   │────▶│  反序列化     │────▶│  到安装目录   │
└──────────────┘     └──────────────┘     └──────────────┘     └──────────────┘
       │                    │                    │                    │
       ▼                    ▼                    ▼                    ▼
  ┌──────────┐       ┌──────────┐       ┌──────────┐       ┌──────────────────┐
  │ 自解压    │       │ 版本检测   │       │ zstd     │       │ 创建桌面集成     │
  │ 执行入口  │       │ 升级/降级  │       │ 解压缩    │       │ 执行生命周期脚本  │
  └──────────┘       └──────────┘       └──────────┘       └──────────────────┘
```

### 1.4 模块依赖关系

```
┌─────────────────────────────────────────────────────────────────────┐
│                        builder/ (打包工具)                           │
│                                                                     │
│  main.cpp                                                            │
│    │                                                                 │
│    ├──▶ template_manager.cpp  ──▶  packager.cpp                      │
│    │       (项目模板初始化)          (打包核心逻辑)                    │
│    │                                  │                              │
│    │                                  ├──▶ file_utils.cpp            │
│    │                                  │      (文件读写操作)           │
│    │                                  │                              │
│    │                                  ├──▶ compressor.cpp            │
│    │                                  │      (zstd 压缩)             │
│    │                                  │                              │
│    │                                  ├──▶ desktop_parser.cpp        │
│    │                                  │      (桌面文件解析)           │
│    │                                  │                              │
│    │                                  └──▶ dependency_analyzer.cpp   │
│    │                                         (ldd 依赖分析)          │
│    │                                                                 │
│    └──▶ main.cpp (命令行参数解析)                                     │
│                                                                     │
│  common.h - 共享常量和结构体定义                                      │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                      installer/ (安装器运行时)                       │
│                                                                     │
│  main.cpp                                                            │
│    │                                                                 │
│    ├──▶ installer.cpp  ──▶  file_utils.cpp                          │
│    │      (安装核心逻辑)       (文件读写操作)                         │
│    │         │                                                      │
│    │         ├──▶ compressor.cpp                                     │
│    │         │      (zstd 解压缩)                                    │
│    │         │                                                      │
│    │         ├──▶ header_reader.cpp                                  │
│    │         │      (包头部解析)                                     │
│    │         │                                                      │
│    │         ├──▶ desktop_integration.cpp                            │
│    │         │      (桌面图标/菜单集成)                               │
│    │         │                                                      │
│    │         └──▶ script_runner.cpp                                  │
│    │                (生命周期脚本执行)                                │
│    │                                                                 │
│    └──▶ uninstaller.cpp                                              │
│           (卸载逻辑)                                                 │
│                                                                     │
│  common.h - 共享常量和结构体定义                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 2. 功能列表

### 2.1 核心功能

| 功能 | 说明 | 状态 |
|------|------|------|
| **自解压安装包** | 将应用打包为单个可执行文件，运行即可安装 | ✅ 已完成 |
| **跨发行版兼容** | 不依赖 deb/rpm 等包管理器，可在任何 Linux 发行版运行 | ✅ 已完成 |
| **安装到指定目录** | 像 Windows 一样安装到 `/opt/` 或自定义目录 | ✅ 已完成 |
| **桌面集成** | 自动创建桌面图标、开始菜单项 | ✅ 已完成 |
| **干净卸载** | 一键删除所有安装文件和桌面集成 | ✅ 已完成 |
| **版本管理** | 支持版本号、升级/降级检测 | ✅ 已完成 |
| **动态库依赖打包** | 自动收集并打包应用所需的动态库 | ✅ 已完成 |
| **生命周期脚本** | 安装/卸载/升级各阶段自动执行自定义脚本 | ✅ 已完成 |
| **项目模板初始化** | 类似 deb 的 `debian/` 目录，快速初始化打包配置 | ✅ 已完成 |
| **交互式配置** | 通过问答方式生成打包配置 | ✅ 已完成 |
| **GUI 示例应用** | 使用 Xlib 的 HelloWorld 示例，双击弹出窗口 | ✅ 已完成 |

### 2.2 打包工具功能 (app-builder)

| 功能 | 命令行参数 | 说明 |
|------|-----------|------|
| 基本打包 | `--appdir=<目录> --output=<文件>` | 将 AppDir 打包为 .apppack |
| 指定安装目录 | `--install-dir=<路径>` | 设置默认安装路径 |
| 指定版本号 | `--version=<版本号>` | 设置应用版本号 |
| 自动打包依赖 | `--bundle-deps` | 自动收集并打包动态库依赖 |
| 初始化模板 | `--init` | 交互式创建 apppack/ 模板目录 |
| 从模板构建 | `--build` | 从 apppack/ 目录读取配置并打包 |
| 查看帮助 | `--help` | 显示帮助信息 |

### 2.3 安装器功能 (app-installer)

| 功能 | 命令行参数 | 说明 |
|------|-----------|------|
| 安装应用 | `--install` | 安装应用到指定目录 |
| 指定安装路径 | `--dest=<路径>` | 覆盖默认安装目录 |
| 查看包信息 | `--info` | 显示安装包元数据 |
| 卸载应用 | `--uninstall` | 卸载已安装的应用 |
| 版本检测 | 自动 | 检测已安装版本，决定升级/降级/覆盖安装 |
| 桌面集成 | 自动 | 安装时自动创建桌面图标和菜单 |
| 生命周期脚本 | 自动 | 在对应阶段自动执行自定义脚本 |

### 2.4 生命周期脚本

| 脚本文件 | 执行时机 | 用途 |
|----------|---------|------|
| `apppack-pre-install.sh` | 解压文件**之前** | 检查环境、下载资源、备份旧配置 |
| `apppack-post-install.sh` | 解压和桌面集成**之后** | 设置环境变量、注册服务、创建符号链接 |
| `apppack-post-uninstall.sh` | 删除安装目录**之前** | 清理环境变量、删除符号链接、停止服务 |
| `apppack-pre-upgrade.sh` | 升级/降级时，解压新文件**之前** | 清理旧的系统环境配置 |

### 2.5 项目模板功能 (apppack init)

| 生成的文件 | 说明 |
|-----------|------|
| `apppack.json` | 打包配置文件（应用名、版本、安装目录等） |
| `AppRun` | 应用启动脚本 |
| `{AppName}.desktop` | 桌面入口文件 |
| `{AppName}.svg` | 应用图标（SVG 格式） |
| `apppack-pre-install.sh` | 安装前脚本模板 |
| `apppack-post-install.sh` | 安装后脚本模板 |
| `apppack-post-uninstall.sh` | 卸载后脚本模板 |
| `apppack-pre-upgrade.sh` | 升级前脚本模板 |
| `usr/bin/` | 可执行文件目录 |
| `usr/lib/` | 依赖库目录 |

---

## 3. 技术实现详解

### 3.1 自解压安装包格式 (.apppack)

#### 3.1.1 文件结构

```
┌──────────────────────────────────────────────┐
│              安装器运行时 (ELF)                │
│  ┌────────────────────────────────────────┐  │
│  │  app-installer 可执行文件               │  │
│  │  (C++ 编译的 ELF 二进制)               │  │
│  └────────────────────────────────────────┘  │
├──────────────────────────────────────────────┤
│              包头部信息                       │
│  ┌────────────────────────────────────────┐  │
│  │  Magic: "APPPACK\0" (8 bytes)          │  │
│  │  data_offset: 压缩数据偏移              │  │
│  │  data_size: 压缩数据大小                │  │
│  │  orig_size: 原始数据大小                │  │
│  │  app_name: 应用名称 (256 bytes)         │  │
│  │  install_dir: 安装目录 (256 bytes)      │  │
│  │  version: 版本号 (64 bytes)             │  │
│  │  checksum: 校验和 (4 bytes)             │  │
│  └────────────────────────────────────────┘  │
├──────────────────────────────────────────────┤
│              压缩数据区                       │
│  ┌────────────────────────────────────────┐  │
│  │  Zstandard (zstd) 压缩的数据           │  │
│  │  包含序列化的文件列表和文件内容         │  │
│  └────────────────────────────────────────┘  │
└──────────────────────────────────────────────┘
```

**实现原理**：将安装器运行时（ELF 可执行文件）作为文件头，后面追加包头部和压缩数据。当用户运行 `.apppack` 文件时，操作系统将其作为 ELF 执行，安装器运行时首先运行，然后读取自身文件末尾的头部和数据。

#### 3.1.2 关键技术：自解压

```cpp
// 安装器运行时读取自身文件
std::string self_path = readSelfPath();  // 获取 /proc/self/exe

// 读取包头部
PackageHeader header = readHeader(self_path);

// 读取并解压数据
auto compressed = readFileRange(self_path, header.data_offset, header.data_size);
auto decompressed = decompressData(compressed, header.orig_size);

// 反序列化文件列表
auto files = deserializeFiles(decompressed);
```

### 3.2 数据压缩技术

#### 3.2.1 Zstandard (zstd) 压缩

项目使用 **Zstandard (zstd)** 压缩算法，原因如下：

| 特性 | zstd | gzip | bzip2 | xz |
|------|------|------|-------|-----|
| 压缩速度 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ | ⭐ |
| 解压速度 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| 压缩比 | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| 内存占用 | 低 | 低 | 中 | 高 |

**实现代码**：

```cpp
// 压缩 (打包工具)
std::vector<char> compressData(const std::vector<char>& input) {
    size_t bound = ZSTD_compressBound(input.size());
    std::vector<char> compressed(bound);
    
    size_t result = ZSTD_compress(compressed.data(), bound,
                                   input.data(), input.size(), 3);  // 压缩级别 3
    compressed.resize(result);
    return compressed;
}

// 解压缩 (安装器)
std::vector<char> decompressData(const std::vector<char>& input, size_t orig_size) {
    std::vector<char> decompressed(orig_size);
    
    ZSTD_decompress(decompressed.data(), orig_size,
                    input.data(), input.size());
    return decompressed;
}
```

### 3.3 文件序列化技术

#### 3.3.1 自定义二进制序列化

文件列表和内容使用自定义二进制格式序列化：

```
┌──────────────────────────────────────────────┐
│  文件数量 (uint32_t, 4 bytes)                │
├──────────────────────────────────────────────┤
│  文件条目 1:                                  │
│  ├── 路径长度 (uint32_t, 4 bytes)            │
│  ├── 相对路径 (UTF-8 字符串)                 │
│  ├── 文件权限 (mode_t, 4 bytes)              │
│  ├── 内容长度 (uint64_t, 8 bytes)            │
│  └── 文件内容 (二进制数据)                    │
├──────────────────────────────────────────────┤
│  文件条目 2:                                  │
│  └── ...                                     │
├──────────────────────────────────────────────┤
│  ...                                         │
└──────────────────────────────────────────────┘
```

**实现代码**：

```cpp
// 序列化
std::vector<char> serializeFiles(const std::vector<FileEntry>& files) {
    std::vector<char> buffer;
    
    // 写入文件数量
    uint32_t count = files.size();
    buffer.insert(buffer.end(), (char*)&count, (char*)&count + 4);
    
    for (const auto& file : files) {
        // 写入路径长度和路径
        uint32_t path_len = file.relative_path.size();
        buffer.insert(buffer.end(), (char*)&path_len, (char*)&path_len + 4);
        buffer.insert(buffer.end(), file.relative_path.begin(), file.relative_path.end());
        
        // 写入权限
        buffer.insert(buffer.end(), (char*)&file.permissions, (char*)&file.permissions + 4);
        
        // 写入内容长度和内容
        uint64_t content_len = file.content.size();
        buffer.insert(buffer.end(), (char*)&content_len, (char*)&content_len + 8);
        buffer.insert(buffer.end(), file.content.begin(), file.content.end());
    }
    return buffer;
}

// 反序列化
std::vector<FileEntry> deserializeFiles(const std::vector<char>& data) {
    std::vector<FileEntry> files;
    size_t offset = 0;
    
    uint32_t count;
    std::memcpy(&count, data.data() + offset, 4);
    offset += 4;
    
    for (uint32_t i = 0; i < count; i++) {
        FileEntry entry;
        
        // 读取路径
        uint32_t path_len;
        std::memcpy(&path_len, data.data() + offset, 4);
        offset += 4;
        entry.relative_path = std::string(data.data() + offset, path_len);
        offset += path_len;
        
        // 读取权限
        std::memcpy(&entry.permissions, data.data() + offset, 4);
        offset += 4;
        
        // 读取内容
        uint64_t content_len;
        std::memcpy(&content_len, data.data() + offset, 8);
        offset += 8;
        entry.content.assign(data.data() + offset, data.data() + offset + content_len);
        offset += content_len;
        
        files.push_back(entry);
    }
    return files;
}
```

### 3.4 动态库依赖分析

#### 3.4.1 实现原理

使用 `ldd` 命令扫描 ELF 文件的动态库依赖，递归收集所有需要的库文件：

```
扫描 AppDir 中所有 ELF 文件
    │
    ▼
对每个 ELF 文件执行 ldd
    │
    ▼
解析 ldd 输出，提取依赖库路径
    │
    ▼
过滤系统库（排除 ld-linux, libc, libpthread 等基础库）
    │
    ▼
递归分析依赖库的依赖（最多 10 层）
    │
    ▼
将收集到的库复制到 AppDir/usr/lib/
    │
    ▼
更新 AppRun 设置 LD_LIBRARY_PATH
```

**实现代码**：

```cpp
void collectDependencies(const std::string& appdir_path) {
    std::set<std::string> collected;
    
    // 扫描 AppDir 中所有 ELF 文件
    auto elf_files = findElfFiles(appdir_path);
    
    for (const auto& elf : elf_files) {
        // 递归收集依赖
        collectDepsRecursive(elf, collected, appdir_path, 0);
    }
    
    // 复制依赖库到 AppDir/usr/lib/
    for (const auto& lib : collected) {
        fs::copy(lib, appdir_path + "/usr/lib/");
    }
}
```

### 3.5 版本管理

#### 3.5.1 版本检测机制

安装器通过读取已安装目录中的 `.apppack-version` 文件来检测已安装版本：

```
安装时:
  1. 检查安装目录是否存在
  2. 读取 .apppack-version 文件获取旧版本号
  3. 比较新旧版本号:
     - 相同版本 → 覆盖安装
     - 新版本 → 升级安装
     - 旧版本 → 降级安装（显示警告）
  4. 写入新版本号到 .apppack-version
```

**版本比较算法**：

```cpp
bool isVersionLess(const std::string& v1, const std::string& v2) {
    // 将 "1.2.3" 分割为 [1, 2, 3]
    auto p1 = split(v1);
    auto p2 = split(v2);
    
    // 逐段比较
    for (size_t i = 0; i < max_len; i++) {
        if (p1[i] < p2[i]) return true;
        if (p1[i] > p2[i]) return false;
    }
    return false;
}
```

### 3.6 桌面集成

#### 3.6.1 实现原理

遵循 [FreeDesktop 规范](https://specifications.freedesktop.org/)，安装时自动创建桌面图标和菜单项：

```
安装时:
  1. 从包中提取 .desktop 文件和图标
  2. 复制 .desktop 到 ~/.local/share/applications/
  3. 复制图标到 ~/.local/share/icons/
  4. 运行 update-desktop-database 更新菜单缓存

卸载时:
  1. 删除 ~/.local/share/applications/{AppName}.desktop
  2. 删除 ~/.local/share/icons/{AppName}.png
  3. 运行 update-desktop-database 更新菜单缓存
```

### 3.7 生命周期脚本执行

#### 3.7.1 实现原理

安装器在安装/卸载的不同阶段自动查找并执行约定命名的脚本：

```cpp
ScriptResult runScript(const std::string& script_path, const std::string& phase,
                        const std::string& install_path,
                        const std::string& app_name,
                        const std::string& version) {
    if (!fs::exists(script_path)) {
        return {false, true, 0, ""};  // 脚本不存在，静默跳过
    }
    
    // 设置环境变量并执行
    std::string cmd = "export APPPACK_INSTALL_DIR=\"" + install_path + "\" "
                      "APPPACK_APP_NAME=\"" + app_name + "\" "
                      "APPPACK_VERSION=\"" + version + "\"; "
                      "bash \"" + script_path + "\" 2>&1";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    // ... 读取输出并检查退出码
}
```

**环境变量**：

| 环境变量 | 说明 | 示例 |
|----------|------|------|
| `APPPACK_INSTALL_DIR` | 安装目录 | `/opt/MyApp` |
| `APPPACK_APP_NAME` | 应用名称 | `MyApp` |
| `APPPACK_VERSION` | 版本号 | `1.0.0` |

### 3.8 项目模板系统

#### 3.8.1 实现原理

`app-builder --init` 通过交互式问答生成完整的打包模板：

```
用户输入:
  应用名称, 版本号, 描述, 安装目录,
  可执行文件名, 应用类别, 是否终端运行,
  是否打包依赖

生成文件:
  apppack.json, AppRun, .desktop, .svg,
  4 个生命周期脚本, usr/bin/, usr/lib/
```

`app-builder --build` 从 `apppack/` 目录读取配置并打包：

```
1. 读取 apppack.json 获取配置
2. 收集 apppack/ 目录下所有文件
3. 调用 packager 生成 .apppack 文件
```

### 3.9 使用的技术栈

| 技术 | 用途 | 说明 |
|------|------|------|
| **C++17** | 开发语言 | 使用 C++17 标准，支持 filesystem 库 |
| **CMake** | 构建系统 | 跨平台构建配置 |
| **Zstandard (zstd)** | 数据压缩 | 高性能压缩算法，用于打包数据 |
| **ELF 二进制格式** | 可执行文件 | Linux 原生可执行文件格式 |
| **ldd** | 依赖分析 | 系统工具，用于分析动态库依赖 |
| **FreeDesktop 规范** | 桌面集成 | Linux 桌面图标和菜单标准 |
| **Xlib** | GUI 示例 | X11 窗口系统库，用于示例应用 |
| **popen** | 脚本执行 | 进程通信，用于执行生命周期脚本 |
| **bash** | 脚本语言 | 生命周期脚本使用 bash 编写 |

---

## 4. 数据流与执行流程

### 4.1 打包数据流

```
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
│ AppDir   │───▶│ 收集文件  │───▶│ 序列化   │───▶│ zstd     │───▶│ 拼接     │
│ 目录     │    │ 和元数据  │    │ 为二进制  │    │ 压缩     │    │ 安装包   │
└──────────┘    └──────────┘    └──────────┘    └──────────┘    └──────────┘
                                                                    │
                                                                    ▼
                                                              ┌──────────┐
                                                              │ 安装器    │
                                                              │ 运行时    │
                                                              │ (ELF)    │
                                                              └──────────┘
```

### 4.2 安装数据流

```
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
│ .apppack │───▶│ 读取头部  │───▶│ zstd     │───▶│ 反序列化  │───▶│ 写入文件  │
│ 文件     │    │ 解析元数据 │    │ 解压缩   │    │ 为文件列表 │    │ 到磁盘   │
└──────────┘    └──────────┘    └──────────┘    └──────────┘    └──────────┘
```

### 4.3 完整安装执行流程

```
用户执行: ./MyApp.apppack --install
    │
    ▼
┌─────────────────────────────────────┐
│ 1. 解析命令行参数                    │
│    --install → 安装模式              │
│    --dest=... → 自定义安装路径       │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 2. 读取包头部信息                    │
│    应用名称、版本号、安装目录         │
│    数据偏移、数据大小、原始大小       │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 3. 版本检测                          │
│    检查安装目录是否存在              │
│    读取 .apppack-version            │
│    比较版本号决定安装类型            │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 4. 升级前清理 (仅升级/降级时)        │
│    执行 apppack-pre-upgrade.sh      │
│    清理旧的系统环境配置              │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 5. 安装前准备                        │
│    创建安装目录                      │
│    执行 apppack-pre-install.sh      │
│    (失败则终止安装)                  │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 6. 解压文件                          │
│    读取压缩数据                      │
│    zstd 解压缩                      │
│    反序列化文件列表                  │
│    写入文件到安装目录                │
│    设置文件权限                      │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 7. 创建卸载脚本                      │
│    生成 uninstall.sh                │
│    包含删除目录和桌面集成的命令       │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 8. 桌面集成                          │
│    复制 .desktop 到 ~/.local/share/ │
│    复制图标到 ~/.local/share/       │
│    更新桌面数据库                    │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 9. 安装后配置                        │
│    执行 apppack-post-install.sh     │
│    设置环境变量、注册服务等          │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 10. 安装完成                         │
│     显示安装信息                     │
│     输出卸载命令                     │
└─────────────────────────────────────┘
```

### 4.4 完整卸载执行流程

```
用户执行: ./MyApp.apppack --uninstall
    │
    ▼
┌─────────────────────────────────────┐
│ 1. 解析命令行参数                    │
│    --uninstall → 卸载模式            │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 2. 读取包头部信息                    │
│    获取应用名称和安装目录            │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 3. 执行卸载后脚本                    │
│    执行 apppack-post-uninstall.sh   │
│    清理环境变量、删除符号链接等       │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 4. 删除安装目录                      │
│    递归删除整个安装目录              │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 5. 清理桌面集成                      │
│    删除 .desktop 文件               │
│    删除图标文件                      │
│    更新桌面数据库                    │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 6. 卸载完成                          │
│    显示卸载成功信息                  │
└─────────────────────────────────────┘
```

---

## 5. 项目文件结构

```
app/
├── builder/                          # 打包工具 (app-builder)
│   ├── CMakeLists.txt                # CMake 构建配置
│   ├── main.cpp                      # 主入口，命令行参数解析
│   ├── common.h                      # 共享常量和结构体定义
│   ├── packager.h / packager.cpp     # 打包核心逻辑
│   ├── compressor.h / compressor.cpp # zstd 压缩
│   ├── file_utils.h / file_utils.cpp # 文件读写工具
│   ├── desktop_parser.h / desktop_parser.cpp # 桌面文件解析
│   ├── dependency_analyzer.h / dependency_analyzer.cpp # ldd 依赖分析
│   └── template_manager.h / template_manager.cpp # 项目模板管理
│
├── installer/                        # 安装器运行时 (app-installer)
│   ├── CMakeLists.txt                # CMake 构建配置
│   ├── main.cpp                      # 主入口，命令行参数解析
│   ├── common.h                      # 共享常量和结构体定义
│   ├── installer.h / installer.cpp   # 安装核心逻辑
│   ├── uninstaller.h / uninstaller.cpp # 卸载逻辑
│   ├── compressor.h / compressor.cpp # zstd 解压缩
│   ├── file_utils.h / file_utils.cpp # 文件读写工具
│   ├── header_reader.h / header_reader.cpp # 包头部解析
│   ├── desktop_integration.h / desktop_integration.cpp # 桌面集成
│   └── script_runner.h / script_runner.cpp # 生命周期脚本执行
│
├── examples/hello/                   # 示例应用
│   ├── CMakeLists.txt                # CMake 构建配置
│   ├── main.cpp                      # HelloWorld 应用源码
│   └── apppack/                      # 项目模板示例
│       ├── apppack.json              # 打包配置
│       ├── AppRun                    # 启动脚本
│       ├── HelloWorld.desktop        # 桌面文件
│       ├── HelloWorld.svg            # 应用图标
│       ├── apppack-pre-install.sh    # 安装前脚本
│       ├── apppack-post-install.sh   # 安装后脚本
│       ├── apppack-post-uninstall.sh # 卸载后脚本
│       └── apppack-pre-upgrade.sh    # 升级前脚本
│
├── doc/                              # 文档
│   ├── architecture.md               # 架构文档（本文档）
│   ├── developer-guide.md            # 开发者打包指南
│   └── user-guide.md                 # 用户使用指南
│
├── build.sh                          # 一键构建脚本
└── README.md                         # 项目说明
