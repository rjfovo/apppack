#include "file_utils.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>

namespace fs = std::filesystem;

std::vector<char> readFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("无法读取文件: " + path.string());
    }
    size_t size = file.tellg();
    file.seekg(0);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    return buffer;
}

void writeFile(const fs::path& path, const std::vector<char>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法写入文件: " + path.string());
    }
    file.write(data.data(), data.size());
}

std::vector<FileEntry> collectFiles(const fs::path& appdir) {
    std::vector<FileEntry> files;
    
    for (const auto& entry : fs::recursive_directory_iterator(appdir)) {
        if (entry.is_regular_file()) {
            FileEntry fe;
            fe.relative_path = fs::relative(entry.path(), appdir).string();
            fe.content = readFile(entry.path());
            
            // 获取文件权限
            struct stat st;
            stat(entry.path().c_str(), &st);
            fe.permissions = st.st_mode & 0777;
            
            files.push_back(std::move(fe));
        }
    }
    
    return files;
}

std::vector<char> serializeFiles(const std::vector<FileEntry>& files) {
    std::vector<char> data;
    
    // 写入文件数量
    uint32_t file_count = files.size();
    data.insert(data.end(), 
        reinterpret_cast<const char*>(&file_count), 
        reinterpret_cast<const char*>(&file_count) + sizeof(file_count));
    
    for (const auto& file : files) {
        // 写入路径长度和路径
        uint32_t path_len = file.relative_path.size();
        data.insert(data.end(),
            reinterpret_cast<const char*>(&path_len),
            reinterpret_cast<const char*>(&path_len) + sizeof(path_len));
        data.insert(data.end(), file.relative_path.begin(), file.relative_path.end());
        
        // 写入权限
        data.insert(data.end(),
            reinterpret_cast<const char*>(&file.permissions),
            reinterpret_cast<const char*>(&file.permissions) + sizeof(file.permissions));
        
        // 写入内容大小和内容
        uint64_t content_size = file.content.size();
        data.insert(data.end(),
            reinterpret_cast<const char*>(&content_size),
            reinterpret_cast<const char*>(&content_size) + sizeof(content_size));
        data.insert(data.end(), file.content.begin(), file.content.end());
    }
    
    return data;
}

std::vector<FileEntry> deserializeFiles(const std::vector<char>& data) {
    std::vector<FileEntry> files;
    size_t offset = 0;
    
    // 读取文件数量
    uint32_t file_count;
    std::memcpy(&file_count, data.data() + offset, sizeof(file_count));
    offset += sizeof(file_count);
    
    for (uint32_t i = 0; i < file_count; i++) {
        FileEntry fe;
        
        // 读取路径
        uint32_t path_len;
        std::memcpy(&path_len, data.data() + offset, sizeof(path_len));
        offset += sizeof(path_len);
        fe.relative_path = std::string(data.data() + offset, path_len);
        offset += path_len;
        
        // 读取权限
        std::memcpy(&fe.permissions, data.data() + offset, sizeof(fe.permissions));
        offset += sizeof(fe.permissions);
        
        // 读取内容
        uint64_t content_size;
        std::memcpy(&content_size, data.data() + offset, sizeof(content_size));
        offset += sizeof(content_size);
        fe.content = std::vector<char>(data.data() + offset, data.data() + offset + content_size);
        offset += content_size;
        
        files.push_back(std::move(fe));
    }
    
    return files;
}

uint32_t calculateChecksum(const std::vector<char>& data) {
    uint32_t sum = 0;
    for (size_t i = 0; i < data.size(); i++) {
        sum += static_cast<unsigned char>(data[i]);
    }
    return sum;
}
