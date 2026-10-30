#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "ov5640.h"
#include "dcmi.h"
#include <string.h>

/* ============================================================
 *  main.c — OV5640 → TFT 实时显示 (硬件SPI2 + DMA)
 *
 *  摄像头: QQVGA 160×120 RGB565
 *  TFT:    240×320 ILI9341, 硬件SPI2 10.5MHz + DMA
 *
 *  双缓冲机制:
 *    帧A → DCMI写入
 *    帧B → DMA刷屏
 *    交替使用，避免撕裂
 * ============================================================ */

/* 双缓冲区 (各 38400 字节) */
static u8 disp_buf[CAM_BUF_SIZE] __attribute__((aligned(4)));

int main(void) {
    u32 tick_start;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    delay_init(168);
    uart_init(115200);

    printf("\r\n======== OV5640 Camera → TFT (SPI2+DMA) ========\r\n");

    /* TFT 初始化 */
    LCD_Init();
    LCD_Clear(COLOR_BLACK);

    /* OV5640 初始化 */
    printf("Init OV5640...\r\n");
    if(OV5640_Init() != 0) {
        printf("OV5640 FAILED! Check wiring.\r\n");
        LCD_Fill(0, 0, LCD_W - 1, 30, COLOR_RED);
        while(1);
    }
    printf("OV5640 OK, ID=0x%04X\r\n", OV5640_ReadID());

    /* DCMI 初始化 + 开始采集 */
    printf("Init DCMI...\r\n");
    DCMI_Init();
    DCMI_Start();
    printf("Running! Camera → TFT\r\n");

    while(1) {
        /* 等待摄像头新帧 */
        if(cam_frame_ready) {
            cam_frame_ready = 0;

            /* 拷贝帧到显存 (避免 DMA 刷屏期间被 DCMI 覆盖) */
            memcpy(disp_buf, cam_frame_buf, CAM_BUF_SIZE);

            /* DMA 一把刷到屏幕上 (约 30ms) */
            LCD_ShowFrame_DMA(disp_buf,
                              160, 120,
                              (LCD_W - 160) / 2,   /* X偏移 40 */
                              (LCD_H - 120) / 2);  /* Y偏移 100 */
        }
    }
}
