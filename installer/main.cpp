/**
 * app-installer - AppPack 自解压安装包运行时入口
 * 
 * 嵌入到 .apppack 文件头部，运行时读取自身尾部附加的压缩数据，
 * 提供安装、卸载、查看信息等功能。
 * 
 * 支持:
 *   - 首次安装
 *   - 版本检测与升级安装
 *   - 覆盖/降级安装
 *   - 干净卸载
 */

#include <iostream>
#include <string>
#include <filesystem>
#include <unistd.h>
#include "common.h"
#include "file_utils.h"
#include "header_reader.h"
#include "installer.h"
#include "uninstaller.h"

namespace fs = std::filesystem;

void showInfo(const PackageHeader& header) {
    std::cout << "==========================================" << std::endl;
    std::cout << "  AppPack 安装包信息" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "  应用名称: " << header.app_name << std::endl;
    std::cout << "  版本号:   v" << header.version << std::endl;
    std::cout << "  安装目录: " << header.install_dir << std::endl;
    std::cout << "  数据偏移: " << header.data_offset << std::endl;
    std::cout << "  数据大小: " << header.data_size << " 字节" << std::endl;
    std::cout << "  原始大小: " << header.orig_size << " 字节" << std::endl;
    std::cout << "  校验和: 0x" << std::hex << header.checksum << std::dec << std::endl;
    std::cout << "==========================================" << std::endl;
}

void printUsage() {
    std::cout << "AppPack 自解压安装包" << std::endl;
    std::cout << std::endl;
    std::cout << "用法: " << std::endl;
    std::cout << "  ./<package>.apppack --install [--dest=<目录>]   安装/升级应用" << std::endl;
    std::cout << "  ./<package>.apppack --uninstall                 卸载应用" << std::endl;
    std::cout << "  ./<package>.apppack --info                      查看包信息" << std::endl;
    std::cout << "  ./<package>.apppack --help                      显示帮助" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --dest=<目录>    指定安装目录 (默认: /opt/<应用名>)" << std::endl;
    std::cout << std::endl;
    std::cout << "特性:" << std::endl;
    std::cout << "  ✓ 自动检测已安装版本，支持升级安装" << std::endl;
    std::cout << "  ✓ 自动收集并打包动态库依赖 (使用 --bundle-deps 打包)" << std::endl;
    std::cout << "  ✓ 创建桌面图标和菜单项" << std::endl;
    std::cout << "  ✓ 支持干净卸载" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string action;
    std::string custom_dest;
    
    // 解析参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--install") {
            action = "install";
        } else if (arg == "--uninstall") {
            action = "uninstall";
        } else if (arg == "--info") {
            action = "info";
        } else if (arg == "--help") {
            action = "help";
        } else if (arg.find("--dest=") == 0) {
            custom_dest = arg.substr(7);
        } else {
            std::cerr << "未知参数: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }
    
    if (action.empty() || action == "help") {
        printUsage();
        return 0;
    }
    
    try {
        // 获取自身路径
        std::string self_path = getSelfPath();
        if (self_path.empty()) {
            std::cerr << "错误: 无法获取程序路径" << std::endl;
            return 1;
        }
        
        // 读取头部
        PackageHeader header = readHeader(self_path);
        
        if (action == "info") {
            showInfo(header);
            return 0;
        }
        
        if (action == "install") {
            std::string install_path = custom_dest.empty() ? 
                std::string(header.install_dir) : custom_dest;
            
            // 展开 ~ 为 $HOME
            if (install_path.size() > 0 && install_path[0] == '~') {
                const char* home = getenv("HOME");
                if (home) {
                    install_path = std::string(home) + install_path.substr(1);
                }
            }
            
            // 检查权限 - 如果安装到系统目录，可能需要 sudo
            if (install_path.find("/opt/") == 0 || install_path.find("/usr/") == 0) {
                std::string parent = fs::path(install_path).parent_path().string();
                if (access(parent.c_str(), W_OK) != 0) {
                    std::cerr << "警告: 没有写入 " << parent << " 的权限" << std::endl;
                    std::cerr << "请使用 sudo 运行安装命令:" << std::endl;
                    std::cerr << "  sudo ./" << fs::path(self_path).filename().string() 
                              << " --install" << std::endl;
                    return 1;
                }
            }

            
            InstallResult result = doInstall(self_path, header, install_path);
            return result.success ? 0 : 1;
        }
        
        if (action == "uninstall") {
            bool success = doUninstall(self_path, header);
            return success ? 0 : 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
