/**
 * test_integration.cpp - 集成测试
 * 
 * 测试完整的打包→安装→卸载流程
 * 使用 builder 的 packager 创建包，然后用 installer 安装和卸载
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <cstdlib>

// builder 模块
#include "common.h"
#include "file_utils.h"
#include "compressor.h"
#include "desktop_parser.h"
#include "packager.h"
#include "dependency_analyzer.h"
#include "template_manager.h"

// installer 模块
#include "header_reader.h"
#include "desktop_integration.h"
#include "script_runner.h"
#include "installer.h"
#include "uninstaller.h"

namespace fs = std::filesystem;

// ============================================================
// 测试夹具
// ============================================================
class IntegrationTest : public ::testing::Test {
protected:
    fs::path temp_dir;
    fs::path appdir;
    fs::path output_path;
    fs::path install_path;
    fs::path installer_path;
    
    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "apppack_test_integration_XXXXXX";
        std::string tmpl = temp_dir.string();
        if (mkdtemp(tmpl.data())) {
            temp_dir = tmpl;
        } else {
            temp_dir = fs::temp_directory_path() / ("apppack_test_integration_" + std::to_string(time(nullptr)));
            fs::create_directories(temp_dir);
        }
        
        appdir = temp_dir / "TestApp.AppDir";
        output_path = temp_dir / "TestApp-v1.0.0.apppack";
        install_path = temp_dir / "opt" / "TestApp";
        
        // 查找安装器运行时
        installer_path = fs::current_path() / "build" / "app-installer";
        if (!fs::exists(installer_path)) {
            // 尝试其他路径
            installer_path = fs::current_path() / "app-installer";
        }
        
        // 创建 AppDir
        createTestAppDir();
    }
    
    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }
    
    // 创建完整的测试应用目录
    void createTestAppDir() {
        fs::create_directories(appdir / "usr" / "bin");
        fs::create_directories(appdir / "usr" / "lib");
        fs::create_directories(appdir / "usr" / "share" / "icons");
        
        // 创建 AppRun
        std::ofstream(appdir / "AppRun") << "#!/bin/bash\n"
            << "APPDIR=\"$(dirname \"$(readlink -f \"$0\")\")\"\n"
            << "export LD_LIBRARY_PATH=\"${APPDIR}/usr/lib:${LD_LIBRARY_PATH}\"\n"
            << "exec \"${APPDIR}/usr/bin/testapp\" \"$@\"\n";
        fs::permissions(appdir / "AppRun", fs::perms::owner_exec | fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read);
        
        // 创建 desktop 文件
        std::ofstream(appdir / "TestApp.desktop") << "[Desktop Entry]\n"
            << "Name=TestApp\n"
            << "Comment=Test Application\n"
            << "Exec=AppRun\n"
            << "Icon=TestApp\n"
            << "Terminal=false\n"
            << "Type=Application\n"
            << "Categories=Development;\n"
            << "StartupNotify=true\n";
        
        // 创建 SVG 图标
        std::ofstream(appdir / "TestApp.svg") << "<?xml version=\"1.0\"?>\n"
            << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"64\">\n"
            << "  <rect width=\"64\" height=\"64\" fill=\"blue\"/>\n"
            << "  <text x=\"32\" y=\"40\" fill=\"white\" text-anchor=\"middle\">T</text>\n"
            << "</svg>\n";
        
        // 创建可执行文件
        std::ofstream(appdir / "usr" / "bin" / "testapp") << "#!/bin/bash\n"
            << "echo \"TestApp v1.0.0\"\n"
            << "echo \"Install dir: ${APPPACK_INSTALL_DIR:-unknown}\"\n";
        fs::permissions(appdir / "usr" / "bin" / "testapp", fs::perms::owner_exec | fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read);
        
        // 创建生命周期脚本
        std::ofstream(appdir / "apppack-pre-install.sh") << "#!/bin/bash\n"
            << "echo \"pre-install: ${APPPACK_APP_NAME} ${APPPACK_VERSION}\"\n";
        fs::permissions(appdir / "apppack-pre-install.sh", fs::perms::owner_exec | fs::perms::owner_read);
        
        std::ofstream(appdir / "apppack-post-install.sh") << "#!/bin/bash\n"
            << "echo \"post-install: ${APPPACK_APP_NAME} ${APPPACK_VERSION}\"\n";
        fs::permissions(appdir / "apppack-post-install.sh", fs::perms::owner_exec | fs::perms::owner_read);
        
        std::ofstream(appdir / "apppack-post-uninstall.sh") << "#!/bin/bash\n"
            << "echo \"post-uninstall: ${APPPACK_APP_NAME}\"\n";
        fs::permissions(appdir / "apppack-post-uninstall.sh", fs::perms::owner_exec | fs::perms::owner_read);
        
        std::ofstream(appdir / "apppack-pre-upgrade.sh") << "#!/bin/bash\n"
            << "echo \"pre-upgrade: ${APPPACK_APP_NAME} ${APPPACK_VERSION}\"\n";
        fs::permissions(appdir / "apppack-pre-upgrade.sh", fs::perms::owner_exec | fs::perms::owner_read);
    }
};

// ============================================================
// 完整流程测试
// ============================================================

// 测试完整的打包→安装→验证→卸载流程
TEST_F(IntegrationTest, FullPackageInstallUninstall) {
    if (!fs::exists(installer_path)) {
        GTEST_SKIP() << "安装器运行时不存在，跳过集成测试: " << installer_path.string();
    }
    
    // ========== 1. 打包 ==========
    std::cout << "\n[集成测试] 步骤 1: 打包应用..." << std::endl;
    
    PackageConfig config;
    config.appdir_path = appdir.string();
    config.output_path = output_path.string();
    config.install_dir = install_path.string();
    config.installer_path = installer_path.string();
    config.version = "1.0.0";
    config.bundle_deps = false;
    config.overwrite = true;
    
    bool package_ok = createPackage(config);
    ASSERT_TRUE(package_ok) << "打包失败";
    ASSERT_TRUE(fs::exists(output_path)) << "安装包文件未创建";
    EXPECT_GT(fs::file_size(output_path), 0) << "安装包文件为空";
    
    std::cout << "  ✓ 打包成功: " << output_path.string() << std::endl;
    std::cout << "    大小: " << fs::file_size(output_path) << " 字节" << std::endl;
    
    // ========== 2. 读取包信息 ==========
    std::cout << "\n[集成测试] 步骤 2: 读取包信息..." << std::endl;
    
    PackageHeader header = readHeader(output_path.string());
    EXPECT_STREQ(header.app_name, "TestApp");
    EXPECT_STREQ(header.version, "1.0.0");
    EXPECT_STREQ(header.install_dir, install_path.string().c_str());
    EXPECT_GT(header.data_offset, 0);
    EXPECT_GT(header.data_size, 0);
    EXPECT_GT(header.orig_size, 0);
    
    std::cout << "  ✓ 包信息读取成功" << std::endl;
    std::cout << "    应用: " << header.app_name << std::endl;
    std::cout << "    版本: " << header.version << std::endl;
    std::cout << "    安装目录: " << header.install_dir << std::endl;
    
    // ========== 3. 安装 ==========
    std::cout << "\n[集成测试] 步骤 3: 安装应用到 " << install_path.string() << "..." << std::endl;
    
    InstallResult install_result = doInstall(output_path.string(), header, install_path.string());
    ASSERT_TRUE(install_result.success) << "安装失败";
    EXPECT_FALSE(install_result.was_upgrade) << "首次安装不应是升级";
    EXPECT_EQ(install_result.new_version, "1.0.0");
    
    // 验证文件被安装
    EXPECT_TRUE(fs::exists(install_path / "AppRun"));
    EXPECT_TRUE(fs::exists(install_path / "TestApp.desktop"));
    EXPECT_TRUE(fs::exists(install_path / "TestApp.svg"));
    EXPECT_TRUE(fs::exists(install_path / "usr" / "bin" / "testapp"));
    EXPECT_TRUE(fs::exists(install_path / ".apppack-version"));
    EXPECT_TRUE(fs::exists(install_path / "uninstall.sh"));
    
    // 验证生命周期脚本被安装
    EXPECT_TRUE(fs::exists(install_path / "apppack-pre-install.sh"));
    EXPECT_TRUE(fs::exists(install_path / "apppack-post-install.sh"));
    EXPECT_TRUE(fs::exists(install_path / "apppack-post-uninstall.sh"));
    EXPECT_TRUE(fs::exists(install_path / "apppack-pre-upgrade.sh"));
    
    // 验证版本文件内容
    std::ifstream vf(install_path / ".apppack-version");
    std::string version;
    std::getline(vf, version);
    EXPECT_EQ(version, "1.0.0");
    
    std::cout << "  ✓ 安装成功" << std::endl;
    std::cout << "    文件数: 验证通过" << std::endl;
    
    // ========== 4. 验证应用可执行 ==========
    std::cout << "\n[集成测试] 步骤 4: 验证应用可执行..." << std::endl;
    
    // 设置环境变量并执行
    std::string cmd = "export APPPACK_INSTALL_DIR=\"" + install_path.string() + "\"; "
                      "bash \"" + (install_path / "usr" / "bin" / "testapp").string() + "\" 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    ASSERT_NE(pipe, nullptr) << "无法执行应用";
    
    char buffer[256];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        output += buffer;
    }
    int exit_code = pclose(pipe);
    
    EXPECT_EQ(exit_code, 0) << "应用执行失败";
    EXPECT_NE(output.find("TestApp v1.0.0"), std::string::npos) << "应用输出不正确";
    EXPECT_NE(output.find(install_path.string()), std::string::npos) << "环境变量未正确传递";
    
    std::cout << "  ✓ 应用可执行" << std::endl;
    std::cout << "    输出: " << output;
    
    // ========== 5. 验证卸载脚本 ==========
    std::cout << "\n[集成测试] 步骤 5: 验证卸载脚本..." << std::endl;
    
    EXPECT_TRUE(fs::exists(install_path / "uninstall.sh"));
    
    // 检查卸载脚本内容
    std::ifstream us(install_path / "uninstall.sh");
    std::string uninstall_content((std::istreambuf_iterator<char>(us)), std::istreambuf_iterator<char>());
    EXPECT_NE(uninstall_content.find("rm -rf"), std::string::npos) << "卸载脚本应包含删除命令";
    EXPECT_NE(uninstall_content.find(install_path.string()), std::string::npos) << "卸载脚本应包含安装路径";
    
    std::cout << "  ✓ 卸载脚本验证通过" << std::endl;
    
    // ========== 6. 卸载 ==========
    std::cout << "\n[集成测试] 步骤 6: 卸载应用..." << std::endl;
    
    bool uninstall_ok = doUninstall(install_path.string(), header);
    ASSERT_TRUE(uninstall_ok) << "卸载失败";
    
    // 验证安装目录被删除
    EXPECT_FALSE(fs::exists(install_path)) << "安装目录应已被删除";
    
    std::cout << "  ✓ 卸载成功" << std::endl;
    std::cout << "    安装目录已删除" << std::endl;
    
    // ========== 总结 ==========
    std::cout << "\n========================================" << std::endl;
    std::cout << "  集成测试全部通过!" << std::endl;
    std::cout << "  流程: 打包 → 安装 → 验证 → 卸载" << std::endl;
    std::cout << "========================================" << std::endl;
}

// 测试升级安装流程
TEST_F(IntegrationTest, UpgradeInstall) {
    if (!fs::exists(installer_path)) {
        GTEST_SKIP() << "安装器运行时不存在，跳过集成测试";
    }
    
    // ========== 安装 v1.0.0 ==========
    PackageConfig config;
    config.appdir_path = appdir.string();
    config.output_path = output_path.string();
    config.install_dir = install_path.string();
    config.installer_path = installer_path.string();
    config.version = "1.0.0";
    config.overwrite = true;
    
    ASSERT_TRUE(createPackage(config));
    PackageHeader header = readHeader(output_path.string());
    InstallResult result1 = doInstall(output_path.string(), header, install_path.string());
    ASSERT_TRUE(result1.success);
    EXPECT_FALSE(result1.was_upgrade);
    
    // ========== 升级到 v2.0.0 ==========
    // 修改版本号并重新打包
    config.version = "2.0.0";
    fs::path output_v2 = temp_dir / "TestApp-v2.0.0.apppack";
    config.output_path = output_v2.string();
    ASSERT_TRUE(createPackage(config));
    
    PackageHeader header_v2 = readHeader(output_v2.string());
    InstallResult result2 = doInstall(output_v2.string(), header_v2, install_path.string());
    
    EXPECT_TRUE(result2.success);
    EXPECT_TRUE(result2.was_upgrade) << "应检测到升级";
    EXPECT_EQ(result2.old_version, "1.0.0");
    EXPECT_EQ(result2.new_version, "2.0.0");
    
    // 验证版本文件已更新
    std::ifstream vf(install_path / ".apppack-version");
    std::string version;
    std::getline(vf, version);
    EXPECT_EQ(version, "2.0.0");
    
    // 清理
    doUninstall(install_path.string(), header_v2);
}

// 测试降级安装流程
TEST_F(IntegrationTest, DowngradeInstall) {
    if (!fs::exists(installer_path)) {
        GTEST_SKIP() << "安装器运行时不存在，跳过集成测试";
    }
    
    // ========== 安装 v2.0.0 ==========
    PackageConfig config;
    config.appdir_path = appdir.string();
    config.output_path = output_path.string();
    config.install_dir = install_path.string();
    config.installer_path = installer_path.string();
    config.version = "2.0.0";
    config.overwrite = true;
    
    ASSERT_TRUE(createPackage(config));
    PackageHeader header = readHeader(output_path.string());
    InstallResult result1 = doInstall(output_path.string(), header, install_path.string());
    ASSERT_TRUE(result1.success);
    
    // ========== 降级到 v1.0.0 ==========
    config.version = "1.0.0";
    fs::path output_v1 = temp_dir / "TestApp-v1.0.0.apppack";
    config.output_path = output_v1.string();
    ASSERT_TRUE(createPackage(config));
    
    PackageHeader header_v1 = readHeader(output_v1.string());
    InstallResult result2 = doInstall(output_v1.string(), header_v1, install_path.string());
    
    EXPECT_TRUE(result2.success);
    // 降级安装时 was_upgrade 可能为 false，但安装应该成功
    // 主要验证版本文件已更新
    std::ifstream vf(install_path / ".apppack-version");
    std::string version;
    std::getline(vf, version);
    EXPECT_EQ(version, "1.0.0");
    
    // 清理
    doUninstall(install_path.string(), header_v1);
}

// 测试打包含依赖的应用
TEST_F(IntegrationTest, PackageWithDeps) {
    if (!fs::exists(installer_path)) {
        GTEST_SKIP() << "安装器运行时不存在，跳过集成测试";
    }
    
    // 复制一个真正的 ELF 文件
    if (fs::exists("/bin/echo")) {
        fs::copy("/bin/echo", appdir / "usr" / "bin" / "testapp",
                 fs::copy_options::overwrite_existing);
        fs::permissions(appdir / "usr" / "bin" / "testapp", fs::perms::owner_exec | fs::perms::owner_read);
    }
    
    PackageConfig config;
    config.appdir_path = appdir.string();
    config.output_path = output_path.string();
    config.install_dir = install_path.string();
    config.installer_path = installer_path.string();
    config.version = "1.0.0";
    config.bundle_deps = true;  // 打包依赖
    config.overwrite = true;
    
    bool package_ok = createPackage(config);
    EXPECT_TRUE(package_ok);
    
    if (package_ok) {
        EXPECT_TRUE(fs::exists(output_path));
        EXPECT_GT(fs::file_size(output_path), 0);
    }
}

// 测试使用 --build 方式打包
TEST_F(IntegrationTest, BuildFromAppPackDir) {
    if (!fs::exists(installer_path)) {
        GTEST_SKIP() << "安装器运行时不存在，跳过集成测试";
    }
    
    // 创建 apppack/ 目录结构
    fs::path apppack_dir = appdir / "apppack";
    fs::create_directories(apppack_dir / "usr" / "bin");
    
    // 创建 apppack.json
    std::ofstream(apppack_dir / "apppack.json") << "{\n"
        << "    \"app_name\": \"TestApp\",\n"
        << "    \"version\": \"1.0.0\",\n"
        << "    \"install_dir\": \"" << install_path.string() << "\",\n"
        << "    \"executable\": \"testapp\",\n"
        << "    \"description\": \"Test Application\",\n"
        << "    \"categories\": \"Development;\",\n"
        << "    \"terminal\": false,\n"
        << "    \"bundle_deps\": false\n"
        << "}\n";
    
    // 创建 AppRun
    std::ofstream(apppack_dir / "AppRun") << "#!/bin/bash\necho \"Hello\"\n";
    fs::permissions(apppack_dir / "AppRun", fs::perms::owner_exec | fs::perms::owner_read);
    
    // 创建 desktop 文件
    std::ofstream(apppack_dir / "TestApp.desktop") << "[Desktop Entry]\n"
        << "Name=TestApp\n"
        << "Exec=AppRun\n"
        << "Type=Application\n";
    
    // 创建可执行文件
    std::ofstream(apppack_dir / "usr" / "bin" / "testapp") << "#!/bin/bash\necho \"Hello\"\n";
    fs::permissions(apppack_dir / "usr" / "bin" / "testapp", fs::perms::owner_exec | fs::perms::owner_read);
    
    bool result = buildFromAppPackDir(appdir, output_path.string(), installer_path.string(), false);
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
