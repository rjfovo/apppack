#ifndef APPPACK_BUILDER_TEMPLATE_MANAGER_H
#define APPPACK_BUILDER_TEMPLATE_MANAGER_H

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// 初始化 apppack 项目模板
bool initAppPackProject(const fs::path& project_dir);

// 从 apppack 目录构建安装包
bool buildFromAppPackDir(const fs::path& project_dir, 
                          const std::string& output_path,
                          const std::string& installer_path,
                          bool bundle_deps);

#endif // APPPACK_BUILDER_TEMPLATE_MANAGER_H
