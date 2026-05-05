#!/bin/bash
#
# AppPack 构建脚本
# 用法: ./build.sh <command> [options]
#
# 命令:
#   build        构建所有程序（打包工具、安装器运行时、示例应用）
#   package      将工具链打包为可分发的 .apppack 安装包
#   test         构建并运行单元测试
#   test-build   仅构建测试用例（不运行）
#   clean        清除编译生成物
#   help         显示此帮助信息
#
set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

# ============================================================
# 显示帮助信息
# ============================================================
show_help() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  AppPack - 自解压安装包构建工具${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    echo "用法: ./build.sh <command> [options]"
    echo ""
    echo "命令:"
    echo "  build        构建所有程序（打包工具、安装器运行时、示例应用）"
    echo "  package      将工具链打包为可分发的 .apppack 安装包"
    echo "  test         构建并运行单元测试"
    echo "  test-build   仅构建测试用例（不运行）"
    echo "  clean        清除编译生成物"
    echo "  help         显示此帮助信息"
    echo ""
    echo "选项:"
    echo "  --no-example    在 build 命令中跳过构建示例应用"
    echo "  --output=<文件> 在 package 命令中指定输出路径"
    echo ""
    echo "示例:"
    echo "  ./build.sh build              # 构建所有程序"
    echo "  ./build.sh build --no-example # 仅构建工具链，跳过示例"
    echo "  ./build.sh package            # 打包工具链为安装包"
    echo "  ./build.sh test               # 构建并运行测试"
    echo "  ./build.sh test-build         # 仅构建测试"
    echo "  ./build.sh clean              # 清理构建产物"
    echo ""
}

# ============================================================
# 检查构建环境
# ============================================================
check_environment() {
    echo -e "${YELLOW}[检查] 构建环境...${NC}"
    MISSING_TOOLS=""
    MISSING_DEVEL=""

    check_tool() {
        if ! command -v "$1" &> /dev/null; then
            MISSING_TOOLS="$MISSING_TOOLS $1"
        fi
    }

    check_devel() {
        if [ ! -f "$1" ]; then
            MISSING_DEVEL="$MISSING_DEVEL $2"
        fi
    }

    check_tool cmake
    check_tool g++
    check_tool make
    check_tool zstd
    check_tool ldd

    # 检查开发头文件（可选，如果缺少则编译控制台版本）
    check_devel "/usr/include/zstd.h" "libzstd-dev"

    if [ -n "$MISSING_DEVEL" ]; then
        echo -e "${YELLOW}  提示: 缺少开发包，示例应用将编译为控制台版本${NC}"
        echo -e "${YELLOW}  如需 GUI 版本，请安装:${NC}"
        echo "    Ubuntu/Debian: sudo apt-get install libx11-dev libzstd-dev"
        echo "    Fedora:        sudo dnf install libX11-devel libzstd-devel"
        echo "    Arch:          sudo pacman -S libx11 zstd"
        echo ""
    fi

    if [ -n "$MISSING_TOOLS" ]; then
        echo -e "${RED}错误: 缺少以下工具:${NC}"
        for tool in $MISSING_TOOLS; do
            echo "  - $tool"
        done
        echo ""
        echo "请安装缺失的工具，例如:"
        echo "  Ubuntu/Debian: sudo apt-get install cmake g++ make zstd"
        echo "  Fedora: sudo dnf install cmake gcc-c++ make zstd"
        echo "  Arch: sudo pacman -S cmake gcc make zstd"
        exit 1
    fi
    echo -e "${GREEN}  ✓ 构建环境检查通过${NC}"
    echo ""
}

# ============================================================
# 构建安装器运行时
# ============================================================
build_installer() {
    echo -e "${YELLOW}[构建] 安装器运行时...${NC}"
    INSTALLER_BUILD_DIR="${BUILD_DIR}/installer"
    mkdir -p "${INSTALLER_BUILD_DIR}"
    cd "${INSTALLER_BUILD_DIR}"
    cmake "${PROJECT_DIR}/installer" -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -3
    make -j$(nproc) 2>&1 | tail -5
    if [ -f "${INSTALLER_BUILD_DIR}/app-installer" ]; then
        echo -e "${GREEN}  ✓ 安装器运行时构建成功${NC}"
    else
        echo -e "${RED}  ✗ 安装器运行时构建失败${NC}"
        exit 1
    fi
    echo ""
}

# ============================================================
# 构建打包工具
# ============================================================
build_builder() {
    echo -e "${YELLOW}[构建] 打包工具...${NC}"
    BUILDER_BUILD_DIR="${BUILD_DIR}/builder"
    mkdir -p "${BUILDER_BUILD_DIR}"
    cd "${BUILDER_BUILD_DIR}"
    cmake "${PROJECT_DIR}/builder" \
        -DCMAKE_BUILD_TYPE=Release \
        -DINSTALLER_PATH="${BUILD_DIR}/installer/app-installer" 2>&1 | tail -3
    make -j$(nproc) 2>&1 | tail -5
    if [ -f "${BUILDER_BUILD_DIR}/app-builder" ]; then
        cp "${BUILDER_BUILD_DIR}/app-builder" "${BUILD_DIR}/app-builder"
        echo -e "${GREEN}  ✓ 打包工具构建成功${NC}"
    else
        echo -e "${RED}  ✗ 打包工具构建失败${NC}"
        exit 1
    fi
    echo ""
}

# ============================================================
# 构建示例应用
# ============================================================
build_example() {
    echo -e "${YELLOW}[构建] 示例应用...${NC}"
    HELLO_BUILD_DIR="${BUILD_DIR}/hello"
    mkdir -p "${HELLO_BUILD_DIR}"
    cd "${HELLO_BUILD_DIR}"
    cmake "${PROJECT_DIR}/examples/hello" -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -3
    make -j$(nproc) 2>&1 | tail -5
    if [ -f "${HELLO_BUILD_DIR}/hello" ]; then
        echo -e "${GREEN}  ✓ 示例应用构建成功${NC}"
    else
        echo -e "${RED}  ✗ 示例应用构建失败${NC}"
        exit 1
    fi
    echo ""
}

# ============================================================
# 打包示例应用
# ============================================================
package_example() {
    echo -e "${YELLOW}[打包] 示例应用...${NC}"
    HELLO_APPDIR="${PROJECT_DIR}/examples/hello/apppack"
    HELLO_BUILD_DIR="${BUILD_DIR}/hello"

    # 将编译好的可执行文件复制到 apppack/usr/bin/
    cp "${HELLO_BUILD_DIR}/hello" "${HELLO_APPDIR}/usr/bin/"

    echo -e "${GREEN}  ✓ 可执行文件已复制到 apppack/usr/bin/${NC}"

    # 使用 --build 方式打包（从 apppack/ 目录读取配置）
    echo -e "${YELLOW}  正在打包示例应用 (从 apppack/ 模板)...${NC}"
    cd "${PROJECT_DIR}/examples/hello"
    "${BUILD_DIR}/app-builder" --build --bundle-deps

    # 复制生成的安装包到 build 目录
    cp "${PROJECT_DIR}/examples/hello/HelloWorld-1.0.0.apppack" "${BUILD_DIR}/HelloWorld-v1.0.0-bundled.apppack"

    # 也打包一个不带依赖的版本
    echo -e "${YELLOW}  正在打包示例应用 (不含依赖)...${NC}"
    "${BUILD_DIR}/app-builder" --build --output="${BUILD_DIR}/HelloWorld-v1.0.0.apppack"

    # 创建最新版本链接
    cp "${BUILD_DIR}/HelloWorld-v1.0.0.apppack" "${BUILD_DIR}/HelloWorld.apppack"

    echo -e "${GREEN}  ✓ 示例应用打包完成${NC}"
    echo ""
}

# ============================================================
# 构建测试用例
# ============================================================
build_tests() {
    echo -e "${YELLOW}[构建] 单元测试...${NC}"
    TEST_BUILD_DIR="${BUILD_DIR}/tests"
    mkdir -p "${TEST_BUILD_DIR}"
    cd "${TEST_BUILD_DIR}"
    cmake "${PROJECT_DIR}/tests" -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -3
    make -j$(nproc) 2>&1 | tail -5
    echo -e "${GREEN}  ✓ 测试用例构建成功${NC}"
    echo ""
}

# ============================================================
# 运行单元测试
# ============================================================
run_tests() {
    echo -e "${YELLOW}[测试] 运行单元测试...${NC}"
    TEST_BUILD_DIR="${BUILD_DIR}/tests"
    if [ ! -d "${TEST_BUILD_DIR}" ]; then
        build_tests
    fi
    cd "${TEST_BUILD_DIR}"
    ctest --output-on-failure 2>&1
    echo ""
    echo -e "${GREEN}  ✓ 测试完成${NC}"
    echo ""
}

# ============================================================
# 打包工具链为 .apppack 安装包
# ============================================================
package_toolchain() {
    local output_path="$1"

    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  打包 AppPack 工具链${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""

    # 确保工具链已构建
    if [ ! -f "${BUILD_DIR}/app-builder" ] || [ ! -f "${BUILD_DIR}/installer/app-installer" ]; then
        echo -e "${YELLOW}工具链尚未构建，先执行构建...${NC}"
        cmd_build true
    fi

    # 准备 apppack 目录
    local APPPACK_DIR="${PROJECT_DIR}/apppack"
    mkdir -p "${APPPACK_DIR}/usr/bin"

    # 复制可执行文件
    echo -e "${YELLOW}[1/4] 复制可执行文件...${NC}"
    cp "${BUILD_DIR}/app-builder" "${APPPACK_DIR}/usr/bin/"
    cp "${BUILD_DIR}/installer/app-installer" "${APPPACK_DIR}/usr/bin/"
    chmod +x "${APPPACK_DIR}/usr/bin/"*
    echo -e "${GREEN}  ✓ 已复制: app-builder, app-installer${NC}"
    echo ""

    # 复制文档
    echo -e "${YELLOW}[2/4] 复制文档...${NC}"
    mkdir -p "${APPPACK_DIR}/usr/share/doc/apppack"
    cp "${PROJECT_DIR}/README.md" "${APPPACK_DIR}/usr/share/doc/apppack/" 2>/dev/null || true
    cp -r "${PROJECT_DIR}/doc/"* "${APPPACK_DIR}/usr/share/doc/apppack/" 2>/dev/null || true
    echo -e "${GREEN}  ✓ 文档已复制${NC}"
    echo ""

    # 复制构建脚本和 CMake 模板（方便在其他系统上重新构建）
    echo -e "${YELLOW}[3/4] 复制构建支持文件...${NC}"
    mkdir -p "${APPPACK_DIR}/usr/share/apppack"
    cp "${PROJECT_DIR}/build.sh" "${APPPACK_DIR}/usr/share/apppack/"
    echo -e "${GREEN}  ✓ 构建支持文件已复制${NC}"
    echo ""

    # 使用 app-builder 打包
    echo -e "${YELLOW}[4/4] 生成 .apppack 安装包...${NC}"
    cd "${PROJECT_DIR}"

    if [ -n "$output_path" ]; then
        "${BUILD_DIR}/app-builder" --appdir="${APPPACK_DIR}" --output="${output_path}" --bundle-deps
    else
        "${BUILD_DIR}/app-builder" --appdir="${APPPACK_DIR}" --output="${BUILD_DIR}/AppPack-v1.0.0.apppack" --bundle-deps
        # 创建最新版本链接
        cp "${BUILD_DIR}/AppPack-v1.0.0.apppack" "${BUILD_DIR}/AppPack.apppack"
    fi

    echo ""
    echo -e "${GREEN}  ✓ AppPack 工具链打包完成!${NC}"
    echo ""

    if [ -n "$output_path" ]; then
        echo "安装包: ${output_path}"
    else
        echo "安装包: ${BUILD_DIR}/AppPack-v1.0.0.apppack"
        echo "        ${BUILD_DIR}/AppPack.apppack (最新版本链接)"
    fi
    echo ""
    echo "安装方法:"
    echo "  sudo ${BUILD_DIR}/AppPack.apppack --install"
    echo ""
    echo "安装后即可在终端中使用:"
    echo "  app-builder --help"
    echo ""
}

# ============================================================
# 清除编译生成物
# ============================================================
cmd_clean() {
    echo -e "${YELLOW}[清理] 清除编译生成物...${NC}"

    # 删除 build 目录
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
        echo -e "${GREEN}  ✓ 已删除 build 目录${NC}"
    fi

    # 删除 apppack/usr/bin 中的可执行文件（这些是构建产物）
    if [ -f "${PROJECT_DIR}/apppack/usr/bin/app-builder" ]; then
        rm -f "${PROJECT_DIR}/apppack/usr/bin/app-builder"
        echo -e "${GREEN}  ✓ 已清理 apppack/usr/bin/app-builder${NC}"
    fi
    if [ -f "${PROJECT_DIR}/apppack/usr/bin/app-installer" ]; then
        rm -f "${PROJECT_DIR}/apppack/usr/bin/app-installer"
        echo -e "${GREEN}  ✓ 已清理 apppack/usr/bin/app-installer${NC}"
    fi

    # 删除 apppack/usr/share 目录
    if [ -d "${PROJECT_DIR}/apppack/usr/share" ]; then
        rm -rf "${PROJECT_DIR}/apppack/usr/share"
        echo -e "${GREEN}  ✓ 已清理 apppack/usr/share${NC}"
    fi

    # 删除示例应用中编译的可执行文件
    if [ -f "${PROJECT_DIR}/examples/hello/apppack/usr/bin/hello" ]; then
        # 只删除通过构建脚本复制过去的（保留原始文件）
        local HELLO_BUILD_FILE="${BUILD_DIR}/hello/hello"
        if [ ! -f "$HELLO_BUILD_FILE" ]; then
            rm -f "${PROJECT_DIR}/examples/hello/apppack/usr/bin/hello"
            echo -e "${GREEN}  ✓ 已清理 examples/hello/apppack/usr/bin/hello${NC}"
        fi
    fi

    # 删除生成的 .apppack 文件
    find "${PROJECT_DIR}" -name "*.apppack" -not -path "*/build/*" -exec rm -f {} \; 2>/dev/null || true

    echo ""
    echo -e "${GREEN}  ✓ 清理完成${NC}"
    echo ""
}

# ============================================================
# 构建命令 (build)
# ============================================================
cmd_build() {
    local skip_example="$1"

    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  AppPack - 自解压安装包构建工具${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""

    check_environment

    # 创建构建目录
    mkdir -p "${BUILD_DIR}"

    build_installer
    build_builder

    if [ "$skip_example" != "true" ]; then
        build_example
        package_example
    fi

    echo -e "${BLUE}========================================${NC}"
    echo -e "${GREEN}  AppPack 构建完成!${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    echo "构建产物:"
    echo "  ${BUILD_DIR}/app-builder                          - 打包工具"
    echo "  ${BUILD_DIR}/HelloWorld-v1.0.0.apppack            - 示例安装包 (基础)"
    echo "  ${BUILD_DIR}/HelloWorld-v1.0.0-bundled.apppack    - 示例安装包 (含依赖)"
    echo "  ${BUILD_DIR}/HelloWorld.apppack                   - 最新版本链接"
    echo ""
    echo "使用方法:"
    echo "  1. 安装示例应用:"
    echo "     sudo ${BUILD_DIR}/HelloWorld.apppack --install"
    echo ""
    echo "  2. 查看包信息:"
    echo "     ${BUILD_DIR}/HelloWorld.apppack --info"
    echo ""
    echo "  3. 打包工具链为安装包:"
    echo "     ./build.sh package"
    echo ""
    echo "  4. 运行单元测试:"
    echo "     ./build.sh test"
    echo ""
}

# ============================================================
# 主入口
# ============================================================
main() {
    if [ $# -lt 1 ]; then
        show_help
        exit 0
    fi

    local command="$1"
    shift

    case "$command" in
        build)
            local skip_example="false"
            for arg in "$@"; do
                case "$arg" in
                    --no-example) skip_example="true" ;;
                    --help|-h) show_help; exit 0 ;;
                esac
            done
            cmd_build "$skip_example"
            ;;

        package)
            local output_path=""
            for arg in "$@"; do
                case "$arg" in
                    --output=*) output_path="${arg#*=}" ;;
                    --help|-h) show_help; exit 0 ;;
                esac
            done
            package_toolchain "$output_path"
            ;;

        test)
            check_environment
            mkdir -p "${BUILD_DIR}"
            build_tests
            run_tests
            ;;

        test-build)
            check_environment
            mkdir -p "${BUILD_DIR}"
            build_tests
            echo -e "${GREEN}测试用例构建完成，使用以下命令运行:${NC}"
            echo "  ./build.sh test"
            echo "  # 或直接运行:"
            echo "  ${BUILD_DIR}/tests/test_common"
            echo "  ${BUILD_DIR}/tests/test_packager"
            echo "  ${BUILD_DIR}/tests/test_installer"
            echo "  ${BUILD_DIR}/tests/test_deps"
            echo "  ${BUILD_DIR}/tests/test_integration"
            echo ""
            ;;

        clean)
            cmd_clean
            ;;

        help|--help|-h)
            show_help
            ;;

        *)
            echo -e "${RED}错误: 未知命令 '$command'${NC}"
            echo ""
            show_help
            exit 1
            ;;
    esac
}

main "$@"
