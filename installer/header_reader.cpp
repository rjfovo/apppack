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
    
    // 头部在安装器运行时之后、压缩数据之前
    // 安装器运行时通常只有 100-200KB
    // 搜索策略：
    // 1. 首先在文件开头附近搜索（头部在安装器运行时之后，通常在 0~2MB 范围内）
    // 2. 如果没找到，再在文件末尾附近搜索（兼容旧格式）
    
    const char* magic = "APPPACK";
    int found = -1;
    uint64_t found_offset = 0;
    
    std::ifstream file(self_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法读取自身文件");
    }
    
    // 策略1：在文件开头附近搜索（头部在安装器运行时之后，通常在 0~2MB 范围内）
    // 安装器运行时最大可能 2MB，头部在运行时之后
    uint64_t front_search_size = std::min<uint64_t>(file_size, 2 * 1024 * 1024);  // 搜索前 2MB
    std::vector<char> front_data(static_cast<size_t>(front_search_size));
    file.seekg(0);
    file.read(front_data.data(), front_data.size());
    
    if (front_data.size() >= sizeof(PackageHeader)) {
        for (size_t i = 0; i <= front_data.size() - sizeof(PackageHeader); i++) {
            if (std::memcmp(front_data.data() + i, magic, 7) == 0) {
                PackageHeader header;
                std::memcpy(&header, front_data.data() + i, sizeof(header));
                
                // 验证 magic 和合理的 data_size
                if (std::memcmp(header.magic, magic, 7) == 0 &&
                    header.data_size > 0 && 
                    header.data_size < file_size &&
                    header.orig_size > 0) {
                    found = i;
                    found_offset = 0;
                    break;
                }
            }
        }
    }
    
    // 策略2：如果文件开头没找到，在文件末尾附近搜索（兼容旧格式）
    if (found < 0 && file_size > 1024 * 1024) {
        uint64_t tail_search_size = std::min<uint64_t>(file_size, 2 * 1024 * 1024);  // 搜索末尾 2MB
        uint64_t search_start = file_size - tail_search_size;
        std::vector<char> tail_data(static_cast<size_t>(tail_search_size));
        file.seekg(search_start);
        file.read(tail_data.data(), tail_data.size());
        
        if (tail_data.size() >= sizeof(PackageHeader)) {
            // 从后向前搜索，找到最后一个匹配的头部
            for (size_t i = tail_data.size() - sizeof(PackageHeader); i > 0; i--) {
                if (std::memcmp(tail_data.data() + i, magic, 7) == 0) {
                    PackageHeader header;
                    std::memcpy(&header, tail_data.data() + i, sizeof(header));
                    
                    if (std::memcmp(header.magic, magic, 7) == 0 &&
                        header.data_size > 0 && 
                        header.data_size < file_size &&
                        header.orig_size > 0) {
                        found = i;
                        found_offset = search_start;
                        break;
                    }
                }
            }
        }
    }
    
    file.close();
    
    if (found >= 0) {
        PackageHeader header;
        // 重新读取头部数据（found_offset + found 是头部在文件中的绝对偏移）
        std::ifstream re_read(self_path, std::ios::binary);
        re_read.seekg(found_offset + found);
        re_read.read(reinterpret_cast<char*>(&header), sizeof(header));
        re_read.close();
        
        // 计算头部在文件中的偏移
        header.data_offset = found_offset + found + sizeof(PackageHeader);
        return header;
    }

    
    throw std::runtime_error("无效的安装包：找不到包头部");
}

