#ifndef APPPACK_INSTALLER_DESKTOP_INTEGRATION_H
#define APPPACK_INSTALLER_DESKTOP_INTEGRATION_H

#include <string>
#include <vector>
#include "common.h"

// 创建桌面集成（desktop 文件、图标等）
// system_wide: true=安装到系统目录 /usr/share/，false=安装到用户目录 ~/.local/share/
void setupDesktopIntegration(
    const std::string& install_path,
    const std::string& app_name,
    const std::vector<FileEntry>& files,
    bool system_wide = false
);

// 清理桌面集成
void cleanupDesktopIntegration(const std::string& app_name);

#endif // APPPACK_INSTALLER_DESKTOP_INTEGRATION_H
