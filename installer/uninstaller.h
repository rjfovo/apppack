#ifndef APPPACK_INSTALLER_UNINSTALLER_H
#define APPPACK_INSTALLER_UNINSTALLER_H

#include <string>
#include "common.h"

// 执行卸载操作
bool doUninstall(const std::string& self_path, const PackageHeader& header);

#endif // APPPACK_INSTALLER_UNINSTALLER_H
