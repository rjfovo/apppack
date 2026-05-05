cmake_minimum_required(VERSION 3.10)
project(HelloWorld CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 尝试查找 X11（可选，如果找不到则编译为控制台版本）
find_package(X11 QUIET)

if(X11_FOUND)
    message(STATUS "X11 found, building GUI version")
    add_definitions(-DHAS_X11)
    add_executable(hello main.cpp)
    target_link_libraries(hello PRIVATE ${X11_LIBRARIES})
    target_include_directories(hello PRIVATE ${X11_INCLUDE_DIR})
else()
    message(STATUS "X11 not found, building console version")
    add_executable(hello main.cpp)
endif()
