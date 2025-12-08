#ifndef _ANSI_H_
#define _ANSI_H_

// ANSI转义序列定义
#define ESC "\033"

// 重置所有属性
#define ANSI_RESET       ESC "[0m"

// 前景色（文字颜色）
#define ANSI_BLACK       ESC "[30m"
#define ANSI_RED         ESC "[31m"
#define ANSI_GREEN       ESC "[32m"
#define ANSI_YELLOW      ESC "[33m"
#define ANSI_BLUE        ESC "[34m"
#define ANSI_MAGENTA     ESC "[35m"
#define ANSI_CYAN        ESC "[36m"
#define ANSI_WHITE       ESC "[37m"

// 背景色
#define ANSI_BG_BLACK    ESC "[40m"
#define ANSI_BG_RED      ESC "[41m"
#define ANSI_BG_GREEN    ESC "[42m"
#define ANSI_BG_YELLOW   ESC "[43m"
#define ANSI_BG_BLUE     ESC "[44m"
#define ANSI_BG_MAGENTA  ESC "[45m"
#define ANSI_BG_CYAN     ESC "[46m"
#define ANSI_BG_WHITE    ESC "[47m"

// 文本样式
#define ANSI_BOLD        ESC "[1m"
#define ANSI_DIM         ESC "[2m"
#define ANSI_UNDERLINE   ESC "[4m"
#define ANSI_BLINK       ESC "[5m"
#define ANSI_REVERSE     ESC "[7m"

// 便捷颜色组合
#define ANSI_ERROR       ANSI_BOLD ANSI_RED
#define ANSI_WARNING     ANSI_BOLD ANSI_YELLOW
#define ANSI_SUCCESS     ANSI_BOLD ANSI_GREEN
#define ANSI_INFO        ANSI_BOLD ANSI_CYAN

#endif // _ANSI_H_