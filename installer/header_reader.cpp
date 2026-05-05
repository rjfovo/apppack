#include "header_reader.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <vector>

namespace fs = std::filesystem;

PackageHeader readHeader(const std::string& self_path) {
    uint64_t file_size = fs::file_size(self_path);
    
    // 文件必须至少包含一个头部大小
    if (file_size < sizeof(PackageHeader)) {
        throw std::runtime_error("无效的安装包：文件太小");
    }
    
    // 头部在安装器运行时之后、压缩数据之前，位于文件开头附近
    // 安装器运行时通常只有 100-200KB，所以头部通常在文件前 1MB 范围内
    // 但为了安全，搜索整个文件的前半部分（头部不可能在文件后半部分）
    uint64_t search_size = file_size / 2;
    if (search_size > 50 * 1024 * 1024) {
        search_size = 50 * 1024 * 1024;  // 最多搜索 50MB
    }
    
    std::ifstream file(self_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法读取自身文件");
    }
    
    std::vector<char> head_data(static_cast<size_t>(search_size));
    file.read(head_data.data(), head_data.size());
    file.close();
    
    // 从前向后搜索 "APPPACK" magic
    const char* magic = "APPPACK";
    int found = -1;
    
    if (head_data.size() >= sizeof(PackageHeader)) {
        for (size_t i = 0; i <= head_data.size() - sizeof(PackageHeader); i++) {
            if (std::memcmp(head_data.data() + i, magic, 7) == 0) {
                PackageHeader header;
                std::memcpy(&header, head_data.data() + i, sizeof(header));
                
                // 验证 magic 和合理的 data_size
                if (std::memcmp(header.magic, magic, 7) == 0 &&
                    header.data_size > 0 && 
                    header.data_size < file_size &&
                    header.orig_size > 0) {
                    found = i;
                    break;  // 取第一个匹配的头部
                }
            }
        }
    }
    
    if (found >= 0) {
        PackageHeader header;
        std::memcpy(&header, head_data.data() + found, sizeof(header));
        // 计算头部在文件中的偏移
        header.data_offset = found + sizeof(PackageHeader);
        return header;
    }
    
    throw std::runtime_error("无效的安装包：找不到包头部");
}
