#include "desktop_integration.h"
#include "file_utils.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>

namespace fs = std::filesystem;

// 获取用户主目录
static std::string getUserHome() {
    const char* home_dir = getenv("HOME");
    if (!home_dir) {
        struct passwd* pw = getpwuid(getuid());
        home_dir = pw->pw_dir;
    }
    return std::string(home_dir);
}

// 获取文件扩展名（小写）
static std::string getExtension(const std::string& path) {
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) return "";
    std::string ext = path.substr(dot);
    for (auto& c : ext) c = tolower(c);
    return ext;
}

// 安装桌面图标到系统目录（所有用户可见）
static void installSystemWide(
    const std::string& install_path,
    const std::string& app_name,
    const std::vector<FileEntry>& files) {
    
    std::cout << "  正在创建系统级桌面图标..." << std::endl;
    
    std::string desktop_file_src;
    std::string icon_src;
    std::string icon_ext;
    
    for (const auto& file : files) {
        std::string ext = getExtension(file.relative_path);
        if (ext == ".desktop") {
            desktop_file_src = install_path + "/" + file.relative_path;
        }
        if (ext == ".png" || ext == ".svg" || ext == ".xpm") {
            icon_src = install_path + "/" + file.relative_path;
            icon_ext = ext;
        }
    }
    
    // 安装图标到系统目录
    if (!icon_src.empty() && fs::exists(icon_src)) {
        std::string icon_dir = "/usr/share/icons/hicolor/scalable/apps/";
        if (icon_ext == ".png") icon_dir = "/usr/share/icons/hicolor/48x48/apps/";
        createDirectories(icon_dir);
        std::string icon_dst = icon_dir + app_name + icon_ext;
        fs::copy_file(icon_src, icon_dst, fs::copy_options::overwrite_existing);
        chmod(icon_dst.c_str(), 0644);
        std::cout << "  图标已安装: " << icon_dst << std::endl;
    }
    
    // 安装 desktop 文件到系统目录
    if (!desktop_file_src.empty()) {
        std::string desktop_dst = "/usr/share/applications/" + app_name + ".desktop";
        createDirectories("/usr/share/applications/");
        
        std::ifstream df(desktop_file_src);
        std::ofstream df_out(desktop_dst);
        std::string line;
        
        while (std::getline(df, line)) {
            if (line.find("Exec=") == 0) {
                // 使用 AppRun 路径，这样普通用户也能执行
                std::string apprun_path = install_path + "/AppRun";
                if (fs::exists(apprun_path)) {
                    df_out << "Exec=" << apprun_path << std::endl;
                } else {
                    df_out << "Exec=" << install_path << "/taskmanager" << std::endl;
                }
            } else if (line.find("Icon=") == 0) {
                // 使用图标名称（不带路径和扩展名），系统会从图标主题查找
                df_out << "Icon=" << app_name << std::endl;
            } else {
                df_out << line << std::endl;
            }
        }
        df.close();
        df_out.close();
        chmod(desktop_dst.c_str(), 0644);
        std::cout << "  桌面入口已安装: " << desktop_dst << std::endl;
    }
    
    // 更新图标缓存
    system("gtk-update-icon-cache /usr/share/icons/hicolor/ 2>/dev/null || true");
    // 更新桌面数据库
    system("update-desktop-database /usr/share/applications/ 2>/dev/null || true");
}

// 安装桌面图标到当前用户目录
static void installUserLocal(
    const std::string& install_path,
    const std::string& app_name,
    const std::vector<FileEntry>& files) {
    
    std::cout << "  正在创建用户级桌面图标..." << std::endl;
    
    std::string desktop_file_src;
    std::string icon_src;
    std::string icon_ext;
    
    for (const auto& file : files) {
        std::string ext = getExtension(file.relative_path);
        if (ext == ".desktop") {
            desktop_file_src = install_path + "/" + file.relative_path;
        }
        if (ext == ".png" || ext == ".svg" || ext == ".xpm") {
            icon_src = install_path + "/" + file.relative_path;
            icon_ext = ext;
        }
    }
    
    std::string user_home = getUserHome();
    
    // 复制 desktop 文件到用户目录
    if (!desktop_file_src.empty()) {
        std::string desktop_dst = user_home + "/.local/share/applications/" + app_name + ".desktop";
        createDirectories(user_home + "/.local/share/applications");
        
        std::ifstream df(desktop_file_src);
        std::ofstream df_out(desktop_dst);
        std::string line;
        
        while (std::getline(df, line)) {
            if (line.find("Exec=") == 0) {
                std::string exec_val = line.substr(5);
                if (exec_val[0] != '/') {
                    std::string apprun_path = install_path + "/AppRun";
                    if (fs::exists(apprun_path)) {
                        df_out << "Exec=" << apprun_path << std::endl;
                    } else {
                        df_out << "Exec=" << install_path << "/" << exec_val << std::endl;
                    }
                } else {
                    df_out << line << std::endl;
                }
            } else if (line.find("Icon=") == 0 && !icon_src.empty()) {
                df_out << "Icon=" << icon_src << std::endl;
            } else if (line.find("Path=") == 0) {
                // 跳过原有的 Path
            } else {
                df_out << line << std::endl;
            }
        }
        df.close();
        df_out.close();
        
        // 也复制到桌面（如果桌面目录存在）
        std::string desktop_dir = user_home + "/Desktop";
        if (fs::exists(desktop_dir)) {
            std::string desktop_desktop = desktop_dir + "/" + app_name + ".desktop";
            fs::copy_file(desktop_dst, desktop_desktop, fs::copy_options::overwrite_existing);
            chmod(desktop_desktop.c_str(), 0755);
        }
        
        // 更新桌面数据库
        system("update-desktop-database ~/.local/share/applications/ 2>/dev/null");
    }
    
    // 复制图标到 ~/.local/share/icons/
    if (!icon_src.empty() && fs::exists(icon_src)) {
        std::string icon_dst = user_home + "/.local/share/icons/" + app_name + icon_ext;
        createDirectories(user_home + "/.local/share/icons");
        fs::copy_file(icon_src, icon_dst, fs::copy_options::overwrite_existing);
        std::cout << "  图标已安装: " << icon_dst << std::endl;
    }
}

void setupDesktopIntegration(
    const std::string& install_path,
    const std::string& app_name,
    const std::vector<FileEntry>& files,
    bool system_wide) {
    
    if (system_wide) {
        installSystemWide(install_path, app_name, files);
    } else {
        installUserLocal(install_path, app_name, files);
    }
}

void cleanupDesktopIntegration(const std::string& app_name) {
    std::string user_home = getUserHome();
    
    // 删除桌面文件
    std::string desktop_file = user_home + "/.local/share/applications/" + app_name + ".desktop";
    if (fs::exists(desktop_file)) {
        fs::remove(desktop_file);
        std::cout << "  已删除: " << desktop_file << std::endl;
    }
    
    std::string desktop_shortcut = user_home + "/Desktop/" + app_name + ".desktop";
    if (fs::exists(desktop_shortcut)) {
        fs::remove(desktop_shortcut);
        std::cout << "  已删除: " << desktop_shortcut << std::endl;
    }
    
    // 删除图标（支持多种扩展名）
    std::vector<std::string> icon_exts = {".png", ".svg", ".xpm"};
    for (const auto& ext : icon_exts) {
        std::string icon_file = user_home + "/.local/share/icons/" + app_name + ext;
        if (fs::exists(icon_file)) {
            fs::remove(icon_file);
            std::cout << "  已删除: " << icon_file << std::endl;
        }
    }
    
    // 更新桌面数据库
    system("update-desktop-database ~/.local/share/applications/ 2>/dev/null");
}
