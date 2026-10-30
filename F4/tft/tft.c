#include "tft.h"
#include "delay.h"

/* ============================================================
 *  初始化与屏幕控制
 * ============================================================ */

void TFT_Init(void)
{
	LCD_Init();
}

void TFT_Clear(u16 color)
{
	LCD_Fill(0, 0, LCD_W, LCD_H, color);
}

void TFT_ClearWhite(void)
{
	LCD_Fill(0, 0, LCD_W, LCD_H, TFT_WHITE);
}

void TFT_ClearBlack(void)
{
	LCD_Fill(0, 0, LCD_W, LCD_H, TFT_BLACK);
}

void TFT_BacklightOn(void)
{
	LCD_BLK_Set();
}

void TFT_BacklightOff(void)
{
	LCD_BLK_Clr();
}


/* ============================================================
 *  基础绘图
 * ============================================================ */

void TFT_DrawPoint(u16 x, u16 y, u16 color)
{
	LCD_DrawPoint(x, y, color);
}

void TFT_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	LCD_DrawLine(x1, y1, x2, y2, color);
}

/* 水平线——直接设置窗口行，连续写像素，比通用Bresenham画线快 */
void TFT_DrawHLine(u16 x, u16 y, u16 w, u16 color)
{
	u16 i;
	LCD_Address_Set(x, y, x + w - 1, y);
	for(i = 0; i < w; i++)
		LCD_WR_DATA(color);
}

/* 垂直线 */
void TFT_DrawVLine(u16 x, u16 y, u16 h, u16 color)
{
	u16 i;
	LCD_Address_Set(x, y, x, y + h - 1);
	for(i = 0; i < h; i++)
		LCD_WR_DATA(color);
}

void TFT_DrawRect(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	LCD_DrawRectangle(x1, y1, x2, y2, color);
}

void TFT_FillRect(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	LCD_Fill(x1, y1, x2, y2, color);
}

void TFT_DrawCircle(u16 x0, u16 y0, u8 r, u16 color)
{
	Draw_Circle(x0, y0, r, color);
}

/* 填充圆——逐行画水平弦 */
void TFT_FillCircle(u16 x0, u16 y0, u8 r, u16 color)
{
	s16 x = r;
	s16 y = 0;
	s16 err = 1 - r;

	while(x >= y)
	{
		/* 画上下对称的水平弦 */
		TFT_DrawHLine(x0 - x, y0 + y, 2 * x, color);
		TFT_DrawHLine(x0 - x, y0 - y, 2 * x, color);
		TFT_DrawHLine(x0 - y, y0 + x, 2 * y, color);
		TFT_DrawHLine(x0 - y, y0 - x, 2 * y, color);

		y++;
		if(err < 0)
		{
			err += 2 * y + 1;
		}
		else
		{
			x--;
			err += 2 * (y - x) + 1;
		}
	}
}

void TFT_DrawTriangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 x3, u16 y3, u16 color)
{
	TFT_DrawLine(x1, y1, x2, y2, color);
	TFT_DrawLine(x2, y2, x3, y3, color);
	TFT_DrawLine(x3, y3, x1, y1, color);
}

/* 填充三角形——用三角形两条边逐行插值画水平线 */
void TFT_FillTriangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 x3, u16 y3, u16 color)
{
	u16 t;
	u16 xa, xb;
	u16 ymin, ymax;
	s16 dx12, dx13, dx23;
	u16 y;

	/* 找y范围 */
	ymin = y1 < y2 ? y1 : y2;
	if(y3 < ymin) ymin = y3;
	ymax = y1 > y2 ? y1 : y2;
	if(y3 > ymax) ymax = y3;

	/* 退化情况：三点共线或面积太小，退化为画线 */
	if(ymin == ymax)
	{
		TFT_DrawTriangle(x1, y1, x2, y2, x3, y3, color);
		return;
	}

	for(y = ymin; y <= ymax; y++)
	{
		/* 边1: (x1,y1)-(x3,y3) */
		if(y3 != y1)
			xa = x1 + (u16)((s32)(x3 - x1) * (s32)(y - y1) / (s32)(y3 - y1));
		else
			xa = x1;

		/* 边2: 根据y在哪段选不同边 */
		if(y < y2)
		{
			if(y2 != y1)
				xb = x1 + (u16)((s32)(x2 - x1) * (s32)(y - y1) / (s32)(y2 - y1));
			else
				xb = x2;
		}
		else
		{
			if(y3 != y2)
				xb = x2 + (u16)((s32)(x3 - x2) * (s32)(y - y2) / (s32)(y3 - y2));
			else
				xb = x3;
		}

		/* 保证xa <= xb */
		if(xa > xb) { t = xa; xa = xb; xb = t; }

		TFT_DrawHLine(xa, y, xb - xa + 1, color);
	}
}


/* ============================================================
 *  文字显示 (内置点阵字体)
 * ============================================================ */

void TFT_ShowChar(u16 x, u16 y, char ch, u16 fc, u16 bc, u8 size, u8 mode)
{
	LCD_ShowChar(x, y, (u8)ch, fc, bc, size, mode);
}

void TFT_ShowString(u16 x, u16 y, const char *str, u16 fc, u16 bc, u8 size, u8 mode)
{
	LCD_ShowString(x, y, (const u8 *)str, fc, bc, size, mode);
}

void TFT_ShowInt(u16 x, u16 y, u32 num, u8 len, u16 fc, u16 bc, u8 size)
{
	u8 t, temp;
	u8 enshow = 0;
	u8 sizex = size / 2;

	for(t = 0; t < len; t++)
	{
		temp = (num / mypow(10, len - t - 1)) % 10;
		if(enshow == 0 && t < (len - 1))
		{
			if(temp == 0)
			{
				LCD_ShowChar(x + t * sizex, y, ' ', fc, bc, size, 0);
				continue;
			}
			else enshow = 1;
		}
		LCD_ShowChar(x + t * sizex, y, temp + 48, fc, bc, size, 0);
	}
}

void TFT_ShowFloat(u16 x, u16 y, float num, u8 len, u16 fc, u16 bc, u8 size)
{
	LCD_ShowFloatNum1(x, y, num, len, fc, bc, size);
}

void TFT_ShowChinese(u16 x, u16 y, const u8 *s, u16 fc, u16 bc, u8 size, u8 mode)
{
	LCD_ShowChinese(x, y, (u8 *)s, fc, bc, size, mode);
}


/* ============================================================
 *  文字显示 (字库ROM芯片)
 * ============================================================ */

void TFT_ShowGB2312(u16 x, u16 y, u8 zk_num, const u8 *text, u16 fc, u16 bc)
{
	Display_GB2312_String(x, y, zk_num, (u8 *)text, fc, bc);
}

void TFT_ShowAsciiROM(u16 x, u16 y, u8 zk_num, const u8 *text, u16 fc, u16 bc)
{
	Display_Asc_String(x, y, zk_num, (u8 *)text, fc, bc);
}

void TFT_ShowArial(u16 x, u16 y, u8 zk_num, const u8 *text, u16 fc, u16 bc)
{
	Display_Arial_String(x, y, zk_num, (u8 *)text, fc, bc);
}

void TFT_ShowTimesNewRoman(u16 x, u16 y, u8 zk_num, const u8 *text, u16 fc, u16 bc)
{
	Display_TimesNewRoman_String(x, y, zk_num, (u8 *)text, fc, bc);
}


/* ============================================================
 *  图片显示
 * ============================================================ */

void TFT_ShowImage(u16 x, u16 y, u16 w, u16 h, const u8 *pic)
{
	LCD_ShowPicture(x, y, w, h, pic);
}


/* ============================================================
 *  工具函数
 * ============================================================ */

u16 TFT_RGB565(u8 r, u8 g, u8 b)
{
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void TFT_SetWindow(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_Address_Set(x1, y1, x2, y2);
}

void TFT_WritePixel(u16 color)
{
	LCD_WR_DATA(color);
}
