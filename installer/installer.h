#ifndef APPPACK_INSTALLER_INSTALLER_H
#define APPPACK_INSTALLER_INSTALLER_H

#include <string>
#include "common.h"

// 安装结果
struct InstallResult {
    bool success;
    bool was_upgrade;  // 是否是升级安装
    std::string old_version;  // 旧版本号（如果是升级）
    std::string new_version;  // 新版本号
};

// 执行安装操作
InstallResult doInstall(const std::string& self_path, const PackageHeader& header, 
                        const std::string& install_path);

#endif // APPPACK_INSTALLER_INSTALLER_H
