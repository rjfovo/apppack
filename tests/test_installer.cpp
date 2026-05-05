/**
 * test_installer.cpp - 安装器模块单元测试
 * 
 * 测试内容:
 *   - header_reader: 包头部读取
 *   - desktop_integration: 桌面集成
 *   - script_runner: 生命周期脚本执行
 *   - installer: 安装逻辑
 *   - uninstaller: 卸载逻辑
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include "common.h"
#include "file_utils.h"
#include "compressor.h"
#include "header_reader.h"
#include "desktop_integration.h"
#include "script_runner.h"
#include "installer.h"
#include "uninstaller.h"

namespace fs = std::filesystem;

// ============================================================
// 测试夹具
// ============================================================
class InstallerTest : public ::testing::Test {
protected:
    fs::path temp_dir;
    fs::path install_dir;
    
    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "apppack_test_installer_XXXXXX";
        std::string tmpl = temp_dir.string();
        if (mkdtemp(tmpl.data())) {
            temp_dir = tmpl;
        } else {
            temp_dir = fs::temp_directory_path() / ("apppack_test_installer_" + std::to_string(time(nullptr)));
            fs::create_directories(temp_dir);
        }
        
        install_dir = temp_dir / "installed";
    }
    
    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }
    
    // 创建测试安装包文件
    fs::path createTestPackage() {
        fs::path pkg_path = temp_dir / "test.apppack";
        
        // 创建测试文件列表
        std::vector<FileEntry> files;
        
        FileEntry f1;
        f1.relative_path = "usr/bin/myapp";
        f1.content = {'#', '!', '/', 'b', 'i', 'n', '/', 'b', 'a', 's', 'h', '\n',
                      'e', 'c', 'h', 'o', ' ', '"', 'H', 'e', 'l', 'l', 'o', '"', '\n'};
        f1.permissions = 0755;
        files.push_back(f1);
        
        FileEntry f2;
        f2.relative_path = "usr/share/icons/myapp.svg";
        f2.content = {'<', 's', 'v', 'g', '>', '<', '/', 's', 'v', 'g', '>'};
        f2.permissions = 0644;
        files.push_back(f2);
        
        FileEntry f3;
        f3.relative_path = "myapp.desktop";
        f3.content = {'[', 'D', 'e', 's', 'k', 't', 'o', 'p', ' ', 'E', 'n', 't', 'r', 'y', ']', '\n',
                      'N', 'a', 'm', 'e', '=', 'M', 'y', 'A', 'p', 'p', '\n',
                      'E', 'x', 'e', 'c', '=', 'A', 'p', 'p', 'R', 'u', 'n', '\n',
                      'T', 'y', 'p', 'e', '=', 'A', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '\n'};
        f3.permissions = 0644;
        files.push_back(f3);
        
        // 序列化
        auto serialized = serializeFiles(files);
        
        // 压缩
        auto compressed = compressData(serialized);
        
        // 创建包头部
        PackageHeader header;
        std::strncpy(header.app_name, "MyApp", sizeof(header.app_name) - 1);
        std::strncpy(header.install_dir, install_dir.string().c_str(), sizeof(header.install_dir) - 1);
        std::strncpy(header.version, "1.0.0", sizeof(header.version) - 1);
        header.data_offset = sizeof(PackageHeader);
        header.data_size = compressed.size();
        header.orig_size = serialized.size();
        header.checksum = calculateChecksum(compressed);
        
        // 写入文件
        std::ofstream pkg(pkg_path, std::ios::binary);
        pkg.write(reinterpret_cast<const char*>(&header), sizeof(header));
        pkg.write(compressed.data(), compressed.size());
        pkg.close();
        
        return pkg_path;
    }
};

// ============================================================
// header_reader 测试
// ============================================================

// 测试读取包头部
TEST_F(InstallerTest, ReadHeader) {
    fs::path pkg_path = createTestPackage();
    
    PackageHeader header = readHeader(pkg_path.string());
    
    EXPECT_STREQ(header.app_name, "MyApp");
    EXPECT_STREQ(header.version, "1.0.0");
    EXPECT_EQ(header.data_offset, sizeof(PackageHeader));
    EXPECT_GT(header.data_size, 0);
    EXPECT_GT(header.orig_size, 0);
}

// 测试读取不存在的文件
TEST_F(InstallerTest, ReadHeaderFileNotFound) {
    EXPECT_THROW(readHeader("/nonexistent/file.apppack"), std::runtime_error);
}

// 测试读取无效文件
TEST_F(InstallerTest, ReadHeaderInvalidFile) {
    fs::path invalid = temp_dir / "invalid.apppack";
    std::ofstream(invalid) << "not a valid package";
    
    EXPECT_THROW(readHeader(invalid.string()), std::runtime_error);
}

// ============================================================
// desktop_integration 测试
// ============================================================

// 测试 setupDesktopIntegration
TEST_F(InstallerTest, SetupDesktopIntegration) {
    // 创建测试文件
    std::vector<FileEntry> files;
    
    FileEntry desktop;
    desktop.relative_path = "myapp.desktop";
    desktop.content = {'[', 'D', 'e', 's', 'k', 't', 'o', 'p', ' ', 'E', 'n', 't', 'r', 'y', ']', '\n',
                      'N', 'a', 'm', 'e', '=', 'M', 'y', 'A', 'p', 'p', '\n',
                      'E', 'x', 'e', 'c', '=', 'A', 'p', 'p', 'R', 'u', 'n', '\n',
                      'T', 'y', 'p', 'e', '=', 'A', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '\n'};
    desktop.permissions = 0644;
    files.push_back(desktop);
    
    FileEntry icon;
    icon.relative_path = "myapp.svg";
    icon.content = {'<', 's', 'v', 'g', '>', '<', '/', 's', 'v', 'g', '>'};
    icon.permissions = 0644;
    files.push_back(icon);
    
    // 执行桌面集成
    setupDesktopIntegration(install_dir.string(), "MyApp", files);
    
    // 验证桌面文件被创建
    fs::path desktop_dest = fs::path(getenv("HOME")) / ".local/share/applications/MyApp.desktop";
    EXPECT_TRUE(fs::exists(desktop_dest));
    
    // 清理
    fs::remove(desktop_dest);
}

// ============================================================
// script_runner 测试
// ============================================================

// 测试脚本不存在
TEST_F(InstallerTest, RunScriptNotExists) {
    ScriptResult result = runPreInstall((install_dir / "nonexistent.sh").string(), "MyApp", "1.0.0");
    EXPECT_FALSE(result.executed);
    EXPECT_TRUE(result.success);
}

// 测试安装前脚本
TEST_F(InstallerTest, RunPreInstallScript) {
    // 创建测试脚本
    fs::create_directories(install_dir);
    std::ofstream(install_dir / "apppack-pre-install.sh") << "#!/bin/bash\necho \"pre-install ok\"\n";
    fs::permissions(install_dir / "apppack-pre-install.sh", fs::perms::owner_exec | fs::perms::owner_read);
    
    ScriptResult result = runPreInstall(install_dir.string(), "MyApp", "1.0.0");
    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.success);
}

// 测试安装后脚本
TEST_F(InstallerTest, RunPostInstallScript) {
    fs::create_directories(install_dir);
    std::ofstream(install_dir / "apppack-post-install.sh") << "#!/bin/bash\necho \"post-install ok\"\n";
    fs::permissions(install_dir / "apppack-post-install.sh", fs::perms::owner_exec | fs::perms::owner_read);
    
    ScriptResult result = runPostInstall(install_dir.string(), "MyApp", "1.0.0");
    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.success);
}

// 测试卸载后脚本
TEST_F(InstallerTest, RunPostUninstallScript) {
    fs::create_directories(install_dir);
    std::ofstream(install_dir / "apppack-post-uninstall.sh") << "#!/bin/bash\necho \"post-uninstall ok\"\n";
    fs::permissions(install_dir / "apppack-post-uninstall.sh", fs::perms::owner_exec | fs::perms::owner_read);
    
    ScriptResult result = runPostUninstall(install_dir.string(), "MyApp", "1.0.0");
    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.success);
}

// 测试升级前脚本
TEST_F(InstallerTest, RunPreUpgradeScript) {
    fs::create_directories(install_dir);
    std::ofstream(install_dir / "apppack-pre-upgrade.sh") << "#!/bin/bash\necho \"pre-upgrade ok\"\n";
    fs::permissions(install_dir / "apppack-pre-upgrade.sh", fs::perms::owner_exec | fs::perms::owner_read);
    
    ScriptResult result = runPreUpgrade(install_dir.string(), "MyApp", "1.0.0");
    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.success);
}

// 测试脚本执行失败
TEST_F(InstallerTest, RunScriptFailure) {
    fs::create_directories(install_dir);
    std::ofstream(install_dir / "apppack-pre-install.sh") << "#!/bin/bash\nexit 1\n";
    fs::permissions(install_dir / "apppack-pre-install.sh", fs::perms::owner_exec | fs::perms::owner_read);
    
    ScriptResult result = runPreInstall(install_dir.string(), "MyApp", "1.0.0");
    EXPECT_TRUE(result.executed);
    EXPECT_FALSE(result.success);
}

// 测试脚本环境变量
TEST_F(InstallerTest, RunScriptEnvironment) {
    fs::create_directories(install_dir);
    std::ofstream(install_dir / "apppack-post-install.sh") << "#!/bin/bash\necho \"APP_NAME=$APP_NAME\"\necho \"VERSION=$VERSION\"\necho \"DIR=$DIR\"\n";
    fs::permissions(install_dir / "apppack-post-install.sh", fs::perms::owner_exec | fs::perms::owner_read);
    
    ScriptResult result = runPostInstall(install_dir.string(), "MyApp", "1.0.0");
    EXPECT_TRUE(result.executed);
    EXPECT_TRUE(result.success);
}

// ============================================================
// installer 测试
// ============================================================

// 测试 doInstall
TEST_F(InstallerTest, DoInstall) {
    fs::path pkg_path = createTestPackage();
    PackageHeader header = readHeader(pkg_path.string());
    
    InstallResult result = doInstall(pkg_path.string(), header, install_dir.string());
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(fs::exists(install_dir));
    EXPECT_TRUE(fs::exists(install_dir / "usr" / "bin" / "myapp"));
    EXPECT_TRUE(fs::exists(install_dir / "uninstall.sh"));
}

// 测试覆盖安装
TEST_F(InstallerTest, DoInstallOverwrite) {
    fs::path pkg_path = createTestPackage();
    PackageHeader header = readHeader(pkg_path.string());
    
    // 第一次安装
    InstallResult result1 = doInstall(pkg_path.string(), header, install_dir.string());
    EXPECT_TRUE(result1.success);
    EXPECT_TRUE(fs::exists(install_dir));
    
    // 第二次安装（覆盖）
    InstallResult result2 = doInstall(pkg_path.string(), header, install_dir.string());
    EXPECT_TRUE(result2.success);
    EXPECT_TRUE(fs::exists(install_dir));
}

// 测试安装到新目录
TEST_F(InstallerTest, DoInstallToNewDir) {
    fs::path pkg_path = createTestPackage();
    PackageHeader header = readHeader(pkg_path.string());
    fs::path new_install = temp_dir / "new_install";
    
    InstallResult result = doInstall(pkg_path.string(), header, new_install.string());
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(fs::exists(new_install));
}

// ============================================================
// uninstaller 测试
// ============================================================

// 测试 doUninstall
TEST_F(InstallerTest, DoUninstall) {
    fs::path pkg_path = createTestPackage();
    PackageHeader header = readHeader(pkg_path.string());
    
    // 先安装
    InstallResult install_result = doInstall(pkg_path.string(), header, install_dir.string());
    EXPECT_TRUE(install_result.success);
    
    // 验证安装成功
    EXPECT_TRUE(fs::exists(install_dir));
    
    // 执行卸载
    bool uninstalled = doUninstall(install_dir.string(), header);
    EXPECT_TRUE(uninstalled);
    
    // 验证安装目录被删除
    EXPECT_FALSE(fs::exists(install_dir));
}

// 测试卸载不存在的应用
TEST_F(InstallerTest, DoUninstallNotExists) {
    PackageHeader dummy_header;
    std::strncpy(dummy_header.app_name, "Nonexistent", sizeof(dummy_header.app_name) - 1);
    std::strncpy(dummy_header.install_dir, "/nonexistent/app", sizeof(dummy_header.install_dir) - 1);
    bool uninstalled = doUninstall("/nonexistent/app", dummy_header);
    // doUninstall 总是返回 true（即使目录不存在也视为成功）
    EXPECT_TRUE(uninstalled);
}

// ============================================================
// 主函数
// ============================================================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
