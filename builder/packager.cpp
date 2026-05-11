#include "packager.h"
#include "file_utils.h"
#include "compressor.h"
#include "desktop_parser.h"
#include "dependency_analyzer.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

bool createPackage(const PackageConfig& config) {
    try {
        fs::path appdir(config.appdir_path);
        if (!fs::exists(appdir)) {
            std::cerr << "错误: 应用目录不存在: " << config.appdir_path << std::endl;
            return false;
        }
        
        std::cout << "正在收集文件..." << std::endl;
        auto files = collectFiles(appdir);
        std::cout << "  发现 " << files.size() << " 个应用文件" << std::endl;
        
        // 获取应用名称
        std::string app_name = parseAppName(appdir);
        std::string install_dir = config.install_dir.empty() ? 
            "/opt/" + app_name : config.install_dir;
        
        std::cout << "  应用名称: " << app_name << std::endl;
        std::cout << "  安装目录: " << install_dir << std::endl;
        std::cout << "  版本号: " << (config.version.empty() ? "1.0.0" : config.version) << std::endl;
        
        // 收集动态库依赖
        if (config.bundle_deps) {
            auto deps = collectDependencies(appdir);
            files.insert(files.end(), deps.begin(), deps.end());
        }
        
        std::cout << "正在序列化文件..." << std::endl;
        auto serialized = serializeFiles(files);
        std::cout << "  原始大小: " << serialized.size() << " 字节" << std::endl;
        
        std::cout << "正在压缩数据 (zstd)..." << std::endl;
        auto compressed = compressData(serialized);
        std::cout << "  压缩后大小: " << compressed.size() << " 字节" << std::endl;
        std::cout << "  压缩率: " << std::fixed << std::setprecision(1) 
                  << (100.0 * compressed.size() / serialized.size()) << "%" << std::endl;
        
        // 读取安装器运行时
        std::cout << "正在读取安装器运行时..." << std::endl;
        std::vector<char> installer = readFile(config.installer_path);
        std::cout << "  运行时大小: " << installer.size() << " 字节" << std::endl;
        
        // 创建包头部
        PackageHeader header;
        header.data_offset = installer.size() + sizeof(PackageHeader);
        header.data_size = compressed.size();
        header.orig_size = serialized.size();
        std::strncpy(header.app_name, app_name.c_str(), sizeof(header.app_name) - 1);
        std::strncpy(header.install_dir, install_dir.c_str(), sizeof(header.install_dir) - 1);
        if (!config.version.empty()) {
            std::strncpy(header.version, config.version.c_str(), sizeof(header.version) - 1);
        }
        header.checksum = calculateChecksum(compressed);
        
        // 从 apppack.json 读取 flags 配置
        // 默认 flags=0（所有功能禁用），根据配置开启需要的功能
        header.flags = 0;
        fs::path appdir_path(config.appdir_path);
        fs::path config_path = appdir_path / "apppack.json";
        if (fs::exists(config_path)) {
            std::ifstream config_file(config_path);
            std::stringstream buffer;
            buffer << config_file.rdbuf();
            std::string content = buffer.str();
            
            // 辅助函数：提取 JSON 字符串值
            auto extractJsonString = [](const std::string& json, const std::string& key) -> std::string {
                std::string search = "\"" + key + "\": \"";
                size_t pos = json.find(search);
                if (pos == std::string::npos) return "";
                pos += search.length();
                size_t end = json.find("\"", pos);
                if (end == std::string::npos) return "";
                return json.substr(pos, end - pos);
            };
            
            // 解析 create_desktop_entry: "disabled" | "user" | "system"
            std::string desktop_entry = extractJsonString(content, "create_desktop_entry");
            if (desktop_entry == "system") {
                header.flags |= DESKTOP_ENTRY_SYSTEM;
                std::cout << "  桌面图标: 系统目录 (create_desktop_entry=system)" << std::endl;
            } else if (desktop_entry == "user") {
                header.flags |= DESKTOP_ENTRY_USER;
                std::cout << "  桌面图标: 用户目录 (create_desktop_entry=user)" << std::endl;
            } else {
                std::cout << "  桌面图标: 已禁用 (create_desktop_entry=disabled)" << std::endl;
            }
        }
        
        // 写入输出文件
        std::cout << "正在生成安装包..." << std::endl;
        std::ofstream out(config.output_path, std::ios::binary);
        if (!out) {
            throw std::runtime_error("无法创建输出文件: " + config.output_path);
        }
        
        // 写入安装器运行时
        out.write(installer.data(), installer.size());
        
        // 写入头部
        out.write(reinterpret_cast<const char*>(&header), sizeof(header));
        
        // 写入压缩数据
        out.write(compressed.data(), compressed.size());
        
        out.close();
        
        // 设置可执行权限
        fs::permissions(config.output_path, 
            fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write |
            fs::perms::group_exec | fs::perms::group_read |
            fs::perms::others_exec | fs::perms::others_read);
        
        std::cout << std::endl;
        std::cout << "✓ 安装包创建成功!" << std::endl;
        std::cout << "  输出: " << config.output_path << std::endl;
        std::cout << "  总大小: " << (installer.size() + sizeof(header) + compressed.size()) << " 字节" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return false;
    }
}
