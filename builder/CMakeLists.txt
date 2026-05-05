cmake_minimum_required(VERSION 3.10)
project(app-builder CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 静态链接以支持不同发行版
set(CMAKE_EXE_LINKER_FLAGS "-static")

# 公共模块源文件
set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/file_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/compressor.cpp
)

# 源文件
set(SOURCES
    main.cpp
    packager.cpp
    desktop_parser.cpp
    dependency_analyzer.cpp
    template_manager.cpp
    ${COMMON_SOURCES}
)

add_executable(app-builder ${SOURCES})

# 包含路径
target_include_directories(app-builder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

# 通过编译定义传递安装器路径
target_compile_definitions(app-builder PRIVATE 
    INSTALLER_PATH="${INSTALLER_PATH}"
)
