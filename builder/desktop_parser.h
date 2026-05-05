#ifndef APPPACK_BUILDER_DESKTOP_PARSER_H
#define APPPACK_BUILDER_DESKTOP_PARSER_H

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// 从 desktop 文件解析应用名称
std::string parseAppName(const fs::path& appdir);

#endif // APPPACK_BUILDER_DESKTOP_PARSER_H
