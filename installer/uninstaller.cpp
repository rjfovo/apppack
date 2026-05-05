#include "uninstaller.h"
#include "file_utils.h"
#include "desktop_integration.h"
#include "script_runner.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

bool doUninstall(const std::string& self_path, const PackageHeader& header) {
    try {
        std::string install_path(header.install_dir);
        
        std::cout << "正在卸载 " << header.app_name << "..." << std::endl;
        
        // 先执行卸载后清理脚本（在删除目录之前）
        ScriptResult cleanup_result = runPostUninstall(install_path, header.app_name, header.version);
        if (cleanup_result.executed && !cleanup_result.success) {
            std::cerr << "  警告: 卸载后脚本执行失败" << std::endl;
        }
        
        // 删除安装目录
        if (fs::exists(install_path)) {
            fs::remove_all(install_path);
            std::cout << "  已删除: " << install_path << std::endl;
        } else {
            std::cout << "  安装目录不存在: " << install_path << std::endl;
        }
        
        // 清理桌面集成
        cleanupDesktopIntegration(header.app_name);
        
        std::cout << std::endl;
        std::cout << "✓ " << header.app_name << " 已成功卸载!" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "卸载失败: " << e.what() << std::endl;
        return false;
    }
}
