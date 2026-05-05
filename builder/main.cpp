/**
 * app-builder - AppPack 打包工具入口
 * 
 * 功能:
 *   --init:         在项目目录创建 apppack/ 模板目录
 *   --build:        从 apppack/ 目录构建安装包
 *   --appdir:       打包指定应用目录
 *   --output:       指定输出文件
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include "packager.h"
#include "template_manager.h"


namespace fs = std::filesystem;

// 安装器运行时的默认路径（由 CMake 编译定义覆盖）
#ifndef INSTALLER_PATH
#define INSTALLER_PATH "../installer/build/app-installer"
#endif

void printUsage() {
    std::cout << "AppPack Builder - 创建自解压安装包" << std::endl;
    std::cout << std::endl;
    std::cout << "用法:" << std::endl;
    std::cout << "  app-builder --init [项目目录]              初始化项目模板" << std::endl;
    std::cout << "  app-builder --build [项目目录] [选项]      从 apppack/ 目录构建" << std::endl;
    std::cout << "  app-builder --appdir=<目录> --output=<文件> [选项]  打包应用目录" << std::endl;
    std::cout << std::endl;
    std::cout << "必要参数 (打包模式):" << std::endl;
    std::cout << "  --appdir=<目录>    要打包的应用目录路径" << std::endl;
    std::cout << "  --output=<文件>    输出的安装包路径" << std::endl;
    std::cout << std::endl;
    std::cout << "可选参数:" << std::endl;
    std::cout << "  --install-dir=<路径>  默认安装目录 (默认: /opt/<应用名>)" << std::endl;
    std::cout << "  --version=<版本号>    应用版本号 (默认: 1.0.0)" << std::endl;
    std::cout << "  --bundle-deps         自动收集并打包动态库依赖" << std::endl;
    std::cout << "  --help                显示此帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  app-builder --init                          # 在当前目录创建模板" << std::endl;
    std::cout << "  app-builder --init /path/to/project         # 在指定目录创建模板" << std::endl;
    std::cout << "  app-builder --build                         # 从当前目录的 apppack/ 构建" << std::endl;
    std::cout << "  app-builder --build --bundle-deps           # 构建并打包依赖" << std::endl;
    std::cout << "  app-builder --appdir=apppack --output=MyApp.apppack  # 打包指定目录" << std::endl;
}

int main(int argc, char* argv[]) {
    // 检查是否有参数
    if (argc < 2) {
        printUsage();
        return 1;
    }
    
    std::string first_arg = argv[1];
    
    // ============================================================
    // --init: 初始化项目模板
    // ============================================================
    if (first_arg == "--init") {
        fs::path project_dir;
        if (argc >= 3) {
            project_dir = fs::absolute(argv[2]);
        } else {
            project_dir = fs::current_path();
        }
        
        bool success = initAppPackProject(project_dir);
        return success ? 0 : 1;
    }
    
    // ============================================================
    // --build: 从 apppack/ 目录构建
    // ============================================================
    if (first_arg == "--build") {
        fs::path project_dir;
        std::string output_path;
        bool bundle_deps = false;
        
        // 解析参数
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg.find("--output=") == 0) {
                output_path = arg.substr(9);
            } else if (arg == "--bundle-deps") {
                bundle_deps = true;
            } else if (arg.find("--") != 0) {
                // 非选项参数作为项目目录
                project_dir = fs::absolute(arg);
            }
        }
        
        if (project_dir.empty()) {
            project_dir = fs::current_path();
        }
        
        std::cout << "从项目目录构建: " << project_dir.string() << std::endl;
        
        bool success = buildFromAppPackDir(project_dir, output_path, INSTALLER_PATH, bundle_deps);
        return success ? 0 : 1;
    }
    
    // ============================================================
    // --help: 显示帮助
    // ============================================================
    if (first_arg == "--help") {
        printUsage();
        return 0;
    }
    
    // ============================================================
    // 传统打包模式: --appdir --output
    // ============================================================
    PackageConfig config;
    config.installer_path = INSTALLER_PATH;
    config.bundle_deps = false;
    config.overwrite = true;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.find("--appdir=") == 0) {
            config.appdir_path = arg.substr(9);
        } else if (arg.find("--output=") == 0) {
            config.output_path = arg.substr(9);
        } else if (arg.find("--install-dir=") == 0) {
            config.install_dir = arg.substr(14);
        } else if (arg.find("--version=") == 0) {
            config.version = arg.substr(10);
        } else if (arg == "--bundle-deps") {
            config.bundle_deps = true;
        } else if (arg == "--help") {
            printUsage();
            return 0;
        } else if (arg != "--init" && arg != "--build") {
            std::cerr << "未知参数: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }
    
    if (config.appdir_path.empty() || config.output_path.empty()) {
        std::cerr << "错误: --appdir 和 --output 是必要参数" << std::endl;
        printUsage();
        return 1;
    }
    
    // 尝试从 apppack.json 读取配置（如果存在）
    fs::path appdir_path(config.appdir_path);
    fs::path config_path = appdir_path / "apppack.json";
    if (fs::exists(config_path)) {
        std::ifstream config_file(config_path);
        std::stringstream buffer;
        buffer << config_file.rdbuf();
        std::string content = buffer.str();
        
        // 简单解析 JSON 字段
        auto extractJsonValue = [](const std::string& json, const std::string& key) -> std::string {
            std::string search = "\"" + key + "\": \"";
            size_t pos = json.find(search);
            if (pos == std::string::npos) return "";
            pos += search.length();
            size_t end = json.find("\"", pos);
            if (end == std::string::npos) return "";
            return json.substr(pos, end - pos);
        };
        
        // 仅当命令行未指定时，从 apppack.json 读取
        if (config.install_dir.empty()) {
            config.install_dir = extractJsonValue(content, "install_dir");
        }
        if (config.version.empty()) {
            config.version = extractJsonValue(content, "version");
        }
    }

    
    bool success = createPackage(config);

    return success ? 0 : 1;
}
