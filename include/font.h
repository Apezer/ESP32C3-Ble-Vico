#pragma once

#include <stdint.h>

// 蓝牙图标：8x8 像素
extern const uint8_t BT_ICON[];

// Claude Code logo：64x32 像素
#define CLAUDE_LOGO_WIDTH 64
#define CLAUDE_LOGO_HEIGHT 32
extern const uint8_t CLAUDE_LOGO_64X32[];

// Claude Code logo：32x16 像素，用于 Vico 状态界面
#define CLAUDE_LOGO_SMALL_WIDTH 32
#define CLAUDE_LOGO_SMALL_HEIGHT 16
extern const uint8_t CLAUDE_LOGO_32X16[];
