#ifndef APPPACK_COMMON_H
#define APPPACK_COMMON_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>

// 包头部结构（使用 1 字节对齐确保跨平台兼容）
#pragma pack(push, 1)
struct PackageHeader {
    char magic[8];          // "APPPACK\0"
    uint64_t data_offset;   // 压缩数据在文件中的偏移
    uint64_t data_size;     // 压缩数据大小
    uint64_t orig_size;     // 原始数据大小
    char app_name[256];     // 应用名称
    char install_dir[256];  // 默认安装目录
    char version[64];       // 应用版本号
    uint32_t checksum;      // 简单校验和

    PackageHeader() {
        std::memset(this, 0, sizeof(PackageHeader));
        std::memcpy(magic, "APPPACK", 7);
        std::strncpy(version, "1.0.0", sizeof(version) - 1);
    }
};
#pragma pack(pop)

// 文件条目
struct FileEntry {
    std::string relative_path;
    std::vector<char> content;
    mode_t permissions;
};

// 依赖库信息
struct DependencyInfo {
    std::string soname;      // 库的 SONAME (如 libssl.so.3)
    std::string real_path;   // 库的实际路径
    std::string package;     // 所属包名 (如 libssl3)
};

// 约定脚本文件名
constexpr const char* SCRIPT_PRE_INSTALL    = "apppack-pre-install.sh";
constexpr const char* SCRIPT_POST_INSTALL   = "apppack-post-install.sh";
constexpr const char* SCRIPT_POST_UNINSTALL = "apppack-post-uninstall.sh";
constexpr const char* SCRIPT_PRE_UPGRADE    = "apppack-pre-upgrade.sh";

// 检查文件名是否为约定脚本
inline bool isAppPackScript(const std::string& filename) {
    return filename == SCRIPT_PRE_INSTALL ||
           filename == SCRIPT_POST_INSTALL ||
           filename == SCRIPT_POST_UNINSTALL ||
           filename == SCRIPT_PRE_UPGRADE;
}

#endif // APPPACK_COMMON_H
