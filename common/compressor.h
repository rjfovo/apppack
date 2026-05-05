#ifndef APPPACK_COMMON_COMPRESSOR_H
#define APPPACK_COMMON_COMPRESSOR_H

#include <cstdint>
#include <vector>

// 使用 zstd 压缩数据
std::vector<char> compressData(const std::vector<char>& input);

// 使用 zstd 解压数据
std::vector<char> decompressData(const std::vector<char>& input, uint64_t orig_size);

#endif // APPPACK_COMMON_COMPRESSOR_H
