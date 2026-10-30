#ifndef __OV5640_H
#define __OV5640_H

#include "sys.h"
#include "stm32f4xx_dcmi.h"

/* OV5640 ID */
#define OV5640_PID                   0x5640
#define OV5640_ADDR_WRITE            0x78
#define OV5640_ADDR_READ             0x79

/* 输出分辨率 */
typedef enum {
    OV5640_MODE_QQVGA_160x120  = 0,
    OV5640_MODE_QVGA_320x240   = 1,
    OV5640_MODE_VGA_640x480    = 2,
} ov5640_mode_t;

/* DCMI帧缓冲区 - QQVGA 160x120 RGB565 */
#define CAM_BUF_SIZE    (160 * 120 * 2)     /* 38400 bytes */
extern u8  cam_frame_buf[CAM_BUF_SIZE];
extern u8  cam_frame_ready;

/* ---- 函数声明 ---- */
u8   OV5640_Init(void);
u8   OV5640_ReadID(void);
void OV5640_SetMode(ov5640_mode_t mode);
void OV5640_SetLightMode(u8 mode);
void OV5640_ColorSaturation(u8 sat);
void OV5640_Brightness(u8 bright);
void OV5640_Contrast(u8 contrast);
void OV5640_Sharpness(u8 sharp);
void OV5640_Flip(u8 flip);

#endif
