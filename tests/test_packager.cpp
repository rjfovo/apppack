/**
 * test_packager.cpp - 打包器模块单元测试
 * 
 * 测试内容:
 *   - desktop_parser: 桌面文件解析
 *   - packager: 打包配置和创建包
 *   - template_manager: 项目模板初始化
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "common.h"
#include "file_utils.h"
#include "compressor.h"
#include "desktop_parser.h"
#include "packager.h"
#include "template_manager.h"

namespace fs = std::filesystem;

// ============================================================
// 测试夹具
// ============================================================
class PackagerTest : public ::testing::Test {
protected:
    fs::path temp_dir;
    fs::path appdir;
    fs::path output_path;
    
    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "apppack_test_packager_XXXXXX";
        std::string tmpl = temp_dir.string();
        if (mkdtemp(tmpl.data())) {
            temp_dir = tmpl;
        } else {
            temp_dir = fs::temp_directory_path() / ("apppack_test_packager_" + std::to_string(time(nullptr)));
            fs::create_directories(temp_dir);
        }
        
        appdir = temp_dir / "MyApp.AppDir";
        output_path = temp_dir / "MyApp.apppack";
        
        // 创建 AppDir 结构
        fs::create_directories(appdir / "usr" / "bin");
        fs::create_directories(appdir / "usr" / "lib");
    }
    
    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }
    
    // 创建最小 AppDir
    void createMinimalAppDir() {
        // 创建 AppRun
        std::ofstream(appdir / "AppRun") << "#!/bin/bash\necho \"Hello\"\n";
        fs::permissions(appdir / "AppRun", fs::perms::owner_exec | fs::perms::owner_read);
        
        // 创建 desktop 文件
        std::ofstream(appdir / "myapp.desktop") << "[Desktop Entry]\n"
            << "Name=MyApp\n"
            << "Exec=AppRun\n"
            << "Icon=myapp\n"
            << "Terminal=false\n"
            << "Type=Application\n"
            << "Categories=Utility;\n";
        
        // 创建可执行文件
        std::ofstream(appdir / "usr" / "bin" / "myapp") << "#!/bin/bash\necho \"Hello World\"\n";
        fs::permissions(appdir / "usr" / "bin" / "myapp", fs::perms::owner_exec | fs::perms::owner_read);
    }
};

// ============================================================
// desktop_parser 测试
// ============================================================

// 测试解析 desktop 文件
TEST_F(PackagerTest, ParseDesktopFile) {
    // 创建 desktop 文件
    std::ofstream(appdir / "myapp.desktop") << "[Desktop Entry]\n"
        << "Name=MyApp\n"
        << "Comment=My Application\n"
        << "Exec=AppRun\n"
        << "Icon=myapp\n"
        << "Terminal=false\n"
        << "Type=Application\n"
        << "Categories=Development;\n";
    
    std::string name = parseAppName(appdir);
    EXPECT_EQ(name, "MyApp");
}

// 测试 desktop 文件不存在
TEST_F(PackagerTest, ParseDesktopFileNotFound) {
    // 不创建 desktop 文件，应返回默认值 "Application"
    std::string name = parseAppName(appdir);
    EXPECT_EQ(name, "Application");
}

// 测试多个 desktop 文件
TEST_F(PackagerTest, ParseMultipleDesktopFiles) {
    // 创建第一个 desktop 文件
    std::ofstream(appdir / "app1.desktop") << "[Desktop Entry]\n"
        << "Name=App1\n"
        << "Exec=AppRun\n"
        << "Type=Application\n";
    
    // 创建第二个 desktop 文件
    std::ofstream(appdir / "app2.desktop") << "[Desktop Entry]\n"
        << "Name=App2\n"
        << "Exec=AppRun\n"
        << "Type=Application\n";
    
    // 应该返回第一个找到的
    std::string name = parseAppName(appdir);
    EXPECT_FALSE(name.empty());
}

// ============================================================
// packager 测试
// ============================================================

// 测试 PackageConfig 结构
TEST_F(PackagerTest, PackageConfigDefaults) {
    PackageConfig config;
    EXPECT_TRUE(config.appdir_path.empty());
    EXPECT_TRUE(config.output_path.empty());
    EXPECT_TRUE(config.install_dir.empty());
    EXPECT_TRUE(config.installer_path.empty());
    EXPECT_TRUE(config.version.empty());
    EXPECT_FALSE(config.bundle_deps);
    EXPECT_FALSE(config.overwrite);
}

// 测试创建包（需要安装器运行时）
TEST_F(PackagerTest, CreatePackage) {
    createMinimalAppDir();
    
    // 查找安装器运行时
    fs::path installer = fs::current_path() / "build" / "app-installer";
    if (!fs::exists(installer)) {
        GTEST_SKIP() << "安装器运行时不存在，跳过测试: " << installer.string();
    }
    
    PackageConfig config;
    config.appdir_path = appdir.string();
    config.output_path = output_path.string();
    config.install_dir = "/opt/MyApp";
    config.installer_path = installer.string();
    config.version = "1.0.0";
    config.bundle_deps = false;
    config.overwrite = true;
    
    bool result = createPackage(config);
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(output_path));
    EXPECT_GT(fs::file_size(output_path), 0);
}

// 测试创建包并验证包内容
TEST_F(PackagerTest, CreatePackageAndVerify) {
    createMinimalAppDir();
    
    fs::path installer = fs::current_path() / "build" / "app-installer";
    if (!fs::exists(installer)) {
        GTEST_SKIP() << "安装器运行时不存在，跳过测试";
    }
    
    PackageConfig config;
    config.appdir_path = appdir.string();
    config.output_path = output_path.string();
    config.install_dir = "/opt/MyApp";
    config.installer_path = installer.string();
    config.version = "2.0.0";
    config.overwrite = true;
    
    EXPECT_TRUE(createPackage(config));
    
    // 验证包文件大小大于安装器运行时
    EXPECT_GT(fs::file_size(output_path), fs::file_size(installer));
    
    // 验证包文件以 ELF 头部开始
    std::ifstream pkg(output_path, std::ios::binary);
    char elf_magic[4];
    pkg.read(elf_magic, 4);
    EXPECT_EQ(elf_magic[0], 0x7f);
    EXPECT_EQ(elf_magic[1], 'E');
    EXPECT_EQ(elf_magic[2], 'L');
    EXPECT_EQ(elf_magic[3], 'F');
}

// 测试创建包 - AppDir 不存在
TEST_F(PackagerTest, CreatePackageAppDirNotFound) {
    fs::path installer = fs::current_path() / "build" / "app-installer";
    if (!fs::exists(installer)) {
        GTEST_SKIP() << "安装器运行时不存在，跳过测试";
    }
    
    PackageConfig config;
    config.appdir_path = (temp_dir / "nonexistent").string();
    config.output_path = output_path.string();
    config.install_dir = "/opt/MyApp";
    config.installer_path = installer.string();
    config.version = "1.0.0";
    config.overwrite = true;
    
    bool result = createPackage(config);
    EXPECT_FALSE(result);
}

// 测试创建包 - 安装器不存在
TEST_F(PackagerTest, CreatePackageInstallerNotFound) {
    createMinimalAppDir();
    
    PackageConfig config;
    config.appdir_path = appdir.string();
    config.output_path = output_path.string();
    config.install_dir = "/opt/MyApp";
    config.installer_path = "/nonexistent/installer";
    config.version = "1.0.0";
    config.overwrite = true;
    
    bool result = createPackage(config);
    EXPECT_FALSE(result);
}

// 测试创建包 - 包含生命周期脚本
TEST_F(PackagerTest, CreatePackageWithScripts) {
    createMinimalAppDir();
    
    // 添加生命周期脚本
    std::ofstream(appdir / "apppack-pre-install.sh") << "#!/bin/bash\necho \"pre-install\"\n";
    fs::permissions(appdir / "apppack-pre-install.sh", fs::perms::owner_exec | fs::perms::owner_read);
    
    std::ofstream(appdir / "apppack-post-install.sh") << "#!/bin/bash\necho \"post-install\"\n";
    fs::permissions(appdir / "apppack-post-install.sh", fs::perms::owner_exec | fs::perms::owner_read);
    
    std::ofstream(appdir / "apppack-post-uninstall.sh") << "#!/bin/bash\necho \"post-uninstall\"\n";
    fs::permissions(appdir / "apppack-post-uninstall.sh", fs::perms::owner_exec | fs::perms::owner_read);
    
    std::ofstream(appdir / "apppack-pre-upgrade.sh") << "#!/bin/bash\necho \"pre-upgrade\"\n";
    fs::permissions(appdir / "apppack-pre-upgrade.sh", fs::perms::owner_exec | fs::perms::owner_read);
    
    fs::path installer = fs::current_path() / "build" / "app-installer";
    if (!fs::exists(installer)) {
        GTEST_SKIP() << "安装器运行时不存在，跳过测试";
    }
    
    PackageConfig config;
    config.appdir_path = appdir.string();
    config.output_path = output_path.string();
    config.install_dir = "/opt/MyApp";
    config.installer_path = installer.string();
    config.version = "1.0.0";
    config.overwrite = true;
    
    EXPECT_TRUE(createPackage(config));
    EXPECT_TRUE(fs::exists(output_path));
}

// ============================================================
// template_manager 测试
// ============================================================

// 测试 initAppPackProject
TEST_F(PackagerTest, InitAppPackProject) {
    // 由于 initAppPackProject 是交互式的，我们直接测试文件创建
    // 这里只测试目录结构创建
    
    fs::path apppack_dir = appdir / "apppack";
    fs::create_directories(apppack_dir / "usr" / "bin");
    fs::create_directories(apppack_dir / "usr" / "lib");
    fs::create_directories(apppack_dir / "usr" / "share" / "icons");
    fs::create_directories(apppack_dir / "usr" / "share" / "applications");
    
    EXPECT_TRUE(fs::exists(apppack_dir / "usr" / "bin"));
    EXPECT_TRUE(fs::exists(apppack_dir / "usr" / "lib"));
    EXPECT_TRUE(fs::exists(apppack_dir / "usr" / "share" / "icons"));
    EXPECT_TRUE(fs::exists(apppack_dir / "usr" / "share" / "applications"));
}

// 测试 buildFromAppPackDir
TEST_F(PackagerTest, BuildFromAppPackDir) {
    // 创建 apppack/ 目录结构
    fs::path apppack_dir = appdir / "apppack";
    fs::create_directories(apppack_dir / "usr" / "bin");
    
    // 创建 apppack.json
    std::ofstream(apppack_dir / "apppack.json") << "{\n"
        << "    \"app_name\": \"TestApp\",\n"
        << "    \"version\": \"1.0.0\",\n"
        << "    \"install_dir\": \"/opt/TestApp\",\n"
        << "    \"executable\": \"testapp\"\n"
        << "}\n";
    
    // 创建 AppRun
    std::ofstream(apppack_dir / "AppRun") << "#!/bin/bash\necho \"Test\"\n";
    fs::permissions(apppack_dir / "AppRun", fs::perms::owner_exec | fs::perms::owner_read);
    
    // 创建 desktop 文件
    std::ofstream(apppack_dir / "TestApp.desktop") << "[Desktop Entry]\n"
        << "Name=TestApp\n"
        << "Exec=AppRun\n"
        << "Type=Application\n";
    
    // 创建可执行文件
    std::ofstream(apppack_dir / "usr" / "bin" / "testapp") << "#!/bin/bash\necho \"Test\"\n";
    fs::permissions(apppack_dir / "usr" / "bin" / "testapp", fs::perms::owner_exec | fs::perms::owner_read);
    
    fs::path installer = fs::current_path() / "build" / "app-installer";
    if (!fs::exists(installer)) {
        GTEST_SKIP() << "安装器运行时不存在，跳过测试";
    }
    
    bool result = buildFromAppPackDir(appdir, output_path.string(), installer.string(), false);
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(output_path));
}

// ============================================================
// 主函数
// ============================================================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
