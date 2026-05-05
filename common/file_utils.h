#ifndef APPPACK_COMMON_FILE_UTILS_H
#define APPPACK_COMMON_FILE_UTILS_H

#include <string>
#include <vector>
#include <filesystem>
#include "common.h"

namespace fs = std::filesystem;

// 读取整个文件到 vector
std::vector<char> readFile(const fs::path& path);

// 写入文件
void writeFile(const fs::path& path, const std::vector<char>& data);

// 收集目录中所有文件
std::vector<FileEntry> collectFiles(const fs::path& appdir);

// 序列化文件列表为二进制数据
std::vector<char> serializeFiles(const std::vector<FileEntry>& files);

// 反序列化文件列表
std::vector<FileEntry> deserializeFiles(const std::vector<char>& data);

// 计算简单校验和
uint32_t calculateChecksum(const std::vector<char>& data);

#endif // APPPACK_COMMON_FILE_UTILS_H
