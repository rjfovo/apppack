#include "file_utils.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <unistd.h>

namespace fs = std::filesystem;

std::vector<char> readFileRange(const std::string& path, uint64_t offset, uint64_t size) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法读取文件: " + path);
    }
    file.seekg(offset);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    return buffer;
}

void createDirectories(const std::string& path) {
    fs::create_directories(path);
}

void setPermissions(const std::string& path, mode_t permissions) {
    chmod(path.c_str(), permissions);
}

std::string getSelfPath() {
    char result[4096];
    ssize_t count = readlink("/proc/self/exe", result, sizeof(result) - 1);
    if (count != -1) {
        result[count] = '\0';
        return std::string(result);
    }
    return "";
}
