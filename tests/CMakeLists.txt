cmake_minimum_required(VERSION 3.14)
project(apppack-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# 自动下载 Google Test
# ============================================================
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ============================================================
# 公共模块源文件
# ============================================================
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(COMMON_SOURCES
    ${PROJECT_ROOT}/common/file_utils.cpp
    ${PROJECT_ROOT}/common/compressor.cpp
)

# builder 模块源文件（不含 main.cpp）
set(BUILDER_SOURCES
    ${PROJECT_ROOT}/builder/desktop_parser.cpp
    ${PROJECT_ROOT}/builder/dependency_analyzer.cpp
    ${PROJECT_ROOT}/builder/packager.cpp
    ${PROJECT_ROOT}/builder/template_manager.cpp
    ${COMMON_SOURCES}
)

# installer 模块源文件（不含 main.cpp）
set(INSTALLER_SOURCES
    ${PROJECT_ROOT}/installer/file_utils.cpp
    ${PROJECT_ROOT}/installer/header_reader.cpp
    ${PROJECT_ROOT}/installer/desktop_integration.cpp
    ${PROJECT_ROOT}/installer/script_runner.cpp
    ${PROJECT_ROOT}/installer/installer.cpp
    ${PROJECT_ROOT}/installer/uninstaller.cpp
    ${COMMON_SOURCES}
)

# ============================================================
# 查找 zstd 库
# ============================================================
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(ZSTD QUIET libzstd)
endif()

if(ZSTD_FOUND)
    set(ZSTD_LIBS ${ZSTD_LIBRARIES})
    set(ZSTD_INCS ${ZSTD_INCLUDE_DIRS})
else()
    find_library(ZSTD_LIBRARY zstd PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/lib64)
    if(NOT ZSTD_LIBRARY)
        find_library(ZSTD_LIBRARY zstd.so.1 PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/lib64)
    endif()
    if(ZSTD_LIBRARY)
        set(ZSTD_LIBS ${ZSTD_LIBRARY})
    else()
        message(FATAL_ERROR "libzstd not found. Install libzstd-dev or libzstd-devel")
    endif()
endif()

# ============================================================
# 通用包含路径
# ============================================================
set(COMMON_INCS
    ${PROJECT_ROOT}/builder
    ${PROJECT_ROOT}/installer
    ${PROJECT_ROOT}/common
    ${ZSTD_INCS}
)

# ============================================================
# 测试可执行文件
# ============================================================

# 测试 1: 通用工具测试（common.h, file_utils, compressor）
add_executable(test_common
    test_common.cpp
    ${COMMON_SOURCES}
)
target_include_directories(test_common PRIVATE ${COMMON_INCS})
target_link_libraries(test_common PRIVATE gtest_main ${ZSTD_LIBS})
add_test(NAME test_common COMMAND test_common)

# 测试 2: 打包器测试（packager, desktop_parser）
add_executable(test_packager
    test_packager.cpp
    ${BUILDER_SOURCES}
)
target_include_directories(test_packager PRIVATE ${COMMON_INCS})
target_link_libraries(test_packager PRIVATE gtest_main ${ZSTD_LIBS})
add_test(NAME test_packager COMMAND test_packager)

# 测试 3: 安装器测试（installer, uninstaller, header_reader, desktop_integration）
add_executable(test_installer
    test_installer.cpp
    ${INSTALLER_SOURCES}
)
target_include_directories(test_installer PRIVATE ${COMMON_INCS})
target_link_libraries(test_installer PRIVATE gtest_main ${ZSTD_LIBS})
add_test(NAME test_installer COMMAND test_installer)

# 测试 4: 依赖分析测试
add_executable(test_deps
    test_deps.cpp
    ${BUILDER_SOURCES}
)
target_include_directories(test_deps PRIVATE ${COMMON_INCS})
target_link_libraries(test_deps PRIVATE gtest_main ${ZSTD_LIBS})
add_test(NAME test_deps COMMAND test_deps)

# 测试 5: 集成测试（完整的打包→安装→卸载流程）
add_executable(test_integration
    test_integration.cpp
    ${BUILDER_SOURCES}
    ${INSTALLER_SOURCES}
)
target_include_directories(test_integration PRIVATE ${COMMON_INCS})
target_link_libraries(test_integration PRIVATE gtest_main ${ZSTD_LIBS})
add_test(NAME test_integration COMMAND test_integration)

# ============================================================
# 启用测试
# ============================================================
enable_testing()
