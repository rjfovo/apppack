#include "compressor.h"
#include "file_utils.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <stdexcept>

std::vector<char> compressData(const std::vector<char>& input) {
    // 创建临时文件
    char tmp_in_template[] = "/tmp/apppack_compress_input_XXXXXX";
    char tmp_out_template[] = "/tmp/apppack_compress_output_XXXXXX";
    
    int fd_in = mkstemp(tmp_in_template);
    if (fd_in == -1) throw std::runtime_error("无法创建临时文件");
    write(fd_in, input.data(), input.size());
    close(fd_in);
    
    int fd_out = mkstemp(tmp_out_template);
    if (fd_out == -1) {
        unlink(tmp_in_template);
        throw std::runtime_error("无法创建临时文件");
    }
    close(fd_out);
    
    std::string tmp_in(tmp_in_template);
    std::string tmp_out(tmp_out_template);
    
    // 调用 zstd 压缩
    std::string cmd = "zstd -q -f -o " + tmp_out + " " + tmp_in;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        unlink(tmp_in.c_str());
        unlink(tmp_out.c_str());
        throw std::runtime_error("zstd 压缩失败");
    }
    
    // 读取压缩后的数据
    std::vector<char> compressed = readFile(tmp_out);
    
    // 清理临时文件
    unlink(tmp_in.c_str());
    unlink(tmp_out.c_str());
    
    return compressed;
}

std::vector<char> decompressData(const std::vector<char>& input, uint64_t orig_size) {
    // 创建临时文件
    char tmp_in_template[] = "/tmp/apppack_decompress_input_XXXXXX";
    char tmp_out_template[] = "/tmp/apppack_decompress_output_XXXXXX";
    
    int fd_in = mkstemp(tmp_in_template);
    if (fd_in == -1) throw std::runtime_error("无法创建临时文件");
    write(fd_in, input.data(), input.size());
    close(fd_in);
    
    int fd_out = mkstemp(tmp_out_template);
    if (fd_out == -1) {
        unlink(tmp_in_template);
        throw std::runtime_error("无法创建临时文件");
    }
    close(fd_out);
    
    std::string tmp_in(tmp_in_template);
    std::string tmp_out(tmp_out_template);
    
    // 调用 zstd 解压
    std::string cmd = "zstd -q -d -f -o " + tmp_out + " " + tmp_in;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        unlink(tmp_in.c_str());
        unlink(tmp_out.c_str());
        throw std::runtime_error("zstd 解压失败");
    }
    
    // 读取解压后的数据
    std::vector<char> decompressed = readFile(tmp_out);
    
    // 清理临时文件
    unlink(tmp_in.c_str());
    unlink(tmp_out.c_str());
    
    return decompressed;
}
