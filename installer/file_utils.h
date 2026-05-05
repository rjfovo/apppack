#ifndef APPPACK_INSTALLER_FILE_UTILS_H
#define APPPACK_INSTALLER_FILE_UTILS_H

#include <string>
#include <vector>
#include "common.h"

// 复用公共文件工具函数（readFile, writeFile, deserializeFiles 等）
#include "../common/file_utils.h"

// 读取文件指定范围的内容
std::vector<char> readFileRange(const std::string& path, uint64_t offset, uint64_t size);

// 创建目录（包括父目录）
void createDirectories(const std::string& path);

// 设置文件权限
void setPermissions(const std::string& path, mode_t permissions);

// 获取当前可执行文件路径
std::string getSelfPath();

#endif // APPPACK_INSTALLER_FILE_UTILS_H
