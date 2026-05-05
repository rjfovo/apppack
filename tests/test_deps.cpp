/**
 * test_deps.cpp - 依赖分析模块单元测试
 * 
 * 测试内容:
 *   - dependency_analyzer: ldd 依赖分析、库解析
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "common.h"
#include "file_utils.h"
#include "compressor.h"
#include "dependency_analyzer.h"

namespace fs = std::filesystem;

// ============================================================
// 测试夹具
// ============================================================
class DepsTest : public ::testing::Test {
protected:
    fs::path temp_dir;
    fs::path appdir;
    
    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "apppack_test_deps_XXXXXX";
        std::string tmpl = temp_dir.string();
        if (mkdtemp(tmpl.data())) {
            temp_dir = tmpl;
        } else {
            temp_dir = fs::temp_directory_path() / ("apppack_test_deps_" + std::to_string(time(nullptr)));
            fs::create_directories(temp_dir);
        }
        
        appdir = temp_dir / "MyApp.AppDir";
        fs::create_directories(appdir / "usr" / "bin");
        fs::create_directories(appdir / "usr" / "lib");
    }
    
    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }
    
    // 创建一个简单的 ELF 可执行文件（使用 shell 脚本模拟）
    void createMockExecutable() {
        std::ofstream(appdir / "usr" / "bin" / "myapp") << "#!/bin/bash\necho \"Hello\"\n";
        fs::permissions(appdir / "usr" / "bin" / "myapp", fs::perms::owner_exec | fs::perms::owner_read);
    }
    
    // 创建一个真正的 ELF 文件（使用 /bin/echo 复制）
    void createRealElfExecutable() {
        if (fs::exists("/bin/echo")) {
            fs::copy("/bin/echo", appdir / "usr" / "bin" / "myapp");
            fs::permissions(appdir / "usr" / "bin" / "myapp", fs::perms::owner_exec | fs::perms::owner_read);
        }
    }
};

// ============================================================
// dependency_analyzer 测试
// ============================================================

// 测试 checkLibraryInstalled
TEST_F(DepsTest, CheckLibraryInstalled) {
    // libc 应该总是存在的
    bool found = checkLibraryInstalled("libc.so.6");
    EXPECT_TRUE(found);
    
    // libstdc++ 应该存在
    found = checkLibraryInstalled("libstdc++.so.6");
    EXPECT_TRUE(found);
    
    // 不存在的库
    found = checkLibraryInstalled("libnonexistent.so.99");
    EXPECT_FALSE(found);
}

// 测试 resolveLibrary
TEST_F(DepsTest, ResolveLibrary) {
    // 解析 libc
    DependencyInfo info = resolveLibrary("libc.so.6");
    EXPECT_EQ(info.soname, "libc.so.6");
    EXPECT_FALSE(info.real_path.empty());
    EXPECT_TRUE(fs::exists(info.real_path));
    
    // 解析 libstdc++
    info = resolveLibrary("libstdc++.so.6");
    EXPECT_EQ(info.soname, "libstdc++.so.6");
    EXPECT_FALSE(info.real_path.empty());
    EXPECT_TRUE(fs::exists(info.real_path));
    
    // 解析不存在的库
    info = resolveLibrary("libnonexistent.so.99");
    EXPECT_TRUE(info.real_path.empty());
}

// 测试 collectDependencies - 脚本文件
TEST_F(DepsTest, CollectDepsScriptFile) {
    createMockExecutable();
    
    // 脚本文件不应该有依赖
    auto deps = collectDependencies(appdir);
    // 脚本文件不会被 ldd 分析，所以应该没有依赖
    EXPECT_TRUE(deps.empty());
}

// 测试 collectDependencies - ELF 文件
TEST_F(DepsTest, CollectDepsElfFile) {
    createRealElfExecutable();
    
    if (!fs::exists(appdir / "usr" / "bin" / "myapp")) {
        GTEST_SKIP() << "无法复制 ELF 文件，跳过测试";
    }
    
    auto deps = collectDependencies(appdir);
    
    // 即使是最简单的 ELF 文件也可能有依赖
    // 我们只验证函数不会崩溃
    SUCCEED();
}

// 测试 collectDependencies - 空目录
TEST_F(DepsTest, CollectDepsEmptyDir) {
    auto deps = collectDependencies(appdir);
    EXPECT_TRUE(deps.empty());
}

// 测试 collectDependencies - 包含 .so 文件
TEST_F(DepsTest, CollectDepsWithLibs) {
    createRealElfExecutable();
    
    if (!fs::exists(appdir / "usr" / "bin" / "myapp")) {
        GTEST_SKIP() << "无法复制 ELF 文件，跳过测试";
    }
    
    // 复制一些系统库到 usr/lib
    if (fs::exists("/usr/lib/x86_64-linux-gnu/libzstd.so.1")) {
        fs::copy("/usr/lib/x86_64-linux-gnu/libzstd.so.1",
                 appdir / "usr" / "lib" / "libzstd.so.1");
    }
    
    auto deps = collectDependencies(appdir);
    
    // 验证函数不会崩溃
    SUCCEED();
}

// 测试 collectDependencies - 多层目录
TEST_F(DepsTest, CollectDepsNestedDirs) {
    createRealElfExecutable();
    
    if (!fs::exists(appdir / "usr" / "bin" / "myapp")) {
        GTEST_SKIP() << "无法复制 ELF 文件，跳过测试";
    }
    
    // 创建嵌套目录结构
    fs::create_directories(appdir / "usr" / "lib" / "subdir");
    fs::create_directories(appdir / "usr" / "lib" / "qt5" / "plugins");
    
    auto deps = collectDependencies(appdir);
    
    // 验证函数不会崩溃
    SUCCEED();
}

// ============================================================
// 主函数
// ============================================================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
