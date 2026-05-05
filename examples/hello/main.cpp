/**
 * HelloWorld - 示例应用
 * 用于演示 AppPack 打包格式
 *
 * 编译方式:
 *   - 如果系统安装了 libx11-dev: 编译为 GUI 窗口版本
 *   - 如果未安装 libx11-dev: 编译为控制台版本（等待用户按键）
 *
 * 运行时依赖:
 *   - GUI 版本: 需要 X11 显示服务（通过 --bundle-deps 自动打包 libX11）
 *   - 控制台版本: 无额外依赖
 */

#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>

#ifdef HAS_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#endif

static const char *WINDOW_TITLE = "Hello World - AppPack 示例";

// 获取安装路径
std::string getInstallPath() {
    const char *env = getenv("APPPACK_INSTALL_DIR");
    if (env) return env;
    return "/opt/HelloWorld";
}

#ifdef HAS_X11

// ========== GUI 版本 (X11) ==========

int showX11Window() {
    static const int WINDOW_WIDTH = 520;
    static const int WINDOW_HEIGHT = 400;
    
    Display *display;
    Window window;
    XEvent event;
    int screen;
    GC gc;
    XFontStruct *font;
    unsigned long black, white;
    
    // 打开 X 显示连接
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "错误: 无法连接到 X 服务器\n");
        fprintf(stderr, "请确保在图形界面环境下运行\n");
        return 1;
    }
    
    screen = DefaultScreen(display);
    black = BlackPixel(display, screen);
    white = WhitePixel(display, screen);
    
    // 创建窗口
    window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 1,
        black, white
    );
    
    // 设置窗口标题
    XStoreName(display, window, WINDOW_TITLE);
    XSetIconName(display, window, WINDOW_TITLE);
    
    // 设置窗口关闭协议
    Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);
    
    // 选择感兴趣的事件
    XSelectInput(display, window,
        ExposureMask | ButtonPressMask | KeyPressMask | StructureNotifyMask);
    
    // 创建图形上下文
    gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, black);
    XSetBackground(display, gc, white);
    
    // 加载字体
    font = XLoadQueryFont(display, "fixed");
    if (font == NULL) {
        font = XLoadQueryFont(display, "9x15");
    }
    if (font) {
        XSetFont(display, gc, font->fid);
    }
    
    // 显示窗口
    XMapWindow(display, window);
    XFlush(display);
    
    // 文本内容
    std::string installPath = getInstallPath();
    
    const char *lines[] = {
        "========================================",
        "  Hello World! - AppPack 示例应用",
        "========================================",
        "",
        "欢迎使用 AppPack 打包格式!",
        "",
        "这个应用演示了如何将 C++ 程序打包成",
        "自解压安装包 (.apppack) 格式。",
        "",
        "特性:",
        "  \xe2\x9c\x93 自包含所有依赖",
        "  \xe2\x9c\x93 安装到指定目录",
        "  \xe2\x9c\x93 创建桌面图标",
        "  \xe2\x9c\x93 支持干净卸载",
        "  \xe2\x9c\x93 跨发行版兼容",
        "",
        ("安装路径: " + installPath).c_str(),
        "",
        "点击窗口或按任意键关闭",
        NULL
    };
    
    int running = 1;
    while (running) {
        XNextEvent(display, &event);
        
        switch (event.type) {
            case Expose:
                if (event.xexpose.count == 0) {
                    XClearWindow(display, window);
                    int y = 30;
                    for (int i = 0; lines[i] != NULL; i++) {
                        XDrawString(display, window, gc, 20, y,
                                   lines[i], strlen(lines[i]));
                        y += 20;
                    }
                    XFlush(display);
                }
                break;
                
            case ButtonPress:
            case KeyPress:
                running = 0;
                break;
                
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == wmDeleteMessage) {
                    running = 0;
                }
                break;
                
            case DestroyNotify:
                running = 0;
                break;
        }
    }
    
    // 清理
    if (font) XFreeFont(display, font);
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    
    return 0;
}

int main() {
    return showX11Window();
}

#else

// ========== 控制台版本 (无 X11) ==========

int main() {
    std::string installPath = getInstallPath();
    
    printf("========================================\n");
    printf("  Hello World! - AppPack 示例应用\n");
    printf("========================================\n");
    printf("\n");
    printf("欢迎使用 AppPack 打包格式!\n");
    printf("\n");
    printf("这个应用演示了如何将 C++ 程序打包成\n");
    printf("自解压安装包 (.apppack) 格式。\n");
    printf("\n");
    printf("特性:\n");
    printf("  ✓ 自包含所有依赖\n");
    printf("  ✓ 安装到指定目录\n");
    printf("  ✓ 创建桌面图标\n");
    printf("  ✓ 支持干净卸载\n");
    printf("  ✓ 跨发行版兼容\n");
    printf("\n");
    printf("安装路径: %s\n", installPath.c_str());
    printf("\n");
    printf("按 ENTER 键退出...\n");
    getchar();
    
    return 0;
}

#endif
