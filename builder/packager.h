#ifndef APPPACK_BUILDER_PACKAGER_H
#define APPPACK_BUILDER_PACKAGER_H

#include <string>
#include "common.h"

// 打包配置
struct PackageConfig {
    std::string appdir_path;     // 应用目录路径
    std::string output_path;     // 输出文件路径
    std::string install_dir;     // 默认安装目录
    std::string installer_path;  // 安装器运行时路径
    std::string version;         // 应用版本号
    bool bundle_deps;            // 是否打包动态库依赖
    bool overwrite;              // 是否允许覆盖已安装版本
};

// 执行打包操作
bool createPackage(const PackageConfig& config);

#endif // APPPACK_BUILDER_PACKAGER_H
