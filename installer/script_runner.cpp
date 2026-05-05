#include "script_runner.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <array>

namespace fs = std::filesystem;

// 通用脚本执行函数
static ScriptResult runScript(const std::string& script_path,
                               const std::string& phase,
                               const std::string& install_path,
                               const std::string& app_name,
                               const std::string& version) {
    ScriptResult result;
    result.executed = false;
    result.success = true;
    result.exit_code = 0;
    
    if (!fs::exists(script_path)) {
        return result; // 脚本不存在，静默跳过
    }
    
    result.executed = true;
    
    // 确保脚本有执行权限
    auto cur_perms = fs::status(script_path).permissions();
    fs::permissions(script_path,
        cur_perms | fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec);
    
    std::cout << "  执行" << phase << "脚本: " << script_path << std::endl;
    
    // 构建命令，传入环境变量
    std::string cmd = "export APPPACK_INSTALL_DIR=\"" + install_path + "\" "
                      "APPPACK_APP_NAME=\"" + app_name + "\" "
                      "APPPACK_VERSION=\"" + version + "\"; "
                      "bash \"" + script_path + "\" 2>&1";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "  错误: 无法执行脚本" << std::endl;
        result.success = false;
        result.exit_code = -1;
        return result;
    }
    
    // 读取输出
    char buffer[4096];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
        // 实时输出，带缩进
        std::cout << "    | " << buffer;
    }
    
    result.exit_code = pclose(pipe);
    result.output = output;
    
    if (result.exit_code == 0) {
        std::cout << "  ✓ " << phase << "脚本执行成功" << std::endl;
        result.success = true;
    } else {
        std::cerr << "  ✗ " << phase << "脚本执行失败 (退出码: " 
                  << result.exit_code << ")" << std::endl;
        result.success = false;
    }
    
    return result;
}

ScriptResult runPreInstall(const std::string& install_path,
                            const std::string& app_name,
                            const std::string& version) {
    // 安装前脚本可能在 AppDir 中（打包时），也可能在已安装目录中（升级时）
    // 先检查安装目录，再检查当前目录
    std::string script_path = install_path + "/" + SCRIPT_PRE_INSTALL;
    if (!fs::exists(script_path)) {
        // 如果安装目录还不存在，检查当前工作目录
        script_path = std::string(SCRIPT_PRE_INSTALL);
    }
    return runScript(script_path, "安装前", install_path, app_name, version);
}

ScriptResult runPostInstall(const std::string& install_path,
                             const std::string& app_name,
                             const std::string& version) {
    std::string script_path = install_path + "/" + SCRIPT_POST_INSTALL;
    return runScript(script_path, "安装后", install_path, app_name, version);
}

ScriptResult runPostUninstall(const std::string& install_path,
                               const std::string& app_name,
                               const std::string& version) {
    std::string script_path = install_path + "/" + SCRIPT_POST_UNINSTALL;
    return runScript(script_path, "卸载后", install_path, app_name, version);
}

ScriptResult runPreUpgrade(const std::string& install_path,
                            const std::string& app_name,
                            const std::string& version) {
    // 升级前脚本在已安装的旧版本目录中查找
    std::string script_path = install_path + "/" + SCRIPT_PRE_UPGRADE;
    return runScript(script_path, "升级前", install_path, app_name, version);
}
