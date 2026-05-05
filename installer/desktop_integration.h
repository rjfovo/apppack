#ifndef APPPACK_INSTALLER_DESKTOP_INTEGRATION_H
#define APPPACK_INSTALLER_DESKTOP_INTEGRATION_H

#include <string>
#include <vector>
#include "common.h"

// 创建桌面集成（desktop 文件、图标等）
void setupDesktopIntegration(
    const std::string& install_path,
    const std::string& app_name,
    const std::vector<FileEntry>& files
);

// 清理桌面集成
void cleanupDesktopIntegration(const std::string& app_name);

#endif // APPPACK_INSTALLER_DESKTOP_INTEGRATION_H
