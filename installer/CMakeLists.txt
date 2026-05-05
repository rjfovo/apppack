cmake_minimum_required(VERSION 3.10)
project(app-installer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 公共模块源文件
set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/file_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/compressor.cpp
)

# 源文件
set(SOURCES
    main.cpp
    installer.cpp
    uninstaller.cpp
    file_utils.cpp
    header_reader.cpp
    desktop_integration.cpp
    script_runner.cpp
    ${COMMON_SOURCES}
)

add_executable(app-installer ${SOURCES})

# 包含路径
target_include_directories(app-installer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

# 查找 zstd 库
# 先尝试 pkg-config
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(ZSTD QUIET libzstd)
endif()

if(ZSTD_FOUND)
    message(STATUS "Found zstd via pkg-config: ${ZSTD_LIBRARIES}")
    target_include_directories(app-installer PRIVATE ${ZSTD_INCLUDE_DIRS})
    target_link_libraries(app-installer PRIVATE ${ZSTD_LIBRARIES})
else()
    # 直接查找 libzstd.so
    find_library(ZSTD_LIBRARY zstd PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/lib64)
    if(NOT ZSTD_LIBRARY)
        # 尝试直接链接 .so.1
        find_library(ZSTD_LIBRARY zstd.so.1 PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/lib64)
    endif()
    if(ZSTD_LIBRARY)
        message(STATUS "Found zstd: ${ZSTD_LIBRARY}")
        target_link_libraries(app-installer PRIVATE ${ZSTD_LIBRARY})
    else()
        # 最后尝试直接链接
        if(EXISTS "/usr/lib/x86_64-linux-gnu/libzstd.so.1")
            message(STATUS "Found zstd: /usr/lib/x86_64-linux-gnu/libzstd.so.1")
            target_link_libraries(app-installer PRIVATE "/usr/lib/x86_64-linux-gnu/libzstd.so.1")
        else()
            message(FATAL_ERROR "libzstd not found. Install libzstd-dev or libzstd-devel")
        endif()
    endif()
endif()
