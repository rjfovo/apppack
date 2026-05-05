/**
 * test_common.cpp - 通用工具模块单元测试
 * 
 * 测试内容:
 *   - common.h: PackageHeader, FileEntry, isAppPackScript
 *   - file_utils: 文件读写、序列化/反序列化、校验和
 *   - compressor: zstd 压缩/解压缩
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include "common.h"
#include "file_utils.h"
#include "compressor.h"

namespace fs = std::filesystem;

// ============================================================
// 测试夹具：创建临时目录
// ============================================================
class TempDirTest : public ::testing::Test {
protected:
    fs::path temp_dir;
    
    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "apppack_test_XXXXXX";
        // 创建唯一临时目录
        std::string tmpl = temp_dir.string();
        if (mkdtemp(tmpl.data())) {
            temp_dir = tmpl;
        } else {
            temp_dir = fs::temp_directory_path() / ("apppack_test_" + std::to_string(time(nullptr)));
            fs::create_directories(temp_dir);
        }
    }
    
    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }
};

// ============================================================
// common.h 测试
// ============================================================

// 测试 PackageHeader 初始化
TEST(CommonTest, PackageHeaderInit) {
    PackageHeader header;
    
    // 验证魔数
    EXPECT_EQ(std::memcmp(header.magic, "APPPACK", 7), 0);
    EXPECT_EQ(header.magic[7], '\0');
    
    // 验证默认值
    EXPECT_EQ(header.data_offset, 0);
    EXPECT_EQ(header.data_size, 0);
    EXPECT_EQ(header.orig_size, 0);
    EXPECT_EQ(header.checksum, 0);
    
    // 验证默认版本号
    EXPECT_STREQ(header.version, "1.0.0");
    
    // 验证应用名称为空
    EXPECT_EQ(header.app_name[0], '\0');
    EXPECT_EQ(header.install_dir[0], '\0');
}

// 测试 PackageHeader 大小
TEST(CommonTest, PackageHeaderSize) {
    PackageHeader header;
    // 验证结构体大小: 8 + 8 + 8 + 8 + 256 + 256 + 64 + 4 = 612
    EXPECT_EQ(sizeof(PackageHeader), 612);
}

// 测试 FileEntry 结构
TEST(CommonTest, FileEntryStructure) {
    FileEntry entry;
    entry.relative_path = "usr/bin/myapp";
    entry.content = {'H', 'e', 'l', 'l', 'o'};
    entry.permissions = 0755;
    
    EXPECT_EQ(entry.relative_path, "usr/bin/myapp");
    EXPECT_EQ(entry.content.size(), 5);
    EXPECT_EQ(entry.content[0], 'H');
    EXPECT_EQ(entry.permissions, 0755);
}

// 测试 isAppPackScript
TEST(CommonTest, IsAppPackScript) {
    EXPECT_TRUE(isAppPackScript("apppack-pre-install.sh"));
    EXPECT_TRUE(isAppPackScript("apppack-post-install.sh"));
    EXPECT_TRUE(isAppPackScript("apppack-post-uninstall.sh"));
    EXPECT_TRUE(isAppPackScript("apppack-pre-upgrade.sh"));
    
    EXPECT_FALSE(isAppPackScript(""));
    EXPECT_FALSE(isAppPackScript("myapp.sh"));
    EXPECT_FALSE(isAppPackScript("apppack-pre-install"));
    EXPECT_FALSE(isAppPackScript("AppRun"));
    EXPECT_FALSE(isAppPackScript("myapp.desktop"));
}

// ============================================================
// file_utils 测试
// ============================================================

// 测试文件读写
TEST_F(TempDirTest, FileReadWrite) {
    fs::path test_file = temp_dir / "test.txt";
    
    // 写入文件
    std::vector<char> write_data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    writeFile(test_file, write_data);
    
    // 验证文件存在
    EXPECT_TRUE(fs::exists(test_file));
    
    // 读取文件
    auto read_data = readFile(test_file);
    
    // 验证内容一致
    EXPECT_EQ(read_data.size(), write_data.size());
    EXPECT_EQ(read_data, write_data);
}

// 测试空文件读写
TEST_F(TempDirTest, EmptyFileReadWrite) {
    fs::path test_file = temp_dir / "empty.txt";
    
    std::vector<char> empty_data;
    writeFile(test_file, empty_data);
    
    EXPECT_TRUE(fs::exists(test_file));
    EXPECT_EQ(fs::file_size(test_file), 0);
    
    auto read_data = readFile(test_file);
    EXPECT_TRUE(read_data.empty());
}

// 测试二进制文件读写
TEST_F(TempDirTest, BinaryFileReadWrite) {
    fs::path test_file = temp_dir / "binary.bin";
    
    // 创建二进制数据（包含 null 字节）
    std::vector<char> binary_data;
    binary_data.resize(256);
    for (int i = 0; i < 256; i++) {
        binary_data[i] = static_cast<char>(i);
    }
    
    writeFile(test_file, binary_data);
    auto read_data = readFile(test_file);
    
    EXPECT_EQ(read_data.size(), 256);
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(read_data[i], static_cast<char>(i));
    }
}

// 测试 collectFiles
TEST_F(TempDirTest, CollectFiles) {
    // 创建测试目录结构
    fs::create_directories(temp_dir / "usr" / "bin");
    fs::create_directories(temp_dir / "usr" / "lib");
    fs::create_directories(temp_dir / "usr" / "share" / "icons");
    
    // 创建测试文件
    writeFile(temp_dir / "AppRun", std::vector<char>{'#', '!', '/', 'b', 'i', 'n', '/', 'b', 'a', 's', 'h'});
    writeFile(temp_dir / "myapp.desktop", std::vector<char>{'[', 'D', 'e', 's', 'k', 't', 'o', 'p', ' ', 'E', 'n', 't', 'r', 'y', ']'});
    writeFile(temp_dir / "usr" / "bin" / "myapp", std::vector<char>{'e', 'l', 'f', ' ', 'd', 'a', 't', 'a'});
    writeFile(temp_dir / "usr" / "lib" / "libfoo.so", std::vector<char>{'l', 'i', 'b', ' ', 'd', 'a', 't', 'a'});
    writeFile(temp_dir / "usr" / "share" / "icons" / "myapp.svg", std::vector<char>{'<', 's', 'v', 'g', '>'});
    
    // 收集文件
    auto files = collectFiles(temp_dir);
    
    // 验证收集到的文件
    // 注意：collectFiles 可能包含 .apppack-version 等额外文件
    // 我们只验证关键文件是否在列表中
    std::set<std::string> paths;
    for (const auto& f : files) {
        paths.insert(f.relative_path);
    }
    
    EXPECT_TRUE(paths.count("AppRun") > 0);
    EXPECT_TRUE(paths.count("myapp.desktop") > 0);
    EXPECT_TRUE(paths.count("usr/bin/myapp") > 0);
    EXPECT_TRUE(paths.count("usr/lib/libfoo.so") > 0);
    EXPECT_TRUE(paths.count("usr/share/icons/myapp.svg") > 0);
}

// 测试序列化/反序列化
TEST_F(TempDirTest, SerializeDeserialize) {
    // 创建文件列表
    std::vector<FileEntry> original;
    
    FileEntry f1;
    f1.relative_path = "usr/bin/myapp";
    f1.content = {'b', 'i', 'n', 'a', 'r', 'y'};
    f1.permissions = 0755;
    original.push_back(f1);
    
    FileEntry f2;
    f2.relative_path = "usr/share/icons/myapp.svg";
    f2.content = {'<', 's', 'v', 'g', '>'};
    f2.permissions = 0644;
    original.push_back(f2);
    
    FileEntry f3;
    f3.relative_path = "AppRun";
    f3.content = {'#', '!', '/', 'b', 'i', 'n', '/', 'b', 'a', 's', 'h'};
    f3.permissions = 0755;
    original.push_back(f3);
    
    // 序列化
    auto serialized = serializeFiles(original);
    EXPECT_GT(serialized.size(), 0);
    
    // 反序列化
    auto deserialized = deserializeFiles(serialized);
    
    // 验证数量一致
    ASSERT_EQ(deserialized.size(), original.size());
    
    // 验证每个文件
    for (size_t i = 0; i < original.size(); i++) {
        EXPECT_EQ(deserialized[i].relative_path, original[i].relative_path);
        EXPECT_EQ(deserialized[i].content, original[i].content);
        EXPECT_EQ(deserialized[i].permissions, original[i].permissions);
    }
}

// 测试空文件列表序列化
TEST_F(TempDirTest, EmptySerializeDeserialize) {
    std::vector<FileEntry> empty;
    auto serialized = serializeFiles(empty);
    auto deserialized = deserializeFiles(serialized);
    EXPECT_TRUE(deserialized.empty());
}

// 测试校验和
TEST_F(TempDirTest, Checksum) {
    std::vector<char> data1 = {'H', 'e', 'l', 'l', 'o'};
    std::vector<char> data2 = {'H', 'e', 'l', 'l', 'o'};
    std::vector<char> data3 = {'W', 'o', 'r', 'l', 'd'};
    
    // 相同数据应产生相同校验和
    EXPECT_EQ(calculateChecksum(data1), calculateChecksum(data2));
    
    // 不同数据应产生不同校验和（理论上）
    // 注意：简单校验和可能有冲突，但 Hello 和 World 应该不同
    EXPECT_NE(calculateChecksum(data1), calculateChecksum(data3));
    
    // 空数据校验和
    std::vector<char> empty;
    EXPECT_EQ(calculateChecksum(empty), 0);
}

// ============================================================
// compressor 测试
// ============================================================

// 测试压缩和解压缩
TEST(CompressorTest, CompressDecompress) {
    // 创建测试数据
    std::vector<char> original;
    original.resize(10000);
    for (size_t i = 0; i < original.size(); i++) {
        original[i] = static_cast<char>(i % 256);
    }
    
    // 压缩
    auto compressed = compressData(original);
    EXPECT_GT(compressed.size(), 0);
    EXPECT_LE(compressed.size(), original.size());  // 压缩后应该更小或相等
    
    // 解压缩
    auto decompressed = decompressData(compressed, original.size());
    
    // 验证解压后数据一致
    ASSERT_EQ(decompressed.size(), original.size());
    EXPECT_EQ(decompressed, original);
}

// 测试空数据压缩
TEST(CompressorTest, CompressEmpty) {
    std::vector<char> empty;
    auto compressed = compressData(empty);
    auto decompressed = decompressData(compressed, 0);
    EXPECT_TRUE(decompressed.empty());
}

// 测试小数据压缩
TEST(CompressorTest, CompressSmallData) {
    std::vector<char> small = {'H', 'e', 'l', 'l', 'o'};
    auto compressed = compressData(small);
    auto decompressed = decompressData(compressed, small.size());
    EXPECT_EQ(decompressed, small);
}

// 测试可重复压缩/解压缩
TEST(CompressorTest, MultipleCompressDecompress) {
    std::vector<char> data = {'T', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    
    for (int i = 0; i < 5; i++) {
        auto compressed = compressData(data);
        data = decompressData(compressed, data.size());
    }
    
    std::vector<char> expected = {'T', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    EXPECT_EQ(data, expected);
}

// 测试大文件压缩
TEST(CompressorTest, CompressLargeData) {
    // 创建 1MB 测试数据
    std::vector<char> original;
    original.resize(1024 * 1024);
    for (size_t i = 0; i < original.size(); i++) {
        original[i] = static_cast<char>('A' + (i % 26));  // 可压缩数据
    }
    
    auto compressed = compressData(original);
    auto decompressed = decompressData(compressed, original.size());
    
    EXPECT_EQ(decompressed, original);
    
    // 验证压缩率（可压缩数据应该压缩得很好）
    double ratio = static_cast<double>(compressed.size()) / original.size();
    EXPECT_LT(ratio, 0.5);  // 压缩率应小于 50%
}

// ============================================================
// 主函数
// ============================================================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
