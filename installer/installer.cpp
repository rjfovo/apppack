#include "installer.h"
#include "file_utils.h"
#include "compressor.h"
#include "desktop_integration.h"
#include "script_runner.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

// 读取已安装应用的版本信息
static std::string getInstalledVersion(const std::string& install_path) {
    std::string version_file = install_path + "/.apppack-version";
    if (fs::exists(version_file)) {
        std::ifstream vf(version_file);
        std::string version;
        std::getline(vf, version);
        return version;
    }
    return "";
}

// 比较版本号 (返回 true 如果 v1 < v2)
static bool isVersionLess(const std::string& v1, const std::string& v2) {
    auto split = [](const std::string& v) -> std::vector<int> {
        std::vector<int> parts;
        std::string current;
        for (char c : v) {
            if (c == '.') {
                parts.push_back(std::stoi(current));
                current.clear();
            } else if (c >= '0' && c <= '9') {
                current += c;
            }
        }
        if (!current.empty()) parts.push_back(std::stoi(current));
        return parts;
    };
    
    auto p1 = split(v1);
    auto p2 = split(v2);
    
    size_t max_len = std::max(p1.size(), p2.size());
    p1.resize(max_len, 0);
    p2.resize(max_len, 0);
    
    for (size_t i = 0; i < max_len; i++) {
        if (p1[i] < p2[i]) return true;
        if (p1[i] > p2[i]) return false;
    }
    return false;
}

InstallResult doInstall(const std::string& self_path, const PackageHeader& header, 
                        const std::string& install_path) {
    InstallResult result;
    result.success = false;
    result.was_upgrade = false;
    result.new_version = header.version;
    
    try {
        std::cout << "正在安装 " << header.app_name << " v" << header.version << "..." << std::endl;
        std::cout << "  安装到: " << install_path << std::endl;
        
        // 检查是否已安装
        std::string old_version = getInstalledVersion(install_path);
        if (!old_version.empty()) {
            std::cout << "  检测到已安装版本: v" << old_version << std::endl;
            result.old_version = old_version;
            
            if (old_version == header.version) {
                std::cout << "  相同版本，执行覆盖安装..." << std::endl;
            } else if (isVersionLess(old_version, header.version)) {
                std::cout << "  ✓ 检测到新版本，执行升级安装..." << std::endl;
                result.was_upgrade = true;
            } else {
                std::cout << "  警告: 安装的版本 (" << header.version 
                          << ") 低于已安装版本 (" << old_version << ")" << std::endl;
                std::cout << "  将执行降级安装..." << std::endl;
            }
        }
        
        // ========== 升级前清理（仅升级/降级时执行）==========
        if (!old_version.empty()) {
            // 执行升级前脚本，清理旧的系统环境配置
            // 这样新的 post-install 脚本可以重新设置新的配置
            ScriptResult upgrade_result = runPreUpgrade(install_path, header.app_name, header.version);
            if (upgrade_result.executed && !upgrade_result.success) {
                std::cerr << "  警告: 升级前脚本执行失败，继续安装..." << std::endl;
            }
        }
        
        // ========== 安装前准备 ==========
        // 创建安装目录（先创建，以便安装前脚本可以使用）
        createDirectories(install_path);
        
        // 执行安装前脚本
        ScriptResult pre_result = runPreInstall(install_path, header.app_name, header.version);
        if (pre_result.executed && !pre_result.success) {
            std::cerr << "  安装前脚本执行失败，终止安装" << std::endl;
            return result;
        }
        
        // ========== 解压文件 ==========
        // 读取压缩数据
        std::cout << "  正在读取包数据..." << std::endl;
        auto compressed = readFileRange(self_path, header.data_offset, header.data_size);
        
        // 解压数据
        std::cout << "  正在解压数据..." << std::endl;
        auto decompressed = decompressData(compressed, header.orig_size);
        
        // 反序列化文件
        std::cout << "  正在提取文件..." << std::endl;
        auto files = deserializeFiles(decompressed);
        
        // 写入文件
        for (const auto& file : files) {
            std::string full_path = install_path + "/" + file.relative_path;
            
            // 创建父目录
            std::string parent = fs::path(full_path).parent_path().string();
            createDirectories(parent);
            
            // 写入文件
            writeFile(full_path, file.content);
            
            // 设置权限
            setPermissions(full_path, file.permissions);
        }
        
        // 写入版本信息文件
        std::string version_file = install_path + "/.apppack-version";
        std::ofstream vf(version_file);
        vf << header.version << std::endl;
        vf.close();
        
        // ========== 创建卸载脚本 ==========
        std::string uninstall_script = install_path + "/uninstall.sh";
        std::ofstream us(uninstall_script);
        us << "#!/bin/bash" << std::endl;
        us << "# " << header.app_name << " 卸载脚本" << std::endl;
        us << "echo \"正在卸载 " << header.app_name << "...\"" << std::endl;
        us << std::endl;
        us << "# 执行卸载后清理脚本（如果存在）" << std::endl;
        us << "if [ -f \"$APPPACK_INSTALL_DIR/" << SCRIPT_POST_UNINSTALL << "\" ]; then" << std::endl;
        us << "    export APPPACK_INSTALL_DIR=\"" << install_path << "\"" << std::endl;
        us << "    export APPPACK_APP_NAME=\"" << header.app_name << "\"" << std::endl;
        us << "    export APPPACK_VERSION=\"" << header.version << "\"" << std::endl;
        us << "    bash \"$APPPACK_INSTALL_DIR/" << SCRIPT_POST_UNINSTALL << "\"" << std::endl;
        us << "fi" << std::endl;
        us << std::endl;
        us << "# 删除安装目录" << std::endl;
        us << "rm -rf \"" << install_path << "\"" << std::endl;
        us << std::endl;
        us << "# 删除桌面图标" << std::endl;
        us << "rm -f ~/.local/share/applications/" << header.app_name << ".desktop" << std::endl;
        us << "rm -f ~/Desktop/" << header.app_name << ".desktop" << std::endl;
        us << "rm -f ~/.local/share/icons/" << header.app_name << ".png" << std::endl;
        us << "rm -f ~/.local/share/icons/" << header.app_name << ".svg" << std::endl;
        us << "rm -f ~/.local/share/icons/" << header.app_name << ".xpm" << std::endl;
        us << std::endl;
        us << "# 更新桌面数据库" << std::endl;
        us << "if command -v update-desktop-database &> /dev/null; then" << std::endl;
        us << "    update-desktop-database ~/.local/share/applications/ 2>/dev/null" << std::endl;
        us << "fi" << std::endl;
        us << std::endl;
        us << "echo \"" << header.app_name << " 已成功卸载\"" << std::endl;
        us.close();
        chmod(uninstall_script.c_str(), 0755);
        
        // ========== 创建桌面集成 ==========
        setupDesktopIntegration(install_path, header.app_name, files);
        
        // ========== 安装后处理 ==========
        // 执行安装后脚本
        ScriptResult post_result = runPostInstall(install_path, header.app_name, header.version);
        if (post_result.executed && !post_result.success) {
            std::cerr << "  警告: 安装后脚本执行失败，但安装已完成" << std::endl;
        }
        
        std::cout << std::endl;
        if (result.was_upgrade) {
            std::cout << "✓ " << header.app_name << " 升级成功!" << std::endl;
            std::cout << "  " << old_version << " → " << header.version << std::endl;
        } else {
            std::cout << "✓ " << header.app_name << " 安装成功!" << std::endl;
        }
        std::cout << "  安装路径: " << install_path << std::endl;
        std::cout << "  卸载命令: " << install_path << "/uninstall.sh" << std::endl;
        
        result.success = true;
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "安装失败: " << e.what() << std::endl;
        return result;
    }
}
