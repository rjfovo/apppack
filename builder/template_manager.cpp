/**
 * template_manager.cpp - AppPack 项目模板管理
 * 
 * 提供类似 deb 包 debian/ 目录的功能：
 *   - apppack init: 在项目目录创建 apppack/ 模板目录
 *   - apppack build: 从 apppack/ 目录构建安装包
 */

#include "template_manager.h"
#include "packager.h"
#include "file_utils.h"
#include "common.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>

namespace fs = std::filesystem;

// ============================================================
// 辅助函数
// ============================================================

static void writeFileContent(const fs::path& path, const std::string& content, bool executable = false) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("无法创建文件: " + path.string());
    }
    out << content;
    out.close();
    
    if (executable) {
        fs::permissions(path,
            fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write |
            fs::perms::group_exec | fs::perms::group_read |
            fs::perms::others_exec | fs::perms::others_read);
    }
}

static bool promptYesNo(const std::string& question) {
    std::cout << question << " [Y/n]: ";
    std::string answer;
    std::getline(std::cin, answer);
    return answer.empty() || answer == "Y" || answer == "y" || answer == "yes";
}

static std::string promptInput(const std::string& prompt, const std::string& default_value) {
    std::cout << prompt << " [" << default_value << "]: ";
    std::string answer;
    std::getline(std::cin, answer);
    return answer.empty() ? default_value : answer;
}

// ============================================================
// 初始化项目模板
// ============================================================

bool initAppPackProject(const fs::path& project_dir) {
    try {
        fs::path apppack_dir = project_dir / "apppack";
        
        // 检查是否已存在
        if (fs::exists(apppack_dir)) {
            if (!promptYesNo("apppack/ 目录已存在，是否覆盖？")) {
                std::cout << "已取消。" << std::endl;
                return true;
            }
            fs::remove_all(apppack_dir);
        }
        
        std::cout << "正在创建 AppPack 项目模板..." << std::endl;
        std::cout << "  项目目录: " << project_dir.string() << std::endl;
        std::cout << std::endl;
        
        // 交互式配置
        std::cout << "请输入应用信息（直接回车使用默认值）:" << std::endl;
        std::string app_name = promptInput("  应用名称", "MyApp");
        std::string version = promptInput("  版本号", "1.0.0");
        std::string description = promptInput("  应用描述", "我的应用程序");
        std::string install_dir = promptInput("  安装目录", "/opt/" + app_name);
        std::string executable = promptInput("  可执行文件名称", "myapp");
        std::string categories = promptInput("  应用类别 (参考: Utility;Development;Graphics;Network;Office;Game;AudioVideo;Settings;System)", "Utility;");
        
        bool terminal = false;
        std::cout << "  是否在终端中运行？ [y/N]: ";
        std::string term_answer;
        std::getline(std::cin, term_answer);
        terminal = (term_answer == "Y" || term_answer == "y" || term_answer == "yes");
        
        bool bundle_deps = false;
        std::cout << "  是否自动打包依赖库？ [y/N]: ";
        std::string deps_answer;
        std::getline(std::cin, deps_answer);
        bundle_deps = (deps_answer == "Y" || deps_answer == "y" || deps_answer == "yes");
        
        std::cout << std::endl;
        
        // 创建目录结构
        fs::create_directories(apppack_dir / "usr" / "bin");
        fs::create_directories(apppack_dir / "usr" / "lib");
        fs::create_directories(apppack_dir / "usr" / "share" / "icons");
        fs::create_directories(apppack_dir / "usr" / "share" / "applications");
        
        std::cout << "  ✓ 创建目录结构" << std::endl;
        
        // 生成 apppack.json 配置文件
        std::ostringstream json;
        json << "{\n";
        json << "    \"app_name\": \"" << app_name << "\",\n";
        json << "    \"version\": \"" << version << "\",\n";
        json << "    \"install_dir\": \"" << install_dir << "\",\n";
        json << "    \"description\": \"" << description << "\",\n";
        json << "    \"executable\": \"" << executable << "\",\n";
        json << "    \"maintainer\": \"Your Name <your@email.com>\",\n";
        json << "    \"categories\": \"" << categories << "\",\n";
        json << "    \"terminal\": " << (terminal ? "true" : "false") << ",\n";
        json << "    \"bundle_deps\": " << (bundle_deps ? "true" : "false") << ",\n";
        json << "    \"scripts\": {\n";
        json << "        \"pre_install\": \"apppack-pre-install.sh\",\n";
        json << "        \"post_install\": \"apppack-post-install.sh\",\n";
        json << "        \"post_uninstall\": \"apppack-post-uninstall.sh\"\n";
        json << "    }\n";
        json << "}\n";
        writeFileContent(apppack_dir / "apppack.json", json.str());
        std::cout << "  ✓ 创建 apppack.json (配置文件)" << std::endl;
        
        // 生成 AppRun 启动脚本
        std::string apprun_content =
            "#!/bin/bash\n"
            "# AppRun - 应用启动脚本\n"
            "# 由 apppack init 自动生成\n"
            "\n"
            "APPDIR=\"$(dirname \"$(readlink -f \"$0\")\")\"\n"
            "\n"
            "# 设置库路径（如果有 bundled 库）\n"
            "if [ -d \"${APPDIR}/usr/lib\" ]; then\n"
            "    export LD_LIBRARY_PATH=\"${APPDIR}/usr/lib:${LD_LIBRARY_PATH}\"\n"
            "fi\n"
            "\n"
            "# 设置安装目录环境变量\n"
            "export APPPACK_INSTALL_DIR=\"${APPDIR}\"\n"
            "\n"
            "# 执行应用\n"
            "exec \"${APPDIR}/usr/bin/" + executable + "\" \"$@\"\n";
        writeFileContent(apppack_dir / "AppRun", apprun_content, true);
        std::cout << "  ✓ 创建 AppRun (启动脚本)" << std::endl;
        
        // 生成 .desktop 文件
        std::ostringstream desktop;
        desktop << "[Desktop Entry]\n";
        desktop << "Name=" << app_name << "\n";
        desktop << "Comment=" << description << "\n";
        desktop << "Exec=" << executable << "\n";
        desktop << "Icon=" << app_name << "\n";
        desktop << "Terminal=" << (terminal ? "true" : "false") << "\n";
        desktop << "Type=Application\n";
        desktop << "Categories=" << categories << "\n";
        desktop << "StartupNotify=true\n";
        writeFileContent(apppack_dir / (app_name + ".desktop"), desktop.str());
        std::cout << "  ✓ 创建 " << app_name << ".desktop (桌面文件)" << std::endl;
        
        // 生成 SVG 图标
        std::string icon_svg =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"256\" height=\"256\" viewBox=\"0 0 256 256\">\n"
            "  <rect width=\"256\" height=\"256\" rx=\"32\" fill=\"#4A90D9\"/>\n"
            "  <text x=\"128\" y=\"160\" font-family=\"Arial, sans-serif\" font-size=\"120\" font-weight=\"bold\" \n"
            "        fill=\"white\" text-anchor=\"middle\" dominant-baseline=\"middle\">" + app_name.substr(0, 1) + "</text>\n"
            "</svg>\n";
        writeFileContent(apppack_dir / (app_name + ".svg"), icon_svg);
        std::cout << "  ✓ 创建 " << app_name << ".svg (应用图标)" << std::endl;
        
        // 生成生命周期脚本 - 安装前
        std::string pre_install =
            "#!/bin/bash\n"
            "# apppack-pre-install.sh - 安装前执行脚本\n"
            "# 可用于检查环境、下载资源等\n"
            "\n"
            "echo \"  安装前检查...\"\n"
            "# 示例：检查磁盘空间\n"
            "# REQUIRED_SPACE=100  # MB\n"
            "# AVAILABLE_SPACE=$(df /opt | awk 'NR==2 {print $4}')\n"
            "# if [ \"$AVAILABLE_SPACE\" -lt \"$((REQUIRED_SPACE * 1024))\" ]; then\n"
            "#     echo \"错误: 磁盘空间不足\"\n"
            "#     exit 1\n"
            "# fi\n"
            "\n"
            "exit 0\n";
        writeFileContent(apppack_dir / "apppack-pre-install.sh", pre_install, true);
        std::cout << "  ✓ 创建 apppack-pre-install.sh (安装前脚本)" << std::endl;
        
        // 生成生命周期脚本 - 安装后
        std::string post_install =
            "#!/bin/bash\n"
            "# apppack-post-install.sh - 安装后执行脚本\n"
            "# 可用于设置环境变量、创建符号链接、注册服务等\n"
            "\n"
            "echo \"  安装后配置...\"\n"
            "echo \"    应用: ${APPPACK_APP_NAME}\"\n"
            "echo \"    版本: ${APPPACK_VERSION}\"\n"
            "echo \"    安装目录: ${APPPACK_INSTALL_DIR}\"\n"
            "\n"
            "# 示例：创建环境变量文件\n"
            "# cat > /etc/profile.d/myapp.sh << EOF\n"
            "# export MYAPP_HOME=${APPPACK_INSTALL_DIR}\n"
            "# export PATH=\\${MYAPP_HOME}/usr/bin:\\${PATH}\n"
            "# EOF\n"
            "\n"
            "exit 0\n";
        writeFileContent(apppack_dir / "apppack-post-install.sh", post_install, true);
        std::cout << "  ✓ 创建 apppack-post-install.sh (安装后脚本)" << std::endl;
        
        // 生成生命周期脚本 - 卸载后
        std::string post_uninstall =
            "#!/bin/bash\n"
            "# apppack-post-uninstall.sh - 卸载后执行脚本\n"
            "# 可用于清理环境变量、删除符号链接、清理配置等\n"
            "\n"
            "echo \"  卸载后清理...\"\n"
            "echo \"    应用: ${APPPACK_APP_NAME}\"\n"
            "\n"
            "# 示例：删除环境变量文件\n"
            "# rm -f /etc/profile.d/myapp.sh\n"
            "\n"
            "exit 0\n";
        writeFileContent(apppack_dir / "apppack-post-uninstall.sh", post_uninstall, true);
        std::cout << "  ✓ 创建 apppack-post-uninstall.sh (卸载后脚本)" << std::endl;
        
        // 生成生命周期脚本 - 升级前
        std::string pre_upgrade =
            "#!/bin/bash\n"
            "# apppack-pre-upgrade.sh - 升级/降级前执行脚本\n"
            "# 在安装新版本之前执行，用于清理旧的系统环境配置\n"
            "# 这样新的 post-install 脚本可以重新设置新的配置\n"
            "# 注意：此脚本仅在升级或降级安装时执行，首次安装不执行\n"
            "\n"
            "echo \"  升级前清理旧配置...\"\n"
            "echo \"    应用: ${APPPACK_APP_NAME}\"\n"
            "echo \"    新版本: ${APPPACK_VERSION}\"\n"
            "echo \"    安装目录: ${APPPACK_INSTALL_DIR}\"\n"
            "\n"
            "# 示例：清理旧的系统环境配置\n"
            "# rm -f /etc/profile.d/myapp.sh\n"
            "# rm -f /usr/local/bin/myapp\n"
            "# systemctl stop myapp.service 2>/dev/null || true\n"
            "# systemctl disable myapp.service 2>/dev/null || true\n"
            "# rm -f /etc/systemd/system/myapp.service\n"
            "\n"
            "exit 0\n";
        writeFileContent(apppack_dir / "apppack-pre-upgrade.sh", pre_upgrade, true);
        std::cout << "  ✓ 创建 apppack-pre-upgrade.sh (升级前脚本)" << std::endl;
        
        // 创建占位文件
        std::string placeholder = "// 请将你的可执行文件放在此目录\n// 例如: cp /path/to/your/program usr/bin/\n";
        writeFileContent(apppack_dir / "usr" / "bin" / ".gitkeep", placeholder);
        
        std::cout << std::endl;
        std::cout << "✓ AppPack 项目模板创建完成!" << std::endl;
        std::cout << std::endl;
        std::cout << "目录结构:" << std::endl;
        std::cout << "  apppack/" << std::endl;
        std::cout << "  ├── apppack.json              # 打包配置文件" << std::endl;
        std::cout << "  ├── AppRun                    # 应用启动脚本" << std::endl;
        std::cout << "  ├── " << app_name << ".desktop          # 桌面文件" << std::endl;
        std::cout << "  ├── " << app_name << ".svg              # 应用图标" << std::endl;
        std::cout << "  ├── apppack-pre-install.sh    # 安装前脚本" << std::endl;
        std::cout << "  ├── apppack-post-install.sh   # 安装后脚本" << std::endl;
        std::cout << "  ├── apppack-post-uninstall.sh # 卸载后脚本" << std::endl;
        std::cout << "  └── usr/" << std::endl;
        std::cout << "      ├── bin/                  # 可执行文件目录" << std::endl;
        std::cout << "      └── lib/                  # 依赖库目录" << std::endl;
        std::cout << std::endl;
        std::cout << "使用说明:" << std::endl;
        std::cout << "  1. 将你的可执行文件放入 apppack/usr/bin/" << std::endl;
        std::cout << "  2. 编辑 apppack/apppack.json 修改配置" << std::endl;
        std::cout << "  3. 运行打包命令:" << std::endl;
        std::cout << "     app-builder --appdir=apppack --output=MyApp.apppack" << std::endl;
        std::cout << "     或 (自动打包依赖):" << std::endl;
        std::cout << "     app-builder --appdir=apppack --output=MyApp.apppack --bundle-deps" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================
// 从 apppack 目录构建安装包
// ============================================================

bool buildFromAppPackDir(const fs::path& project_dir,
                          const std::string& output_path,
                          const std::string& installer_path,
                          bool bundle_deps) {
    try {
        fs::path apppack_dir = project_dir / "apppack";
        
        if (!fs::exists(apppack_dir)) {
            std::cerr << "错误: apppack/ 目录不存在: " << apppack_dir.string() << std::endl;
            std::cerr << "请先在项目目录中运行: app-builder --init" << std::endl;
            return false;
        }
        
        // 读取 apppack.json 配置
        fs::path config_path = apppack_dir / "apppack.json";
        std::string install_dir = "/opt/MyApp";
        std::string version = "1.0.0";
        std::string app_name = "MyApp";
        
        if (fs::exists(config_path)) {
            std::ifstream config_file(config_path);
            std::stringstream buffer;
            buffer << config_file.rdbuf();
            std::string content = buffer.str();
            
            // 简单解析 JSON 字段（不使用 JSON 库）
            auto extractJsonValue = [](const std::string& json, const std::string& key) -> std::string {
                std::string search = "\"" + key + "\": \"";
                size_t pos = json.find(search);
                if (pos == std::string::npos) {
                    // 尝试不带引号的值（如 boolean）
                    search = "\"" + key + "\": ";
                    pos = json.find(search);
                    if (pos == std::string::npos) return "";
                    pos += search.length();
                    size_t end = json.find_first_of(",\n}", pos);
                    if (end == std::string::npos) return "";
                    std::string val = json.substr(pos, end - pos);
                    // 去除空格和引号
                    val.erase(0, val.find_first_not_of(" \t\""));
                    val.erase(val.find_last_not_of(" \t\"") + 1);
                    return val;
                }
                pos += search.length();
                size_t end = json.find("\"", pos);
                if (end == std::string::npos) return "";
                return json.substr(pos, end - pos);
            };
            
            app_name = extractJsonValue(content, "app_name");
            if (app_name.empty()) app_name = "MyApp";
            
            version = extractJsonValue(content, "version");
            if (version.empty()) version = "1.0.0";
            
            install_dir = extractJsonValue(content, "install_dir");
            if (install_dir.empty()) install_dir = "/opt/" + app_name;
            
            std::cout << "  从 apppack.json 读取配置:" << std::endl;
            std::cout << "    应用名称: " << app_name << std::endl;
            std::cout << "    版本号: " << version << std::endl;
            std::cout << "    安装目录: " << install_dir << std::endl;
        } else {
            std::cout << "  未找到 apppack.json，使用默认配置" << std::endl;
        }
        
        // 构建输出路径
        std::string final_output = output_path;
        if (final_output.empty()) {
            final_output = project_dir.string() + "/" + app_name + "-" + version + ".apppack";
        }
        
        // 使用 packager 打包
        PackageConfig config;
        config.appdir_path = apppack_dir.string();
        config.output_path = final_output;
        config.install_dir = install_dir;
        config.installer_path = installer_path;
        config.version = version;
        config.bundle_deps = bundle_deps;
        config.overwrite = true;
        
        return createPackage(config);
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return false;
    }
}
