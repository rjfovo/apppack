#ifndef APPPACK_INSTALLER_SCRIPT_RUNNER_H
#define APPPACK_INSTALLER_SCRIPT_RUNNER_H

#include <string>
#include <vector>
#include "common.h"

// 脚本执行结果
struct ScriptResult {
    bool executed;     // 脚本是否存在并执行
    bool success;      // 执行是否成功
    int exit_code;     // 退出码
    std::string output; // 输出内容
};

// 执行安装前脚本 (apppack-pre-install.sh)
// 在解压文件之前执行，可用于下载依赖、检查环境等
ScriptResult runPreInstall(const std::string& install_path, 
                           const std::string& app_name,
                           const std::string& version);

// 执行安装后脚本 (apppack-post-install.sh)
// 在解压文件和创建桌面集成之后执行，可用于设置环境变量、注册服务等
ScriptResult runPostInstall(const std::string& install_path,
                            const std::string& app_name,
                            const std::string& version);

// 执行卸载后脚本 (apppack-post-uninstall.sh)
// 在删除安装目录和桌面集成之后执行，可用于清理系统环境变量、删除服务等
ScriptResult runPostUninstall(const std::string& install_path,
                              const std::string& app_name,
                              const std::string& version);

// 执行升级前脚本 (apppack-pre-upgrade.sh)
// 在升级/降级安装时，解压新文件之前执行，用于清理旧的系统环境配置
// 这样新的 post-install 脚本可以重新设置新的配置
ScriptResult runPreUpgrade(const std::string& install_path,
                           const std::string& app_name,
                           const std::string& version);

#endif // APPPACK_INSTALLER_SCRIPT_RUNNER_H
