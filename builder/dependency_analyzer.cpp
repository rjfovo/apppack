#include "dependency_analyzer.h"
#include "file_utils.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <set>
#include <map>
#include <regex>
#include <unistd.h>

namespace fs = std::filesystem;

// 检查文件是否为 ELF 可执行文件或动态库
static bool isELF(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    char magic[4];
    f.read(magic, 4);
    return (magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F');
}

// 使用 ldd 分析 ELF 文件的动态库依赖
static std::vector<std::string> analyzeELFDeps(const fs::path& elf_path) {
    std::vector<std::string> deps;
    
    std::string cmd = "ldd \"" + elf_path.string() + "\" 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return deps;
    
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        
        // 跳过空行和头部
        if (line.find("linux-vdso") != std::string::npos ||
            line.find("ld-linux") != std::string::npos) {
            continue;
        }
        
        // 解析 "libxxx.so => /path/to/lib (0x...)" 格式
        // 或 "libxxx.so => not found"
        size_t arrow_pos = line.find("=>");
        if (arrow_pos != std::string::npos) {
            std::string soname = line.substr(0, arrow_pos);
            // 去除首尾空格
            soname.erase(0, soname.find_first_not_of(" \t"));
            soname.erase(soname.find_last_not_of(" \t") + 1);
            
            std::string rest = line.substr(arrow_pos + 2);
            size_t open_paren = rest.find('(');
            std::string lib_path = rest.substr(0, open_paren);
            lib_path.erase(0, lib_path.find_first_not_of(" \t"));
            lib_path.erase(lib_path.find_last_not_of(" \t") + 1);
            
            if (lib_path == "not found") {
                std::cerr << "  警告: 未找到依赖库: " << soname << std::endl;
            } else if (!lib_path.empty()) {
                deps.push_back(lib_path);
            }
        }
    }
    
    pclose(pipe);
    return deps;
}

// 递归收集所有依赖（避免重复）
static void collectDepsRecursive(const fs::path& file_path, 
                                  std::set<std::string>& collected,
                                  std::vector<FileEntry>& result,
                                  int depth = 0) {
    if (depth > 10) return; // 防止循环依赖
    
    auto deps = analyzeELFDeps(file_path);
    for (const auto& dep_path : deps) {
        if (collected.find(dep_path) != collected.end()) continue;
        collected.insert(dep_path);
        
        fs::path dp(dep_path);
        if (fs::exists(dp)) {
            std::cout << "  收集依赖: " << dp.filename().string() << std::endl;
            
            // 读取库文件
            FileEntry fe;
            fe.relative_path = "usr/lib/" + dp.filename().string();
            fe.content = readFile(dp);
            
            struct stat st;
            stat(dp.c_str(), &st);
            fe.permissions = st.st_mode & 0777;
            
            result.push_back(std::move(fe));
            
            // 递归分析这个库的依赖
            collectDepsRecursive(dp, collected, result, depth + 1);
        }
    }
}

std::vector<FileEntry> collectDependencies(const fs::path& appdir) {
    std::vector<FileEntry> deps;
    std::set<std::string> collected;
    
    std::cout << "正在分析动态库依赖..." << std::endl;
    
    // 遍历 AppDir 中的所有 ELF 文件
    for (const auto& entry : fs::recursive_directory_iterator(appdir)) {
        if (entry.is_regular_file() && isELF(entry.path())) {
            std::cout << "  分析: " << fs::relative(entry.path(), appdir).string() << std::endl;
            collectDepsRecursive(entry.path(), collected, deps);
        }
    }
    
    if (deps.empty()) {
        std::cout << "  未发现外部动态库依赖" << std::endl;
    } else {
        std::cout << "  共收集 " << deps.size() << " 个依赖库" << std::endl;
    }
    
    return deps;
}

bool checkLibraryInstalled(const std::string& soname) {
    std::string cmd = "ldconfig -p 2>/dev/null | grep -q \"" + soname + "\"";
    return (system(cmd.c_str()) == 0);
}

DependencyInfo resolveLibrary(const std::string& soname) {
    DependencyInfo info;
    info.soname = soname;
    
    std::string cmd = "ldconfig -p 2>/dev/null | grep \"" + soname + "\" | head -1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buffer[1024];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            // 格式: "libssl.so.3 (libc6,x86-64) => /lib/x86_64-linux-gnu/libssl.so.3"
            size_t arrow_pos = line.find("=>");
            if (arrow_pos != std::string::npos) {
                info.real_path = line.substr(arrow_pos + 2);
                info.real_path.erase(0, info.real_path.find_first_not_of(" \t"));
                info.real_path.erase(info.real_path.find_last_not_of(" \t\r\n") + 1);
            }
        }
        pclose(pipe);
    }
    
    // 尝试获取所属包名
    if (!info.real_path.empty()) {
        std::string pkg_cmd = "dpkg -S \"" + info.real_path + "\" 2>/dev/null | cut -d: -f1";
        FILE* pkg_pipe = popen(pkg_cmd.c_str(), "r");
        if (pkg_pipe) {
            char pkg_buf[256];
            if (fgets(pkg_buf, sizeof(pkg_buf), pkg_pipe)) {
                info.package = std::string(pkg_buf);
                info.package.erase(info.package.find_last_not_of(" \t\r\n") + 1);
            }
            pclose(pkg_pipe);
        }
    }
    
    return info;
}
