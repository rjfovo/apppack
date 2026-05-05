#ifndef APPPACK_BUILDER_DEPENDENCY_ANALYZER_H
#define APPPACK_BUILDER_DEPENDENCY_ANALYZER_H

#include <string>
#include <vector>
#include <filesystem>
#include "common.h"

namespace fs = std::filesystem;

// 分析 ELF 文件的动态库依赖
// 返回依赖的库文件列表（已收集到目标目录）
std::vector<FileEntry> collectDependencies(const fs::path& appdir);

// 检查系统上是否安装了指定的库
bool checkLibraryInstalled(const std::string& soname);

// 获取库的详细信息
DependencyInfo resolveLibrary(const std::string& soname);

#endif // APPPACK_BUILDER_DEPENDENCY_ANALYZER_H
