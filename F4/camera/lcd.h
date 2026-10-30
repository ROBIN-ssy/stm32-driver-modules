#ifndef __LCD_H
#define __LCD_H

#include "sys.h"

/* 屏尺寸 */
#define LCD_W   240
#define LCD_H   320

/* ---- 管脚宏 (与你的压力传感器工程一致) ---- */
#define LCD_BLK_PORT   GPIOB
#define LCD_BLK_PIN    GPIO_Pin_1
#define LCD_RS_PORT    GPIOC
#define LCD_RS_PIN     GPIO_Pin_5
#define LCD_CS_PORT    GPIOB
#define LCD_CS_PIN     GPIO_Pin_12
#define LCD_SCL_PORT   GPIOB
#define LCD_SCL_PIN    GPIO_Pin_13
#define LCD_RST_PORT   GPIOB
#define LCD_RST_PIN    GPIO_Pin_14
#define LCD_SDA_PORT   GPIOB
#define LCD_SDA_PIN    GPIO_Pin_15

#define LCD_CS_SET()   GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN)
#define LCD_CS_CLR()   GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN)
#define LCD_RS_SET()   GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN)
#define LCD_RS_CLR()   GPIO_ResetBits(LCD_RS_PORT, LCD_RS_PIN)
#define LCD_RST_SET()  GPIO_SetBits(LCD_RST_PORT, LCD_RST_PIN)
#define LCD_RST_CLR()  GPIO_ResetBits(LCD_RST_PORT, LCD_RST_PIN)
#define LCD_BLK_SET()  GPIO_SetBits(LCD_BLK_PORT, LCD_BLK_PIN)
#define LCD_BLK_CLR()  GPIO_ResetBits(LCD_BLK_PORT, LCD_BLK_PIN)

/* ---- 颜色 ---- */
#define COLOR_WHITE   0xFFFF
#define COLOR_BLACK   0x0000
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F

/* ---- API ---- */
void LCD_Init(void);
void LCD_SetWindow(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_DrawPoint(u16 x, u16 y, u16 color);
void LCD_Fill(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void LCD_Clear(u16 color);

/* DMA 一次性刷图（替代逐行软件SPI）*/
void LCD_ShowFrame_DMA(const u8 *pixels, u16 w, u16 h,
                       u16 offset_x, u16 offset_y);

#endif
