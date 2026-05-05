# AppPack - Linux 自解压安装包格式

## 概述

AppPack 是一种类似于 AppImage 的 Linux 应用打包格式，但提供了类似 Windows 安装程序的体验：

- **自包含**：程序包含所有依赖库，无需处理复杂的依赖关系
- **安装到指定目录**：像 Windows 的 exe 一样安装到指定路径
- **桌面集成**：自动创建桌面图标、开始菜单项
- **干净卸载**：提供卸载功能，完全移除安装的文件

## 项目结构

```
app/
├── builder/           # 打包工具源码
│   ├── CMakeLists.txt
│   └── main.cpp
├── installer/         # 安装器运行时源码
│   ├── CMakeLists.txt
│   └── main.cpp
├── examples/          # 示例应用
│   └── hello/         # 一个简单的示例应用
│       ├── CMakeLists.txt
│       └── main.cpp
├── build.sh           # 构建脚本
└── README.md
```

## 使用方法

### 构建打包工具

```bash
chmod +x build.sh
./build.sh
```

### 打包应用

```bash
# 打包示例应用
./build/app-builder --appdir=examples/hello/build/hello-appdir --output=HelloWorld.apppack

# 或者打包任意应用目录
./build/app-builder --appdir=/path/to/appdir --output=MyApp.apppack
```

### 安装应用

```bash
chmod +x HelloWorld.apppack
./HelloWorld.apppack --install
```

### 卸载应用

```bash
# 方法1：运行安装包并指定卸载
./HelloWorld.apppack --uninstall

# 方法2：运行安装目录中的卸载程序
/opt/HelloWorld/uninstall.sh
```

## AppDir 目录结构

要打包的应用需要遵循以下目录结构：

```
my-appdir/
├── AppRun              # 应用启动脚本或可执行文件
├── myapp.desktop       # Desktop Entry 文件
├── myapp.png           # 应用图标 (至少 256x256)
└── usr/
    ├── bin/            # 可执行文件
    ├── lib/            # 依赖库
    └── share/          # 共享资源
```

### desktop 文件示例

```ini
[Desktop Entry]
Name=MyApp
Comment=My Application
Exec=myapp
Icon=myapp
Terminal=false
Type=Application
Categories=Utility;
```
