#ifndef __TFT_SPI_H
#define __TFT_SPI_H

#include "sys.h"

/* ============================================================
 *  TFT 硬件 SPI 底层（SPI2 + DMA1 Stream4）
 *
 *  引脚与本工程 LCD 走线完全一致，不需要改动接线：
 *      PB13 = SPI2_SCK    AF5
 *      PB14 = SPI2_MISO   AF5   —— 与板载字库芯片共用（读方向）
 *      PB15 = SPI2_MOSI   AF5
 *      PB12 = 片选（软件控制）：低 = 选中 LCD，高 = 选中字库
 *      PC5  = DC（0 = 命令，1 = 数据）
 *      PB1  = BLK 背光
 *
 *  SPI2 挂在 APB1（42MHz），分频 2 → 21MHz。
 *  整屏 240x320x2B = 153600 字节，理论上限约 58ms；
 *  实际菜单界面只做脏矩形局部刷新，单次重绘在毫秒级。
 * ============================================================ */

/* ---- 片选与控制脚：走位带，单条指令，比 GPIO 库函数快一个数量级 ---- */
#define TFT_CS_LCD()     (PBout(12) = 0)    /* 低电平选中 LCD   */
#define TFT_CS_ROM()     (PBout(12) = 1)    /* 高电平选中字库   */
#define TFT_DC_CMD()     (PCout(5)  = 0)
#define TFT_DC_DATA()    (PCout(5)  = 1)
#define TFT_BLK_ON()     (PBout(1)  = 1)
#define TFT_BLK_OFF()    (PBout(1)  = 0)

/* SPI2 波特率分频（APB1 = 42MHz）：
 *   _2 → 21MHz（默认，实测 ILI9341 可稳定工作）
 *   _4 → 10.5MHz（若出现花屏/错位就降到这档）
 *   _8 → 5.25MHz（保守） */
#define TFT_SPI_BR_21M   SPI_BaudRatePrescaler_2
#define TFT_SPI_BR_10M   SPI_BaudRatePrescaler_4
#define TFT_SPI_BR_5M    SPI_BaudRatePrescaler_8

/* 初始化 SPI2 + DMA1 + 相关 GPIO */
void TFT_SPI_Init(void);

/* 运行中修改波特率分频（传输空闲时才调用） */
void TFT_SPI_SetPrescaler(u16 prescaler);

/* 等 SPI 把最后一位移出去（抬片选前必须调用） */
void TFT_SPI_WaitIdle(void);

/* 阻塞收发一个字节（读字库用） */
u8   TFT_SPI_ReadWriteByte(u8 tx);

/* 阻塞发送一个字节（只等 TXE，适合连续发） */
void TFT_SPI_WriteByte(u8 tx);

/* 阻塞发送一批字节 */
void TFT_SPI_WriteBytes(const u8 *buf, u32 len);

/* DMA 发送一批字节（8 位帧），返回时已发送完毕 */
void TFT_SPI_WriteDMA(const u8 *buf, u32 len);

/* DMA 发送一批 RGB565 像素（自动切 16 位帧，硬件保证先发高字节） */
void TFT_SPI_WriteDMA16(const u16 *buf, u32 count);

/* 是否还有传输在进行 */
u8   TFT_SPI_IsBusy(void);

#endif
