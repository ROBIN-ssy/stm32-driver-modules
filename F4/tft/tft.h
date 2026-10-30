#ifndef __TFT_H
#define __TFT_H

#include "sys.h"
#include "lcd_init.h"
#include "lcd.h"

/* ============================================================
 *  TFT 便捷封装层
 *  基于 ST7796S / ILI9341 驱动，软件模拟 SPI
 *  屏幕分辨率: 240x320 (竖屏) 或 320x240 (横屏)
 *  使用方法: 只需 #include "tft.h"，然后调用 TFT_Init() 即可
 * ============================================================ */

/* -------------------- 颜色定义 (RGB565) -------------------- */
#define TFT_WHITE       0xFFFF
#define TFT_BLACK       0x0000
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_YELLOW      0xFFE0
#define TFT_CYAN        0x7FFF
#define TFT_MAGENTA     0xF81F
#define TFT_BROWN       0xBC40
#define TFT_BRRED       0xFC07
#define TFT_GRAY        0x8430
#define TFT_DARKBLUE    0x01CF
#define TFT_LIGHTBLUE   0x7D7C
#define TFT_LIGHTGREEN  0x841F
#define TFT_LGRAY       0xC618

/* -------------------- 字号枚举 -------------------- */
/* 内置点阵字体 (取自 lcdfont.h) */
#define TFT_FONT_12     12      /* 6x12  ASCII */
#define TFT_FONT_16     16      /* 8x16  ASCII */
#define TFT_FONT_24     24      /* 12x24 ASCII */
#define TFT_FONT_32     32      /* 16x32 ASCII */

/* 字库ROM字号编号 */
#define TFT_ZK_CN_12    1       /* 12x12 汉字 */
#define TFT_ZK_CN_16    2       /* 16x16 汉字 */
#define TFT_ZK_CN_24    3       /* 24x24 汉字 */
#define TFT_ZK_CN_32    4       /* 32x32 汉字 */

#define TFT_ZK_ASC_57   1       /* 5x7   ASCII */
#define TFT_ZK_ASC_78   2       /* 7x8   ASCII */
#define TFT_ZK_ASC_612  3       /* 6x12  ASCII */
#define TFT_ZK_ASC_816  4       /* 8x16  ASCII */
#define TFT_ZK_ASC_1224 5       /* 12x24 ASCII */
#define TFT_ZK_ASC_1632 6       /* 16x32 ASCII */

#define TFT_ZK_ARIAL_812  1     /* 8x12  Arial */
#define TFT_ZK_ARIAL_1216 2     /* 12x16 Arial */
#define TFT_ZK_ARIAL_1624 3     /* 16x24 Arial */
#define TFT_ZK_ARIAL_2432 4     /* 24x32 Arial */

#define TFT_ZK_TNR_812    1     /* 8x12  TimesNewRoman */
#define TFT_ZK_TNR_1216   2     /* 12x16 TimesNewRoman */
#define TFT_ZK_TNR_1624   3     /* 16x24 TimesNewRoman */
#define TFT_ZK_TNR_2432   4     /* 24x32 TimesNewRoman */

/* -------------------- 显示模式 -------------------- */
#define TFT_MODE_SOLID   0      /* 实心模式 (有背景色) */
#define TFT_MODE_TRANS   1      /* 透明模式 (无背景色，叠加显示) */

/* -------------------- 屏幕尺寸宏 -------------------- */
#ifndef LCD_W
#define LCD_W  240
#endif
#ifndef LCD_H
#define LCD_H  320
#endif


/* ============================================================
 *  初始化与屏幕控制
 * ============================================================ */

/* 初始化TFT屏幕 (内部完成GPIO配置+LCD寄存器初始化+清屏) */
void TFT_Init(void);

/* 用指定颜色清屏 */
void TFT_Clear(u16 color);

/* 用白色清屏 */
void TFT_ClearWhite(void);

/* 用黑色清屏 */
void TFT_ClearBlack(void);

/* 开/关背光 */
void TFT_BacklightOn(void);
void TFT_BacklightOff(void);


/* ============================================================
 *  基础绘图
 * ============================================================ */

/* 画一个点 */
void TFT_DrawPoint(u16 x, u16 y, u16 color);

/* 画一条线 (Bresenham算法) */
void TFT_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);

/* 画水平线 (比通用画线更快) */
void TFT_DrawHLine(u16 x, u16 y, u16 w, u16 color);

/* 画垂直线 (比通用画线更快) */
void TFT_DrawVLine(u16 x, u16 y, u16 h, u16 color);

/* 画矩形边框 */
void TFT_DrawRect(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);

/* 画填充矩形 */
void TFT_FillRect(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);

/* 画圆 (中点画圆法) */
void TFT_DrawCircle(u16 x0, u16 y0, u8 r, u16 color);

/* 填充圆 */
void TFT_FillCircle(u16 x0, u16 y0, u8 r, u16 color);

/* 画三角形边框 */
void TFT_DrawTriangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 x3, u16 y3, u16 color);

/* 画填充三角形 */
void TFT_FillTriangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 x3, u16 y3, u16 color);


/* ============================================================
 *  文字显示 (内置点阵字体，无需外部字库芯片)
 * ============================================================ */

/* 显示单个ASCII字符 (size: 12/16/24/32) */
void TFT_ShowChar(u16 x, u16 y, char ch, u16 fc, u16 bc, u8 size, u8 mode);

/* 显示ASCII字符串 */
void TFT_ShowString(u16 x, u16 y, const char *str, u16 fc, u16 bc, u8 size, u8 mode);

/* 显示整数 (len为显示位数，不足高位补空格) */
void TFT_ShowInt(u16 x, u16 y, u32 num, u8 len, u16 fc, u16 bc, u8 size);

/* 显示浮点数 (固定2位小数) */
void TFT_ShowFloat(u16 x, u16 y, float num, u8 len, u16 fc, u16 bc, u8 size);

/* 显示汉字串 (需在lcdfont.h中预先取模，size: 12/16/24/32) */
void TFT_ShowChinese(u16 x, u16 y, const u8 *s, u16 fc, u16 bc, u8 size, u8 mode);


/* ============================================================
 *  文字显示 (字库ROM芯片，支持任意GB2312汉字)
 * ============================================================ */

/* 显示GB2312汉字串 (zk_num: 1=12x12 2=16x16 3=24x24 4=32x32) */
void TFT_ShowGB2312(u16 x, u16 y, u8 zk_num, const u8 *text, u16 fc, u16 bc);

/* 显示ASCII字符串-字库ROM版 (zk_num: 1~6，见上方定义) */
void TFT_ShowAsciiROM(u16 x, u16 y, u8 zk_num, const u8 *text, u16 fc, u16 bc);

/* 显示Arial字体字符串 (zk_num: 1~4，见上方定义) */
void TFT_ShowArial(u16 x, u16 y, u8 zk_num, const u8 *text, u16 fc, u16 bc);

/* 显示TimesNewRoman字体字符串 (zk_num: 1~4，见上方定义) */
void TFT_ShowTimesNewRoman(u16 x, u16 y, u8 zk_num, const u8 *text, u16 fc, u16 bc);


/* ============================================================
 *  图片显示
 * ============================================================ */

/* 显示图片 (pic为RGB565格式的图片数组) */
void TFT_ShowImage(u16 x, u16 y, u16 w, u16 h, const u8 *pic);


/* ============================================================
 *  工具函数
 * ============================================================ */

/* RGB888转RGB565 */
u16  TFT_RGB565(u8 r, u8 g, u8 b);

/* 设置显示窗口 (后续连续写入像素数据时使用) */
void TFT_SetWindow(u16 x1, u16 y1, u16 x2, u16 y2);

/* 向当前窗口写入一个像素 */
void TFT_WritePixel(u16 color);

#endif /* __TFT_H */
